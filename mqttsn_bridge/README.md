# mqttsn_bridge

外部から MQTT-SN (UDP) で受けた `path` / `pause` / `resume` を ROS 2 に流し、
自己位置 `pose` を外部へ配信する、ROS とは別コンテナのブリッジ。
ブローカーは不要で、ブリッジ自身が MQTT-SN の口 (UDP 1884) を持つ。

```
外部PC ──UDP(MQTT-SN)──> mqttsn_bridge コンテナ ──ROS 2 (DDS)──> shiranui コンテナ
```

| MQTT-SN トピック | ペイロード (JSON) | 流し先 |
|---|---|---|
| `path` | `{"id":"a1","seq":0,"n":2,"poses":[[x,y,yaw],...]}` | `path` (nav_msgs/Path, transient_local) |
| `pause` | `{}` | `/pause` (std_msgs/Empty) |
| `resume` | `{}` | `/resume` (std_msgs/Empty) |
| `pose` (配信) | `{"x":1.5,"y":-2.0,"theta":0.5}` | TF `map`→`base_link` を購読者へ送る (theta は yaw [rad]) |

`pose` は外部が SUBSCRIBE したときだけ、`MQTTSN_POSE_HZ` (既定 5Hz) で QoS 0 の UDP として送る。
購読者がいなければ TF も引かない。購読は keep-alive 切れか DISCONNECT で自動的に解除される。

- 受信 (`path` / `pause` / `resume`) の全ペイロードに `"token"` が必要。ブリッジは `MQTTSN_TOKEN` 未設定だと起動しない。
- `pose` の購読にはトークンがない (MQTT-SN の SUBSCRIBE に載せる場所がない)。購読できるのは接続済みのクライアントで、
  同時に 8 台まで。位置を秘密にしたいなら LAN 内か VPN に限る。
- `path` は A* を通さず pure_pursuit_planner に直接渡る。MQTT-SN は 1 データグラム約 1KB 以内なので、
  経路は 30 点程度ずつ `seq`/`n` で分割して送る (同じ `id` のチャンクを集めて完成したら 1 回だけ publish)。
  `./tools/send.py` が自動で分割する。
- 事前定義トピック ID は `path=1`, `pause=2`, `resume=3`, `pose=4`。通常の REGISTER でもトピック名で登録できる。
- 対応: CONNECT / REGISTER / PUBLISH (QoS 0, 1, -1) / SUBSCRIBE (トピック名・事前定義 ID、配信は QoS 0) /
  PINGREQ / DISCONNECT / SEARCHGW。Will、QoS 2、ワイルドカード購読は非対応。
- 検証: トークン、数値が有限、座標 `|x|,|y| <= 1000`、経路 2〜5000 点、チャンク数 200 以下、
  未完成の経路は 5 秒で破棄、`path` は 0.2 秒間隔まで。

## 起動

```bash
echo 'MQTTSN_TOKEN=好きな文字列' >> .env
docker compose --profile mqttsn up -d          # profile を付けたときだけ起動する
```

| 環境変数 | 既定 | 説明 |
|---|---|---|
| `MQTTSN_TOKEN` | (必須) | 共有トークン |
| `MQTTSN_PORT` | `1884` | UDP ポート |
| `MQTTSN_BIND` | `0.0.0.0` | 待ち受けアドレス (LAN の特定 IF に絞るのがおすすめ) |
| `MQTTSN_FRAME` | `map` | path の frame_id / pose の基準フレーム |
| `MQTTSN_BASE_FRAME` | `base_link` | pose を取るロボットのフレーム |
| `MQTTSN_POSE_HZ` | `5` | pose の配信周期 [Hz] |
| `MQTTSN_ALLOW_NO_TOKEN` | - | `1` でトークン無し (信頼できる閉じたネットワークのみ) |

UDP 1884 を外に開けるので、LAN 内に限るかファイアウォールで制限する。MQTT-SN 自体に暗号化・認証はない
(トークンは平文)。インターネット越しに使う場合は VPN (Tailscale など) を使う。

## 送信

```bash
./tools/send.py --host <ロボットのIP> --token <トークン> path 0,0,0 1,0,0 2,1,0.5
./tools/send.py --host <ロボットのIP> --token <トークン> pause
./tools/send.py --host <ロボットのIP> --token <トークン> resume
./tools/send.py --host <ロボットのIP> pose            # 自己位置を購読して表示 (Ctrl-C で終了)
```

mqtt-sn-tools なら `mqtt-sn-pub -h <IP> -p 1884 -q 1 -t pause -m '{"token":"..."}'`、
自己位置は `mqtt-sn-sub -h <IP> -p 1884 -t pose`。

## 別コンテナ間の DDS について

Fast DDS は既定で共有メモリ (SHM) を使うが、別コンテナ間だと discovery だけ成功してデータが届かないことがある。
このイメージは `FASTDDS_BUILTIN_TRANSPORTS=UDPv4` を設定済みで、shiranui 側は既定のままで届く
(`network_mode: host`、`ROS_DOMAIN_ID` を揃える)。届かないときは両方に `ipc: host` を付ける手もある。
起動直後は discovery に数秒かかるので、すぐ送ると取りこぼすことがある。

## 開発

```bash
cd mqttsn_bridge
python3 -m unittest discover -s tests      # ROS 不要
docker build -t mqttsn-bridge .
```

`main` への push (`mqttsn_bridge/` 配下の変更) で `.github/workflows/mqttsn-bridge.yml` が
`ghcr.io/ignismirage/shiranui-mqttsn-bridge` をビルドする。**初回は GitHub Packages で Public にする**
(しないと Raspberry Pi の `docker compose pull` が `unauthorized` になる)。

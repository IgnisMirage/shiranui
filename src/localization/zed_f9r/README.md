# zed_f9r

u-blox **ZED-F9R** から **UBX（バイナリ）プロトコル**で測位データを取得する ROS 2 Python パッケージです。
UBX の組み立て・解析は SparkFun の
[sparkfun-ublox-gps](https://github.com/sparkfun/Qwiic_Ublox_Gps_Py) に任せています。

対象ハードウェアは SparkFun GPS-RTK Dead Reckoning pHat (ZED-F9R)。
基板の配線は [docs/回路図.md](../../../docs/回路図.md) を参照。

## 依存パッケージのインストール

`sparkfun-ublox-gps` は rosdep に無いので pip で入れます。**`pyserial` と `spidev` は
ライブラリが無条件に import するにもかかわらず依存として宣言されていない**ので、明示的に入れます。

```bash
pip3 install --break-system-packages sparkfun-ublox-gps pyserial spidev
```

（`spidev` は SPI を使わなくても import されるため必須です）

## 接続

pHat は Raspberry Pi の UART（GPIO 8 = TXD / 10 = RXD）につながるので、
**`/dev/serial0` のみ**を使います。受信機側の出力ポートは **UART1 固定**です。

Raspberry Pi 側の準備:

```bash
# UART を有効化し、シリアルコンソールを無効化する（raspi-config でも可）
#   /boot/firmware/config.txt   -> enable_uart=1
#   /boot/firmware/cmdline.txt  -> console=serial0,115200 を削除
sudo usermod -aG dialout $USER   # 再ログインが必要
ls -l /dev/serial0               # -> ttyAMA0 などへのシンボリックリンク
```

## 起動

```bash
ros2 launch zed_f9r zed_f9r.launch.xml
```

確認:

```bash
ros2 topic echo /fix
ros2 topic hz /fix
```

## 取得しているメッセージ

| UBX メッセージ | ライブラリの呼び出し | 用途 |
| --- | --- | --- |
| UBX-NAV-PVT | `gps.geo_coords()` | 位置・速度・精度・fix 種別 |
| UBX-NAV-ATT | `gps.veh_attitude()` | 車両姿勢（ロール/ピッチ/ヘディング） |
| UBX-CFG-VALSET | `gps.ubx_set_val()` | 出力メッセージと測位周期の設定 |

## 配信トピック

| トピック | 型 | 内容 |
| --- | --- | --- |
| `/fix` | `sensor_msgs/NavSatFix` | 緯度経度・高度、hAcc/vAcc から対角共分散 |
| `/fix_velocity` | `geometry_msgs/TwistWithCovarianceStamped` | NED→ENU 変換した対地速度、sAcc から共分散 |
| `/imu/data` | `sensor_msgs/Imu` | NAV-ATT の姿勢のみ（角速度・加速度は未設定） |

`NavSatFix.status.status` の対応:

| 受信機の状態 | status |
| --- | --- |
| fixType=0 または gnssFixOK=0 | `STATUS_NO_FIX` (-1) |
| carrSoln>0（RTK Float/Fix）または diffSoln=1 | `STATUS_GBAS_FIX` (2) |
| 上記以外の測位成功 | `STATUS_FIX` (0) |

## パラメータ

| 名前 | 既定値 | 説明 |
| --- | --- | --- |
| `port` | `/dev/serial0` | Raspberry Pi の UART |
| `baudrate` | `38400` | ZED-F9R UART1 の工場出荷値 |
| `serial_timeout` | `1.0` | pyserial の read タイムアウト [s] |
| `configure_receiver` | `true` | 起動時に CFG-VALSET を投入し、定期出力モードにする |
| `disable_nmea` | `true` | NMEA 出力を止める |
| `measurement_rate_ms` | `100` | 測位周期 [ms]（CFG-RATE-MEAS） |
| `publish_imu` | `true` | NAV-ATT を有効化して `/imu/data` を配信 |
| `use_msl_altitude` | `false` | `false`: 楕円体高（NavSatFix の規約） / `true`: 平均海面高 |
| `rate_hz` | `10.0` | 取得ループの上限レート |
| `wait_time_ms` | `1000` | 1 メッセージあたりの待ち時間 |
| `frame_id` / `imu_frame_id` | `gps_link` / `imu_link` | フレーム名 |
| `fix_topic` / `vel_topic` / `imu_topic` | 上表のとおり | トピック名 |

## 動作モード

### `configure_receiver: true`（既定）— 定期出力モード

起動時に **UBX-CFG-VALSET** を `layer=RAM` で投入し、必要なメッセージを受信機から
定期出力させます（フラッシュは書き換えないので、電源を切れば元の設定に戻ります）。

| 設定項目 | キー | 値 |
| --- | --- | --- |
| CFG-RATE-MEAS | `0x30210001` | `measurement_rate_ms` |
| CFG-RATE-NAV | `0x30210002` | 1 |
| CFG-UART1OUTPROT-UBX | `0x10740001` | 1 |
| CFG-UART1OUTPROT-NMEA | `0x10740002` | `disable_nmea` なら 0 |
| CFG-MSGOUT-UBX_NAV_PVT_UART1 | `0x20910007` | 1 |
| CFG-MSGOUT-UBX_NAV_ATT_UART1 | `0x20910020` | `publish_imu` なら 1 |

UART1 以外を使いたくなった場合、キーは `CFG-MSGOUT-*` が I2C→UART1→UART2→USB→SPI の順に `+1`、
`CFG-<port>OUTPROT-*` が同じ順に `+0x20000` の規則で並んでいます。

その後 `set_auto_msg()` でライブラリを auto モードに切り替え、`geo_coords()` が
ポーリングせず「届いた最新パケットを待つ」動作になるようにしています。

### `configure_receiver: false` — ポーリングモード

受信機の設定を一切変更せず、`geo_coords()` / `veh_attitude()` が毎回ポーリング要求を送ります。
往復のたびに待ちが入るためレートは落ちます（実測 7〜9Hz、ジッタ大）。
受信機の設定を触りたくない場合に使ってください。

## ライブラリ使用上の注意

このライブラリには扱いに癖があるので、ノード側で吸収しています。

### 1. 単位のスケーリングが一貫していない

`scale_packet()` が一部のフィールドにだけ係数を掛けます。NAV-PVT の場合:

| フィールド | ライブラリの戻り値 | 対応 |
| --- | --- | --- |
| `lat` / `lon` | deg（1e-7 適用済み） | そのまま使える |
| `height` / `hMSL` | **mm**（未変換） | `× 1e-3` |
| `hAcc` / `vAcc` | **mm × 0.1**（HPPOSLLH 用の係数が誤適用） | `× 1e-2` で m |
| `velN` / `velE` / `velD` / `sAcc` | **mm/s**（未変換） | `× 1e-3` |

NAV-ATT では `roll` / `pitch` / `heading` / `accPitch` / `accHeading` は deg に変換済みですが、
**`accRoll` だけ未変換**（1e-5 deg 単位）です。

### 2. NAV-ATT のフィールド定義がずれている

`sparkfun_predefines.py` の NAV-ATT 定義は `iTOW` と `version` の位置が実際の
インタフェース仕様と入れ替わっており、この 2 つの値は信用できません。
`roll` 以降のオフセットは正しいので、姿勢そのものは問題なく使えます。

### 3. CFG-VALSET の ACK を待てない

`ubx_set_val()` は内部で「CFG-VALSET の応答メッセージ」を待ちますが、
受信機が返すのは UBX-ACK-ACK なので必ずタイムアウトします（1 キーあたり 2.5 秒）。
そのため `wait_time=0` で送信し、ACK は `gps.packets['ACK']` を見て自前で確認しています。

### 4. `set_auto_msg()` は Gen 9 では NAK される

`set_auto_msg()` が送るのは legacy の UBX-CFG-MSG で、ZED-F9R（Gen 9）は非対応です。
受信機は NAK を返しますが、ライブラリ内部の auto 登録は行われるため実害はありません。
実際の出力設定は CFG-VALSET 側で行っています。

### 5. 受信スレッドは 1 バイトずつ 10ms 間隔で走査する

`run_packet_reader()` はフレーム同期のために 1 バイト読むごとに 10ms スリープします。
高レート・高ボーレートでの取り込みには向きません。10Hz 程度なら問題ありません。

## 未実装 / 今後の拡張

- **UBX-ESF-RAW** (`gps.esf_raw_measures()`): IMU の生角速度・加速度。`/imu/data_raw` を出すなら要追加
- **UBX-ESF-STATUS** (`gps.esf_status()`): センサフュージョン状態とキャリブレーション進捗。DR の立ち上げ確認に有用
- **RTCM3 入力**: NTRIP クライアントから RTCM を受信機へ流し込む RTK 補正
- ホイールティック入力は基板側で受け取る（[docs/回路図.md](../../../docs/回路図.md) の J4 5/7 ピン）

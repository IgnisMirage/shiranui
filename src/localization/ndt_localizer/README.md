# ndt_localizer

[ndt_omp](https://github.com/koide3/ndt_omp)（本 workspace では [tier4/ndt_omp](../ndt_omp)）を使った NDT 自己位置推定ノードです。

## パッケージ名について

| 候補 | 用途 |
|------|------|
| **ndt_localizer**（採用） | NDT による localization と一目で分かる |
| ndt_scan_matcher | Autoware 系の命名に近い |
| pclomp_ndt_localizer | ライブラリ名まで含めたい場合 |

## 依存

- `ndt_omp`（`src/localization/ndt_omp`）
- PCL / `pcl_conversions`
- TF2（`base_link` ↔ LiDAR フレーム）

## 入出力

| 方向 | トピック / TF | 内容 |
|------|----------------|------|
| 購読 | `input_topic`（既定 `/mid360/points`） | スキャン（センサ座標）。float32 の `time` フィールドがあれば deskew に使う |
| 購読 | `imu_topic`（既定 `/mid360/imu`） | deskew の回転推定に使うジャイロ |
| 購読 | `/initialpose` | 地図座標系での初期姿勢 |
| 購読 | TF `odom` → `base_link` | 初期値の予測・deskew・フレーム蓄積に使う（必須） |
| 配信 | `ndt_pose` | `map` 上の `base_link` 姿勢（共分散はヘッセ行列から推定） |
| 配信 | `localization_odom` | `map` → `odom` の Odometry |
| 配信 | TF `map` → `odom` | `publish_tf:=true` のとき。棄却時も直前の値を配信し続ける |
| 配信 | `~/nvtl` | マッチングのスコア（閾値調整用） |
| 配信 | `~/iteration_num` | NDT の反復回数 |
| 配信 | `~/aligned_points` | 位置合わせ後のスキャン（`map` 座標系、購読者がいるときだけ） |

`odom` → `base_link` は ODrive 等のオドメトリが配信する前提です。TF ツリーは `map` → `odom` → `base_link` になります。

## 処理の流れ

1. 距離（`min_scan_range`〜`max_scan_range`）で点を除外
2. deskew：スキャン中の車体の動きで点群の歪みを補正（並進はホイールオドメ、回転は IMU のジャイロ。IMU が無ければオドメ）
3. `base_link` 座標に変換し、高さ（`min_height`〜`max_height`）で除外してダウンサンプリング
4. 直近 `scan_accumulation_count` フレームをオドメで位置合わせして重ねる
5. 前回の `map` → `odom` と現在のオドメから初期値を予測して NDT
6. 採否判定：NVTL が `nvtl_threshold` 未満、またはオドメ予測から `max_translation_jump` / `max_yaw_jump_deg` 以上ずれたら棄却し、`map` → `odom` を更新しない（オドメだけで走る）

`/initialpose` 直後の 1 回はジャンプ判定をしません。また 10 回連続で棄却されたときは、オドメ予測自体がずれているとみなして NVTL だけで判定します。

## チューニング

- まず `ros2 topic echo /ndt_localizer/nvtl` で正しく合っているときの NVTL を見て、`nvtl_threshold` をそれより少し下に設定する
- NDT が 10 Hz に間に合わない場合（特に Raspberry Pi）は `source_voxel_leaf_size` を大きくする、`scan_accumulation_count` を減らす、`ndt_search_method:=2` にする
- 木の葉など季節で変わる物が多い場所では `max_height` を下げる

## 3D 地図 PCD の置き場所

2D 占有格子 (`map.yaml` / `map.png`) と同じディレクトリに置きます。

```text
src/siranui_drive/autonomous_drive/map/map.pcd
```

`colcon build` 後は `share/autonomous_drive/map/map.pcd` として参照されます。launch のデフォルト `map_pcd_path` もこのパスです。

PCD は **map 座標系**（2D 地図と原点・向きが一致）で作ってください。ファイルが大きい場合は Git LFS や実機への手動コピーを検討してください。

## 起動例

```bash
source install/setup.bash
ros2 launch ndt_localizer ndt_localizer.launch.xml
# 別パスなら
ros2 launch ndt_localizer ndt_localizer.launch.xml map_pcd_path:=/path/to/map.pcd
```

Foxglove / RViz 等で `/initialpose` を送ってからスキャンが流れるとマッチングが始まります。

## パラメータ

`launch/ndt_localizer.launch.xml` の `<arg>` で指定します。既定値は屋外向け（`ndt_resolution:=2.0` など）です。NDT の近傍探索は **DIRECT7**（`ndt_search_method:=1`）を推奨。

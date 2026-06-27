# robot_sim

ロボットの簡易シミュレータ（スタブ）パッケージ。

## 概要

最初の機能として、`cmd_vel`(geometry_msgs/Twist) を購読し、その速度指令を
時間積分してロボットの姿勢を更新し、`map` -> `base_link` 間の TF を発行する
`cmd_vel_to_tf` ノードを提供する。実機やGazebo等が無くても、プランナ等の
動作確認ができるようにするためのスタブ。

## ノード: lidar_sim

`/map`（OccupancyGrid）に対してレイキャストし、2D LiDAR の `sensor_msgs/LaserScan` を発行する。

### Subscribe
- `map` (nav_msgs/msg/OccupancyGrid): 地図

### Publish
- `scan` (sensor_msgs/msg/LaserScan): 2D LiDAR スキャン（パラメータで変更可）
- TF (static): `base_link` -> `laser`（`publish_laser_tf:=true` 時）

### 主なパラメータ
| 名前 | デフォルト | 説明 |
| --- | --- | --- |
| `num_beams` | 720 | ビーム数 |
| `range_max` | 30.0 | 最大測距 [m] |
| `publish_rate` | 10.0 | スキャン更新レート [Hz] |
| `laser_frame_id` | `laser` | スキャンの frame_id |

```bash
ros2 launch robot_sim lidar_sim.launch.xml
ros2 topic echo /scan --once
```

Foxglove では **Laser Scan** パネルで `/scan` を表示できる。

## ノード: cmd_vel_to_tf

### Subscribe
- `cmd_vel` (geometry_msgs/msg/Twist): 速度指令。`linear.x`, `linear.y`,
  `angular.z` を使用する。

### Publish
- TF: `map` -> `base_link`（パラメータで変更可能）
- `odom` (nav_msgs/msg/Odometry): 参考用の現在姿勢・速度

### パラメータ
| 名前 | 型 | デフォルト | 説明 |
| --- | --- | --- | --- |
| `map_frame_id` | string | `map` | 親フレームID |
| `base_frame_id` | string | `base_link` | 子フレームID |
| `publish_rate` | double | `50.0` | TF 発行レート [Hz] |
| `initial_x` | double | `0.0` | 初期位置 x [m] |
| `initial_y` | double | `0.0` | 初期位置 y [m] |
| `initial_yaw` | double | `0.0` | 初期ヨー角 [rad] |

## 運動モデル

2D の運動モデルで積分する（dt は更新周期）。

```
x   += (vx*cos(yaw) - vy*sin(yaw)) * dt
y   += (vx*sin(yaw) + vy*cos(yaw)) * dt
yaw += wz * dt
```

## 使い方

```bash
colcon build --packages-select robot_sim
source install/setup.bash
ros2 launch robot_sim robot_sim.py
```

別ターミナルで速度指令を送ると TF が動く。

```bash
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.5}, angular: {z: 0.2}}"
```

TF の確認:

```bash
ros2 run tf2_ros tf2_echo map base_link
```

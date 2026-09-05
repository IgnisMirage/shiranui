# build method

## ローカル (colcon)

```bash
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install
source install/setup.bash
ros2 launch autonomous_drive autonomous_drive.launch.xml
```

## Docker（自律走行シミュレーション）

Compose ファイルは `compose.yaml` を使用する（`docker-compose.yml` は使わない）。

```bash
# ビルド & 起動
docker compose build sim
docker compose up sim
```

Foxglove Studio で `ws://localhost:8765` に接続し、`/goal_pose` に目標位置を送る。

### 開発用コンテナ

ソースをマウントしてコンテナ内でビルド:

```bash
docker compose run --rm dev bash
# コンテナ内
colcon build --symlink-install
source install/setup.bash
ros2 launch autonomous_drive autonomous_drive.launch.xml
```

### プランナー切り替え

```bash
docker compose run --rm sim ros2 launch autonomous_drive autonomous_drive.launch.xml planner:=stanley
```

### 環境変数

| 変数 | デフォルト | 説明 |
|------|-----------|------|
| `ROS_DOMAIN_ID` | `0` | ROS 2 ドメイン ID |
| `RMW_IMPLEMENTATION` | `rmw_fastrtps_cpp` | RMW 実装 |

## Docker ターゲット

| Target | 用途 |
|--------|------|
| `deploy` | ビルド済み sim 起動（compose `sim` サービス） |
| `devenv` | 開発用シェル（compose `dev` サービス） |
| `build` | ビルドのみ |

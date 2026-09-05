# build method

## ビルドスクリプト

```bash
# ローカル (colcon)
./scripts/build.sh
./scripts/build.sh local --packages-select autonomous_drive

# Docker イメージ
./scripts/build.sh docker
```

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
# ローカルでイメージをビルドして起動
./scripts/build.sh docker
docker compose up sim

# CI でビルド済みイメージを pull して起動
docker compose pull sim
docker compose up sim
```

Foxglove Studio で `ws://localhost:8765` に接続し、`/goal_pose` に目標位置を送る。

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
| `devenv` | 開発用シェル |
| `build` | ビルドのみ |

## CI/CD

`main` ブランチへの push で GitHub Actions が Docker イメージをビルドし、GHCR に push する。

- Workflow: `.github/workflows/docker.yml`
- イメージ: `ghcr.io/ignismirage/tukuba2026:latest`
- タグ: `latest` と commit SHA

初回 pull 前に、GitHub の Package 設定でイメージを Public にするか、`docker login ghcr.io` が必要。

# build method

## 運用方針

**このリポジトリは基本的に Docker で動かす**（`compose.yaml` + GHCR イメージ、または `./scripts/build.sh docker`）。起動・実機確認・手順の説明は `docker compose pull` / `docker compose up` を前提にする。ローカル colcon はパッケージ開発時のビルド確認用。

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

## Docker（本番起動）

Compose ファイルは `compose.yaml` を使用する（`docker-compose.yml` は使わない）。
GHCR からイメージを pull して起動する（ローカルビルドなし）。

```bash
docker compose pull
docker compose up
```

Foxglove Studio で `ws://localhost:8765` に接続し、`/goal_pose` に目標位置を送る。

suke@raspberrypi:~ $ docker exec -it siranui-shiranui-1 bash -c 'sourse /opt/ros/jazzy/setup.bash && ros2 bag record -a -o /rosbag/$(date +%Y%m%d_%H%M%S) -s mcap'
bash: line 1: sourse: command not found
suke@raspberrypi:~ $ docker exec -it siranui-shiranui-1 bash -c 'sourse /opt/ros/jazzy/setup.bash && ros2 bag record -a -o /rosbag/$(date +%Y%m%d_%H%M%S) -s mcap'
### rosbag 録画

`compose.yaml` で **`./rosbag:/rosbag`** をマウントする。起動中の **同一コンテナ**（`docker ps` の NAMES、例: `siranui-shiranui-1`）で録画する（別 `docker run` だと点群などが bag に入らないことがある）。

**1. マウント付きで起動**（`compose.yaml` があるディレクトリで。既に起動済みなら `--force-recreate` で作り直す）

```bash
mkdir -p rosbag && docker compose up -d --force-recreate
```

**2. 録画**（ホストの `./rosbag/` に直接保存。`docker exec` は entrypoint を通らないので **source 必須**）

```bash
docker exec -it siranui-shiranui-1 bash -c 'source /opt/ros/jazzy/setup.bash && source /ros2_ws/install/setup.bash && ros2 bag record -a -o /rosbag/$(date +%Y%m%d_%H%M%S) -s mcap'
```

launch のオプション（foxglove / lidar_sim / safety_limiter 等）は `autonomous_drive.launch.xml` のデフォルト（true）を使用する。

### ローカルでイメージをビルドする場合（開発・CI 用）

```bash
./scripts/build.sh docker
```

### プランナー切り替え（必要なときだけ）

```bash
docker compose run --rm shiranui \
  ros2 launch autonomous_drive autonomous_drive.launch.xml planner:=stanley
```

### 環境変数

| 変数 | デフォルト | 説明 |
|------|-----------|------|
| `ROS_DOMAIN_ID` | `0` | ROS 2 ドメイン ID |
| `RMW_IMPLEMENTATION` | `rmw_fastrtps_cpp` | RMW 実装 |

## Docker ターゲット

| Target | 用途 |
|--------|------|
| `deploy` | 本番用イメージ（compose `shiranui` サービス） |
| `devenv` | 開発用シェル |
| `build` | ビルドのみ |

## CI/CD

`main` ブランチへの push で GitHub Actions が Docker イメージをビルドし、GHCR に push する。

- Workflow: `.github/workflows/docker.yml`
- イメージ: `ghcr.io/ignismirage/shiranui:latest`
- タグ: `latest` と commit SHA
- push 後、workflow が Package を Public 化する（失敗した場合は下記 B を使う）

### イメージの pull

**A. Public になっている場合（ログイン不要）**

```bash
docker pull ghcr.io/ignismirage/shiranui:latest
```

**B. `unauthorized` / `denied` が出る場合（Private のまま）**

Package が Private のときに出る。PAT でログインしてから pull する。

1. https://github.com/settings/tokens/new で PAT を作成（権限: `read:packages`）
2. ログインして pull:

```bash
echo <YOUR_GITHUB_PAT> | docker login ghcr.io -u IgnisMirage --password-stdin
docker pull ghcr.io/ignismirage/shiranui:latest
```

`-u` は GitHub ユーザー名（`IgnisMirage`）。Raspberry Pi 上でも同じ手順。

**C. そもそもイメージが無い場合**

https://github.com/IgnisMirage/shiranui/actions の Docker workflow が成功しているか確認する。
失敗している場合は `./scripts/build.sh docker` でローカルビルドする。

**D. Raspberry Pi (ARM64)**

CI は `linux/arm64` 向けにもビルドする。古いイメージしか無い場合は CI 再実行後に pull し直す。

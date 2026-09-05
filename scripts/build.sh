#!/usr/bin/env bash
set -eo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

IMAGE_NAME="${IMAGE_NAME:-ghcr.io/ignismirage/tukuba2026:latest}"
DOCKER_TARGET="${DOCKER_TARGET:-deploy}"
ROS_SETUP="${ROS_SETUP:-/opt/ros/jazzy/setup.bash}"

usage() {
  cat <<'EOF'
Usage: scripts/build.sh [local|docker] [options]

Commands:
  local   Build workspace with colcon (default)
  docker  Build deploy Docker image

Local options:
  Extra arguments are passed to colcon build.
  Default: colcon build --symlink-install

Docker options:
  --no-cache    Build without Docker cache
  --tag <name>  Image tag (default: ghcr.io/ignismirage/tukuba2026:latest)

Examples:
  scripts/build.sh
  scripts/build.sh local --packages-select autonomous_drive
  scripts/build.sh docker
  scripts/build.sh docker --tag tukuba2026:dev
EOF
}

cmd_local() {
  if [[ ! -f "$ROS_SETUP" ]]; then
    echo "error: ROS setup not found: $ROS_SETUP" >&2
    exit 1
  fi
  # shellcheck disable=SC1090
  source "$ROS_SETUP"

  if [[ $# -eq 0 ]]; then
    set -- --symlink-install
  fi

  colcon build "$@"
}

cmd_docker() {
  local extra_args=()

  while [[ $# -gt 0 ]]; do
    case "$1" in
      --no-cache)
        extra_args+=(--no-cache)
        shift
        ;;
      --tag)
        IMAGE_NAME="$2"
        shift 2
        ;;
      -h | --help)
        usage
        exit 0
        ;;
      *)
        echo "error: unknown docker option: $1" >&2
        usage
        exit 1
        ;;
    esac
  done

  docker build --target "$DOCKER_TARGET" -t "$IMAGE_NAME" "${extra_args[@]}" .
  echo "Built: $IMAGE_NAME"
}

main() {
  local cmd="${1:-local}"
  if [[ $# -gt 0 ]]; then
    shift
  fi

  case "$cmd" in
    local) cmd_local "$@" ;;
    docker) cmd_docker "$@" ;;
    -h | --help | help)
      usage
      ;;
    *)
      echo "error: unknown command: $cmd" >&2
      usage
      exit 1
      ;;
  esac
}

main "$@"

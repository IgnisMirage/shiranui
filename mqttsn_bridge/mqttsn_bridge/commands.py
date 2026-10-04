"""MQTT-SN で受けたペイロード (JSON) を検証してコマンドに変換する。ROS 非依存。

トピックとペイロード:
  path    {"id": "a1", "seq": 0, "n": 3, "poses": [[x, y, yaw], ...]}   (分割送信)
  pause   {}
  resume  {}
配信 (外部が購読する):
  pose    {"x": 1.23, "y": 4.56, "theta": 0.78}   map 座標系での自己位置 (theta は yaw [rad])
共有トークンを設定した場合は、全ペイロードに "token" が必要。
"""
import hmac
import json
import math
import time
from dataclasses import dataclass
from typing import Callable, Dict, List, Optional, Tuple

# トピック名 -> 事前定義トピック ID
TOPICS = {
    'path': 1,
    'pause': 2,
    'resume': 3,
}

# こちらから配信するトピック名 -> 事前定義トピック ID
PUBLISH_TOPICS = {
    'pose': 4,
}


class CommandError(Exception):
    pass


@dataclass
class Path:
    poses: List[Tuple[float, float, float]]


@dataclass
class Pause:
    pass


@dataclass
class Resume:
    pass


class CommandDecoder:
    def __init__(
        self,
        token: str = '',
        max_abs_coord: float = 1000.0,
        max_path_poses: int = 5000,
        max_path_chunks: int = 200,
        path_timeout: float = 5.0,
        path_min_interval: float = 0.2,
        clock: Callable[[], float] = time.monotonic,
    ):
        self._token = token
        self._max_abs = max_abs_coord
        self._max_poses = max_path_poses
        self._max_chunks = max_path_chunks
        self._path_timeout = path_timeout
        self._path_min_interval = path_min_interval
        self._clock = clock
        self._last_path_time: Optional[float] = None
        # 組み立て中の経路: id, 総チャンク数, 受信済みチャンク, 開始時刻
        self._path_id: Optional[str] = None
        self._path_n = 0
        self._path_chunks: Dict[int, list] = {}
        self._path_started = 0.0

    def decode(self, topic: str, payload: bytes):
        """受理したコマンドを返す。経路の途中 (未完成) は None。不正なら CommandError。"""
        try:
            data = json.loads(payload.decode('utf-8'))
        except (UnicodeDecodeError, ValueError) as e:
            raise CommandError(f'invalid JSON: {e}')
        if not isinstance(data, dict):
            raise CommandError('payload must be a JSON object')
        self._check_token(data)

        if topic == 'path':
            return self._decode_path_chunk(data)
        if topic == 'pause':
            return Pause()
        if topic == 'resume':
            return Resume()
        raise CommandError(f'unknown topic: {topic}')

    def _check_token(self, data: dict) -> None:
        if not self._token:
            return
        given = data.get('token')
        if not isinstance(given, str) or not hmac.compare_digest(given, self._token):
            raise CommandError('invalid token')

    def _number(self, value, name: str) -> float:
        # bool は int のサブクラスなので除外する
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise CommandError(f'{name} must be a number')
        value = float(value)
        if not math.isfinite(value):
            raise CommandError(f'{name} must be finite')
        return value

    def _coord(self, value, name: str) -> float:
        value = self._number(value, name)
        if abs(value) > self._max_abs:
            raise CommandError(f'{name} out of range (|{name}| <= {self._max_abs})')
        return value

    def _pose(self, x, y, yaw) -> Tuple[float, float, float]:
        return (self._coord(x, 'x'), self._coord(y, 'y'),
                self._number(yaw, 'yaw'))

    def _decode_path_chunk(self, data: dict) -> Optional[Path]:
        now = self._clock()
        path_id = data.get('id')
        seq, n, poses = data.get('seq'), data.get('n'), data.get('poses')
        if not isinstance(path_id, str) or not 0 < len(path_id) <= 32:
            raise CommandError('id must be a short string')
        if (not isinstance(seq, int) or isinstance(seq, bool) or
                not isinstance(n, int) or isinstance(n, bool)):
            raise CommandError('seq and n must be integers')
        if not 1 <= n <= self._max_chunks or not 0 <= seq < n:
            raise CommandError('invalid seq/n')
        if not isinstance(poses, list) or not poses:
            raise CommandError('poses must be a non-empty list')

        chunk = []
        for p in poses:
            if not isinstance(p, (list, tuple)) or len(p) != 3:
                raise CommandError('each pose must be [x, y, yaw]')
            chunk.append(self._pose(*p))

        # 新しい id、総数の変更、タイムアウトのいずれかで組み立てをやり直す
        if (path_id != self._path_id or n != self._path_n or
                now - self._path_started > self._path_timeout):
            self._path_id, self._path_n = path_id, n
            self._path_chunks = {}
            self._path_started = now
        self._path_chunks[seq] = chunk
        if sum(len(c) for c in self._path_chunks.values()) > self._max_poses:
            self._path_id, self._path_chunks = None, {}
            raise CommandError(f'path too long (> {self._max_poses} poses)')

        if len(self._path_chunks) < n:
            return None
        poses_all = [p for i in range(n) for p in self._path_chunks[i]]
        self._path_id = None
        self._path_chunks = {}
        if len(poses_all) > self._max_poses:
            raise CommandError(f'path too long (> {self._max_poses} poses)')
        if len(poses_all) < 2:
            raise CommandError('path needs at least 2 poses')
        if (self._last_path_time is not None and
                now - self._last_path_time < self._path_min_interval):
            raise CommandError('path rate limited')
        self._last_path_time = now
        return Path(poses_all)

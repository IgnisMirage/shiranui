import struct
import time
from typing import Callable, Dict, List, Optional

# メッセージ種別
SEARCHGW = 0x01
GWINFO = 0x02
CONNECT = 0x04
CONNACK = 0x05
REGISTER = 0x0A
REGACK = 0x0B
PUBLISH = 0x0C
PUBACK = 0x0D
SUBSCRIBE = 0x12
SUBACK = 0x13
PINGREQ = 0x16
PINGRESP = 0x17
DISCONNECT = 0x18

# リターンコード
RC_ACCEPTED = 0x00
RC_CONGESTION = 0x01
RC_INVALID_TOPIC_ID = 0x02
RC_NOT_SUPPORTED = 0x03

# PUBLISH フラグ中の TopicIdType
TOPIC_NORMAL = 0b00
TOPIC_PREDEFINED = 0b01
TOPIC_SHORT = 0b10

FLAG_WILL = 0x08
PROTOCOL_ID = 0x01


def build(msg_type: int, body: bytes = b'') -> bytes:
    """長さフィールド付きでパケットを組み立てる (255 バイトを超える場合は 3 バイト形式)。"""
    total = 2 + len(body)
    if total <= 255:
        return bytes([total, msg_type]) + body
    return b'\x01' + struct.pack('>H', 4 + len(body)) + bytes([msg_type]) + body


def parse(data: bytes):
    """(msg_type, body) を返す。壊れたパケットは None。"""
    if len(data) < 2:
        return None
    if data[0] == 0x01:
        if len(data) < 4:
            return None
        length = struct.unpack('>H', data[1:3])[0]
        msg_type, body = data[3], data[4:]
    else:
        length = data[0]
        msg_type, body = data[1], data[2:]
    if length != len(data):
        return None
    return msg_type, body


class _Session:
    def __init__(self, client_id: str, duration: int, now: float):
        self.client_id = client_id
        self.duration = duration
        self.last_seen = now
        self.topics: Dict[int, str] = {}  # このクライアントが REGISTER したトピック
        self.subs: Dict[str, int] = {}    # 購読中のトピック名 -> 配信時の TopicIdType


class Gateway:
    def __init__(
        self,
        topics: Dict[str, int],
        on_publish: Callable[[str, bytes, object], bool],
        gw_id: int = 1,
        clock: Callable[[], float] = time.monotonic,
        log: Callable[[str], None] = lambda msg: None,
        publish_topics: Optional[Dict[str, int]] = None,
        max_subscribers: int = 8,
    ):
        """topics: 受け付けるトピック名 -> 事前定義トピック ID。
        on_publish(topic_name, payload, addr) は受理したら True。
        publish_topics: 購読できる (こちらから配信する) トピック名 -> 事前定義トピック ID。"""
        self._ids = dict(topics)
        self._names = {tid: name for name, tid in topics.items()}
        self._out_ids = dict(publish_topics or {})
        self._out_names = {tid: name for name, tid in self._out_ids.items()}
        self._max_subscribers = max_subscribers
        self._on_publish = on_publish
        self._gw_id = gw_id
        self._clock = clock
        self._log = log
        self._sessions: Dict[object, _Session] = {}

    def handle(self, data: bytes, addr) -> List[bytes]:
        parsed = parse(data)
        if parsed is None:
            self._log(f'malformed packet from {addr}')
            return []
        msg_type, body = parsed
        handler = {
            SEARCHGW: self._on_searchgw,
            CONNECT: self._on_connect,
            REGISTER: self._on_register,
            PUBLISH: self._on_publish_packet,
            SUBSCRIBE: self._on_subscribe,
            PINGREQ: self._on_pingreq,
            DISCONNECT: self._on_disconnect,
        }.get(msg_type)
        if handler is None:
            self._log(f'unsupported msg type 0x{msg_type:02x} from {addr}')
            return []
        return handler(body, addr)

    def expire(self) -> None:
        """keep-alive の 1.5 倍を過ぎたセッションを破棄する。"""
        now = self._clock()
        for addr in [a for a, s in self._sessions.items()
                     if s.duration > 0 and now - s.last_seen > s.duration * 1.5]:
            self._log(f'session expired: {self._sessions[addr].client_id}')
            del self._sessions[addr]

    def _touch(self, addr) -> Optional[_Session]:
        session = self._sessions.get(addr)
        if session is not None:
            session.last_seen = self._clock()
        return session

    def _on_searchgw(self, body, addr):
        return [build(GWINFO, bytes([self._gw_id]))]

    def _on_connect(self, body, addr):
        if len(body) < 5:
            return []
        flags, protocol_id = body[0], body[1]
        duration = struct.unpack('>H', body[2:4])[0]
        client_id = body[4:].decode('utf-8', errors='replace')
        if protocol_id != PROTOCOL_ID or flags & FLAG_WILL or not 1 <= len(client_id) <= 23:
            return [build(CONNACK, bytes([RC_NOT_SUPPORTED]))]
        self._sessions[addr] = _Session(client_id, duration, self._clock())
        self._log(f'connected: {client_id} from {addr}')
        return [build(CONNACK, bytes([RC_ACCEPTED]))]

    def _on_register(self, body, addr):
        if len(body) < 5:
            return []
        msg_id = body[2:4]
        name = body[4:].decode('utf-8', errors='replace')
        session = self._touch(addr)
        if session is None:
            return [build(DISCONNECT)]
        topic_id = self._ids.get(name)
        if topic_id is None:
            return [build(REGACK, b'\x00\x00' + msg_id + bytes([RC_INVALID_TOPIC_ID]))]
        session.topics[topic_id] = name
        return [build(REGACK, struct.pack('>H', topic_id) + msg_id + bytes([RC_ACCEPTED]))]

    def _on_publish_packet(self, body, addr):
        if len(body) < 5:
            return []
        flags = body[0]
        topic_id_raw, msg_id = body[1:3], body[3:5]
        payload = body[5:]
        qos = (flags >> 5) & 0b11
        topic_type = flags & 0b11
        topic_id = struct.unpack('>H', topic_id_raw)[0]

        def ack(rc):
            # QoS 1 のときだけ PUBACK を返す
            if qos == 1:
                return [build(PUBACK, topic_id_raw + msg_id + bytes([rc]))]
            return []

        if qos == 2:
            self._log('QoS 2 is not supported')
            return []
        if qos != 0b11:  # QoS -1 以外は接続済みであること
            session = self._touch(addr)
            if session is None:
                return [build(DISCONNECT)]
        else:
            session = None

        if topic_type == TOPIC_PREDEFINED:
            name = self._names.get(topic_id)
        elif topic_type == TOPIC_NORMAL and session is not None:
            name = session.topics.get(topic_id)
        else:
            name = None
        if name is None:
            return ack(RC_INVALID_TOPIC_ID)

        accepted = self._on_publish(name, payload, addr)
        return ack(RC_ACCEPTED if accepted else RC_NOT_SUPPORTED)

    def _on_subscribe(self, body, addr):
        if len(body) < 4:
            return []
        flags, msg_id = body[0], body[1:3]
        topic_type = flags & 0b11

        def suback(topic_id, rc):
            # 配信は QoS 0 のみ許可する (Flags の QoS は 0)
            return [build(SUBACK, b'\x00' + struct.pack('>H', topic_id) + msg_id + bytes([rc]))]

        session = self._touch(addr)
        if session is None:
            return [build(DISCONNECT)]
        if topic_type == TOPIC_NORMAL:  # トピック名 (ワイルドカードは非対応)
            name = body[3:].decode('utf-8', errors='replace')
            topic_id = self._out_ids.get(name)
        elif topic_type == TOPIC_PREDEFINED and len(body) == 5:
            topic_id = struct.unpack('>H', body[3:5])[0]
            name = self._out_names.get(topic_id)
        else:
            return suback(0, RC_NOT_SUPPORTED)
        if name is None or topic_id is None or name not in self._out_ids:
            return suback(0, RC_INVALID_TOPIC_ID)
        subscribers = sum(1 for s in self._sessions.values() if s.subs)
        if not session.subs and subscribers >= self._max_subscribers:
            return suback(0, RC_CONGESTION)
        session.subs[name] = topic_type
        self._log(f'subscribed: {session.client_id} -> {name}')
        # 事前定義トピックの購読では TopicId を 0 で返さず、そのまま返す
        return suback(topic_id, RC_ACCEPTED)

    def has_subscribers(self, topic: str) -> bool:
        return any(topic in s.subs for s in self._sessions.values())

    def publish(self, topic: str, payload: bytes) -> List[tuple]:
        """topic を購読中の全クライアント宛の (addr, データグラム) を返す (QoS 0)。"""
        topic_id = self._out_ids.get(topic)
        if topic_id is None:
            return []
        out = []
        for addr, session in self._sessions.items():
            if topic in session.subs:
                flags = session.subs[topic] & 0b11  # QoS 0 | TopicIdType
                body = bytes([flags]) + struct.pack('>H', topic_id) + b'\x00\x00' + payload
                out.append((addr, build(PUBLISH, body)))
        return out

    def _on_pingreq(self, body, addr):
        if self._touch(addr) is None:
            return [build(DISCONNECT)]
        return [build(PINGRESP)]

    def _on_disconnect(self, body, addr):
        self._sessions.pop(addr, None)
        return [build(DISCONNECT)]

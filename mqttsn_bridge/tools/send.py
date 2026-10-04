#!/usr/bin/env python3
"""ブリッジへ path / pause / resume を MQTT-SN (QoS 1) で送る CLI。標準ライブラリのみ。

  ./tools/send.py --host 192.168.0.10 --token SECRET path 0,0,0 1,0,0 2,1,0.5
  ./tools/send.py --host 192.168.0.10 pose            # 自己位置 (x, y, theta) を購読して表示 (Ctrl-C で終了)
  ./tools/send.py --host 192.168.0.10 --token SECRET pause
経路は 1 データグラム約 1KB 以内に収まるよう自動で分割して送る。
"""
import argparse
import json
import socket
import struct
import sys
import time
import uuid

TOPIC_IDS = {'path': 1, 'pause': 2, 'resume': 3}
CONNECT, CONNACK, PUBLISH, PUBACK, SUBSCRIBE, SUBACK, PINGREQ, DISCONNECT = (
    0x04, 0x05, 0x0C, 0x0D, 0x12, 0x13, 0x16, 0x18)
POSES_PER_CHUNK = 30


def packet(msg_type: int, body: bytes = b'') -> bytes:
    total = 2 + len(body)
    if total <= 255:
        return bytes([total, msg_type]) + body
    return b'\x01' + struct.pack('>H', 4 + len(body)) + bytes([msg_type]) + body


class Client:
    def __init__(self, host: str, port: int, timeout: float):
        self.addr = (host, port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(timeout)
        self.msg_id = 0

    def _recv(self, expect: int) -> bytes:
        data, _ = self.sock.recvfrom(2048)
        if len(data) < 3 or data[1] != expect:
            raise SystemExit(f'unexpected reply: {data!r}')
        return data[2:]

    def connect(self):
        # Flags(CleanSession) ProtocolId Duration ClientId
        client_id = b'send-' + uuid.uuid4().hex[:8].encode()
        self.sock.sendto(packet(CONNECT, bytes([0x04, 0x01]) + struct.pack('>H', 30) + client_id),
                         self.addr)
        if self._recv(CONNACK)[0] != 0:
            raise SystemExit('connection rejected')

    def publish(self, topic: str, payload: dict):
        self.msg_id += 1
        # Flags: QoS 1 (0x20) | TopicIdType predefined (0x01)
        body = (bytes([0x21]) + struct.pack('>H', TOPIC_IDS[topic]) +
                struct.pack('>H', self.msg_id) + json.dumps(payload, separators=(',', ':')).encode())
        self.sock.sendto(packet(PUBLISH, body), self.addr)
        ack = self._recv(PUBACK)  # TopicId(2) MsgId(2) ReturnCode
        if ack[4] != 0:
            raise SystemExit(f'{topic} rejected by bridge (return code {ack[4]}); see bridge log')

    def watch(self, topic: str, count: int):
        """トピックを購読し、受信した JSON を 1 行ずつ表示する。"""
        self.msg_id += 1
        # Flags: TopicIdType 名前 (0b00)。ワイルドカードは使えない
        self.sock.sendto(packet(SUBSCRIBE, bytes([0x00]) + struct.pack('>H', self.msg_id) +
                                topic.encode()), self.addr)
        suback = self._recv(SUBACK)  # Flags TopicId(2) MsgId(2) ReturnCode
        if suback[5] != 0:
            raise SystemExit(f'subscribe rejected (return code {suback[5]})')
        received, last_ping = 0, time.monotonic()
        while count <= 0 or received < count:
            try:
                data, _ = self.sock.recvfrom(2048)
            except socket.timeout:
                data = b''
            if len(data) > 7 and data[1] == PUBLISH:  # Length Type Flags TopicId(2) MsgId(2) Data
                print(data[7:].decode(errors='replace'), flush=True)
                received += 1
            if time.monotonic() - last_ping > 15:  # keep-alive (30s) を維持
                self.sock.sendto(packet(PINGREQ), self.addr)
                last_ping = time.monotonic()

    def close(self):
        self.sock.sendto(packet(DISCONNECT), self.addr)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--host', default='127.0.0.1')
    ap.add_argument('--port', type=int, default=1884)
    ap.add_argument('--token', default='')
    ap.add_argument('--timeout', type=float, default=2.0)
    sub = ap.add_subparsers(dest='cmd', required=True)
    p = sub.add_parser('path')
    p.add_argument('poses', nargs='+', help='x,y,yaw (yaw 省略可)')
    w = sub.add_parser('pose', help='自己位置を購読して表示')
    w.add_argument('--count', type=int, default=0, help='この件数で終了 (0 で無限)')
    sub.add_parser('pause')
    sub.add_parser('resume')
    args = ap.parse_args()

    base = {'token': args.token} if args.token else {}
    client = Client(args.host, args.port, args.timeout)
    try:
        client.connect()
        if args.cmd == 'pose':
            client.watch('pose', args.count)
        elif args.cmd == 'path':
            poses = []
            for text in args.poses:
                v = [float(t) for t in text.split(',')]
                poses.append(v if len(v) == 3 else [v[0], v[1], 0.0])
            chunks = [poses[i:i + POSES_PER_CHUNK] for i in range(0, len(poses), POSES_PER_CHUNK)]
            path_id = uuid.uuid4().hex[:8]
            for seq, chunk in enumerate(chunks):
                client.publish('path', {**base, 'id': path_id, 'seq': seq,
                                              'n': len(chunks), 'poses': chunk})
        else:
            client.publish(args.cmd, base)
        client.close()
    except socket.timeout:
        sys.exit('no response from the bridge (check host/port/firewall)')
    print('ok')


if __name__ == '__main__':
    main()

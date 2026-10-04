import unittest

from mqttsn_bridge.protocol import Gateway

ADDR = ('192.0.2.1', 5000)
TOPICS = {'path': 1, 'pause': 2}


# 仕様書 (MQTT-SN v1.2) の形式でパケットを手組みする。protocol.build は使わない。
CONNECT = bytes([0x0A, 0x04, 0x04, 0x01, 0x00, 0x3C]) + b'cli1'          # CleanSession, 60s
CONNECT_WILL = bytes([0x0A, 0x04, 0x0C, 0x01, 0x00, 0x3C]) + b'cli1'
CONNACK_OK = bytes([0x03, 0x05, 0x00])


def register(name: bytes, msg_id=1):
    body = b'\x00\x00' + msg_id.to_bytes(2, 'big') + name
    return bytes([2 + len(body), 0x0A]) + body


def publish(flags, topic_id, payload, msg_id=7):
    body = bytes([flags]) + topic_id.to_bytes(2, 'big') + msg_id.to_bytes(2, 'big') + payload
    if 2 + len(body) <= 255:
        return bytes([2 + len(body), 0x0C]) + body
    return b'\x01' + (4 + len(body)).to_bytes(2, 'big') + b'\x0C' + body


class GatewayTest(unittest.TestCase):
    def setUp(self):
        self.received = []
        self.accept = True
        self.now = 0.0
        self.gw = Gateway(TOPICS, self._on_publish, clock=lambda: self.now,
                          publish_topics={'pose': 4}, max_subscribers=2)

    def _on_publish(self, name, payload, addr):
        self.received.append((name, payload))
        return self.accept

    def test_connect(self):
        self.assertEqual(self.gw.handle(CONNECT, ADDR), [CONNACK_OK])

    def test_connect_with_will_is_rejected(self):
        self.assertEqual(self.gw.handle(CONNECT_WILL, ADDR), [bytes([0x03, 0x05, 0x03])])

    def test_register_known_topic(self):
        self.gw.handle(CONNECT, ADDR)
        # REGACK: Length MsgType TopicId(2) MsgId(2) ReturnCode
        self.assertEqual(self.gw.handle(register(b'path', 9), ADDR),
                         [bytes([0x07, 0x0B, 0x00, 0x01, 0x00, 0x09, 0x00])])

    def test_register_unknown_topic_is_rejected(self):
        self.gw.handle(CONNECT, ADDR)
        self.assertEqual(self.gw.handle(register(b'other', 9), ADDR),
                         [bytes([0x07, 0x0B, 0x00, 0x00, 0x00, 0x09, 0x02])])

    def test_register_without_connect(self):
        self.assertEqual(self.gw.handle(register(b'path'), ADDR), [bytes([0x02, 0x18])])

    def test_publish_qos1_registered_topic(self):
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(register(b'path'), ADDR)
        # flags: QoS1 (0x20) + TopicIdType normal (0b00)
        reply = self.gw.handle(publish(0x20, 1, b'{"x":1}', msg_id=7), ADDR)
        self.assertEqual(self.received, [('path', b'{"x":1}')])
        self.assertEqual(reply, [bytes([0x07, 0x0D, 0x00, 0x01, 0x00, 0x07, 0x00])])

    def test_publish_qos0_has_no_reply(self):
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(register(b'path'), ADDR)
        self.assertEqual(self.gw.handle(publish(0x00, 1, b'{}'), ADDR), [])
        self.assertEqual(len(self.received), 1)

    def test_publish_predefined_topic_with_qos_minus1_needs_no_connect(self):
        # QoS -1 (0x60) + TopicIdType predefined (0b01)
        self.assertEqual(self.gw.handle(publish(0x61, 2, b'{}'), ADDR), [])
        self.assertEqual(self.received, [('pause', b'{}')])

    def test_publish_unregistered_topic_id(self):
        self.gw.handle(CONNECT, ADDR)
        reply = self.gw.handle(publish(0x20, 1, b'{}', msg_id=3), ADDR)
        self.assertEqual(reply, [bytes([0x07, 0x0D, 0x00, 0x01, 0x00, 0x03, 0x02])])
        self.assertEqual(self.received, [])

    def test_publish_without_connect_is_disconnected(self):
        self.assertEqual(self.gw.handle(publish(0x20, 1, b'{}'), ADDR), [bytes([0x02, 0x18])])
        self.assertEqual(self.received, [])

    def test_rejected_payload_returns_not_supported(self):
        self.accept = False
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(register(b'path'), ADDR)
        reply = self.gw.handle(publish(0x20, 1, b'x', msg_id=4), ADDR)
        self.assertEqual(reply, [bytes([0x07, 0x0D, 0x00, 0x01, 0x00, 0x04, 0x03])])

    def test_qos2_is_dropped(self):
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(register(b'path'), ADDR)
        self.assertEqual(self.gw.handle(publish(0x40, 1, b'{}'), ADDR), [])
        self.assertEqual(self.received, [])

    def test_long_payload_uses_three_byte_length(self):
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(register(b'pause'), ADDR)
        payload = b'a' * 400
        packet = publish(0x00, 2, payload)
        self.assertEqual(packet[0], 0x01)
        self.gw.handle(packet, ADDR)
        self.assertEqual(self.received, [('pause', payload)])

    def test_length_mismatch_is_dropped(self):
        self.gw.handle(CONNECT, ADDR)
        self.assertEqual(self.gw.handle(bytes([0x09, 0x16]), ADDR), [])  # PINGREQ with wrong length
        self.assertEqual(self.gw.handle(b'\x01', ADDR), [])
        self.assertEqual(self.gw.handle(b'', ADDR), [])

    def test_pingreq_and_disconnect(self):
        self.assertEqual(self.gw.handle(bytes([0x02, 0x16]), ADDR), [bytes([0x02, 0x18])])
        self.gw.handle(CONNECT, ADDR)
        self.assertEqual(self.gw.handle(bytes([0x02, 0x16]), ADDR), [bytes([0x02, 0x17])])
        self.assertEqual(self.gw.handle(bytes([0x02, 0x18]), ADDR), [bytes([0x02, 0x18])])
        self.assertEqual(self.gw.handle(bytes([0x02, 0x16]), ADDR), [bytes([0x02, 0x18])])

    # SUBSCRIBE: Flags MsgId(2) TopicName | TopicId   (手組み)
    @staticmethod
    def subscribe(msg_id, name=None, topic_id=None):
        if name is not None:
            body = bytes([0x00]) + msg_id.to_bytes(2, 'big') + name
        else:
            body = bytes([0x01]) + msg_id.to_bytes(2, 'big') + topic_id.to_bytes(2, 'big')
        return bytes([2 + len(body), 0x12]) + body

    def test_subscribe_by_name(self):
        self.gw.handle(CONNECT, ADDR)
        # SUBACK: Flags(QoS0) TopicId(2) MsgId(2) ReturnCode
        self.assertEqual(self.gw.handle(self.subscribe(5, name=b'pose'), ADDR),
                         [bytes([0x08, 0x13, 0x00, 0x00, 0x04, 0x00, 0x05, 0x00])])
        self.assertTrue(self.gw.has_subscribers('pose'))

    def test_subscribe_by_predefined_id(self):
        self.gw.handle(CONNECT, ADDR)
        self.assertEqual(self.gw.handle(self.subscribe(6, topic_id=4), ADDR),
                         [bytes([0x08, 0x13, 0x00, 0x00, 0x04, 0x00, 0x06, 0x00])])

    def test_subscribe_unknown_or_wildcard_topic(self):
        self.gw.handle(CONNECT, ADDR)
        for name in (b'ab', b'#', b'path'):  # path は受信専用で購読できない
            self.assertEqual(self.gw.handle(self.subscribe(5, name=name), ADDR),
                             [bytes([0x08, 0x13, 0x00, 0x00, 0x00, 0x00, 0x05, 0x02])])
        self.assertFalse(self.gw.has_subscribers('pose'))

    def test_subscribe_without_connect(self):
        self.assertEqual(self.gw.handle(self.subscribe(5, name=b'pose'), ADDR), [bytes([0x02, 0x18])])

    def test_subscriber_limit(self):
        for i in range(3):
            addr = ('192.0.2.1', 6000 + i)
            self.gw.handle(CONNECT, addr)
            reply = self.gw.handle(self.subscribe(1, name=b'pose'), addr)
            # 3 台目は rc=1 (輻輳)
            self.assertEqual(reply[0][-1], 0x01 if i == 2 else 0x00)

    def test_publish_to_subscribers_only(self):
        other = ('192.0.2.2', 5001)
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(self.subscribe(1, name=b'pose'), ADDR)
        self.gw.handle(CONNECT, other)  # 接続しただけで購読していない
        out = self.gw.publish('pose', b'{"x":1}')
        # PUBLISH: Flags(QoS0, TopicIdType normal) TopicId(2) MsgId(2)=0 Data
        self.assertEqual(out, [(ADDR, bytes([0x0E, 0x0C, 0x00, 0x00, 0x04, 0x00, 0x00]) + b'{"x":1}')])

    def test_publish_uses_predefined_topic_type_when_subscribed_that_way(self):
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(self.subscribe(1, topic_id=4), ADDR)
        packet = self.gw.publish('pose', b'x')[0][1]
        self.assertEqual(packet[2], 0x01)  # TopicIdType = predefined

    def test_no_publish_after_disconnect_or_expiry(self):
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(self.subscribe(1, name=b'pose'), ADDR)
        self.gw.handle(bytes([0x02, 0x18]), ADDR)
        self.assertEqual(self.gw.publish('pose', b'x'), [])
        self.gw.handle(CONNECT, ADDR)
        self.gw.handle(self.subscribe(1, name=b'pose'), ADDR)
        self.now = 100.0
        self.gw.expire()
        self.assertFalse(self.gw.has_subscribers('pose'))

    def test_publish_unknown_topic(self):
        self.assertEqual(self.gw.publish('nope', b'x'), [])

    def test_searchgw(self):
        self.assertEqual(self.gw.handle(bytes([0x03, 0x01, 0x00]), ADDR), [bytes([0x03, 0x02, 0x01])])

    def test_session_expires_after_keepalive(self):
        self.gw.handle(CONNECT, ADDR)          # duration 60s
        self.now = 91.0                         # 60 * 1.5 = 90s を超過
        self.gw.expire()
        self.assertEqual(self.gw.handle(bytes([0x02, 0x16]), ADDR), [bytes([0x02, 0x18])])


if __name__ == '__main__':
    unittest.main()

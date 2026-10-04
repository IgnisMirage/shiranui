"""MQTT-SN (UDP) で受けたコマンドを ROS 2 に流すノード。

環境変数:
  MQTTSN_BIND   待ち受けアドレス (既定 0.0.0.0)
  MQTTSN_PORT   UDP ポート (既定 1884)
  MQTTSN_TOKEN  共有トークン (必須。認証なしで動かすなら MQTTSN_ALLOW_NO_TOKEN=1)
  MQTTSN_FRAME  path の frame_id / 自己位置の基準フレーム (既定 map)
  MQTTSN_BASE_FRAME  自己位置を取るロボットのフレーム (既定 base_link)
  MQTTSN_POSE_HZ     pose を配信する周期 [Hz] (既定 5。購読者がいるときだけ配信)
"""
import json
import math
import os
import socket

import rclpy
from geometry_msgs.msg import PoseStamped
from nav_msgs.msg import Path as PathMsg
from rclpy.node import Node
from rclpy.time import Time
from rclpy.qos import QoSProfile, DurabilityPolicy, ReliabilityPolicy
from std_msgs.msg import Empty
from tf2_ros import Buffer, TransformException, TransformListener

from .commands import PUBLISH_TOPICS, TOPICS, CommandDecoder, CommandError, Path, Pause, Resume
from .protocol import Gateway

MAX_DATAGRAM = 2048
MAX_DATAGRAMS_PER_TICK = 50


def _yaw_to_quaternion(yaw: float):
    return math.sin(yaw / 2.0), math.cos(yaw / 2.0)


class MqttSnBridge(Node):
    def __init__(self):
        super().__init__('mqttsn_bridge')
        bind = os.environ.get('MQTTSN_BIND', '0.0.0.0')
        port = int(os.environ.get('MQTTSN_PORT', '1884'))
        self._frame = os.environ.get('MQTTSN_FRAME', 'map')
        token = os.environ.get('MQTTSN_TOKEN', '')
        if not token and os.environ.get('MQTTSN_ALLOW_NO_TOKEN') != '1':
            raise SystemExit(
                'MQTTSN_TOKEN is not set. Set a shared token, or set MQTTSN_ALLOW_NO_TOKEN=1 '
                'to run without authentication (trusted networks only).')

        self._decoder = CommandDecoder(token=token)
        self._gateway = Gateway(
            TOPICS, self._on_publish, publish_topics=PUBLISH_TOPICS,
            log=lambda msg: self.get_logger().info(msg))
        self._base_frame = os.environ.get('MQTTSN_BASE_FRAME', 'base_link')
        pose_hz = max(float(os.environ.get('MQTTSN_POSE_HZ', '5')), 0.1)
        self._tf_buffer = Buffer()
        self._tf_listener = TransformListener(self._tf_buffer, self)

        # pure_pursuit_planner の path 購読 (transient_local) に合わせる
        self._path_pub = self.create_publisher(
            PathMsg, 'path',
            QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL,
                       reliability=ReliabilityPolicy.RELIABLE))
        self._pause_pub = self.create_publisher(Empty, '/pause', 1)
        self._resume_pub = self.create_publisher(Empty, '/resume', 1)

        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.bind((bind, port))
        self._sock.setblocking(False)

        self.create_timer(0.01, self._poll)
        self.create_timer(1.0, self._gateway.expire)
        self.create_timer(1.0 / pose_hz, self._publish_pose)
        self.get_logger().info(
            f'MQTT-SN endpoint on udp://{bind}:{port} '
            f'(token {"required" if token else "disabled"})')

    def _poll(self):
        for _ in range(MAX_DATAGRAMS_PER_TICK):
            try:
                data, addr = self._sock.recvfrom(MAX_DATAGRAM)
            except BlockingIOError:
                return
            for reply in self._gateway.handle(data, addr):
                self._sendto(reply, addr)

    def _sendto(self, data: bytes, addr):
        try:
            self._sock.sendto(data, addr)
        except OSError as e:
            self.get_logger().warning(f'send to {addr} failed: {e}')

    def _publish_pose(self):
        # 購読者がいなければ TF を引かない
        if not self._gateway.has_subscribers('pose'):
            return
        try:
            t = self._tf_buffer.lookup_transform(self._frame, self._base_frame, Time())
        except TransformException as e:
            self.get_logger().warning(
                f'pose unavailable ({self._frame}->{self._base_frame}): {e}',
                throttle_duration_sec=5.0)
            return
        q = t.transform.rotation
        theta = math.atan2(2.0 * (q.w * q.z + q.x * q.y), 1.0 - 2.0 * (q.y * q.y + q.z * q.z))
        payload = json.dumps(
            {'x': round(t.transform.translation.x, 4),
             'y': round(t.transform.translation.y, 4),
             'theta': round(theta, 4)},
            separators=(',', ':')).encode()
        for addr, packet in self._gateway.publish('pose', payload):
            self._sendto(packet, addr)

    def _on_publish(self, topic: str, payload: bytes, addr) -> bool:
        try:
            command = self._decoder.decode(topic, payload)
        except CommandError as e:
            self.get_logger().warning(f'rejected {topic} from {addr}: {e}')
            return False
        if command is not None:
            self._dispatch(command)
        return True

    def _pose_stamped(self, x, y, yaw) -> PoseStamped:
        msg = PoseStamped()
        msg.header.frame_id = self._frame
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.pose.position.x = x
        msg.pose.position.y = y
        msg.pose.orientation.z, msg.pose.orientation.w = _yaw_to_quaternion(yaw)
        return msg

    def _dispatch(self, command):
        if isinstance(command, Path):
            msg = PathMsg()
            msg.header.frame_id = self._frame
            msg.header.stamp = self.get_clock().now().to_msg()
            msg.poses = [self._pose_stamped(*p) for p in command.poses]
            self._path_pub.publish(msg)
            self.get_logger().info(f'path -> {len(command.poses)} poses')
        elif isinstance(command, Pause):
            self._pause_pub.publish(Empty())
            self.get_logger().info('pause')
        elif isinstance(command, Resume):
            self._resume_pub.publish(Empty())
            self.get_logger().info('resume')


def main():
    rclpy.init()
    node = MqttSnBridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == '__main__':
    main()

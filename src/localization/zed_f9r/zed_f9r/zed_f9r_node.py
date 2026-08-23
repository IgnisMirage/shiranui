#!/usr/bin/env python3
"""u-blox ZED-F9R から UBX プロトコルで測位データを取得する ROS 2 ノード。

SparkFun の sparkfun-ublox-gps ライブラリを使用する。
https://github.com/sparkfun/Qwiic_Ublox_Gps_Py
"""

import math
import threading
import time

from geometry_msgs.msg import TwistWithCovarianceStamped
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu, NavSatFix, NavSatStatus
import serial
from ublox_gps import UbloxGps

# ---------------------------------------------------------------------------
# ライブラリのスケーリングの癖
#
# sparkfun-ublox-gps の scale_packet() は一部のフィールドにだけ係数を掛ける。
# そのため取得値の単位はフィールドごとに異なる（実測で確認済み）。
#
# NAV-PVT:
#   lat / lon              1e-7 適用済み  -> deg
#   height / hMSL          未適用         -> mm
#   hAcc / vAcc            0.1 適用済み   -> mm * 0.1  (HPPOSLLH 用の係数が誤適用される)
#   velN / velE / velD     未適用         -> mm/s
#   sAcc                   未適用         -> mm/s
# NAV-ATT:
#   roll / pitch / heading 1e-5 適用済み  -> deg
#   accPitch / accHeading  1e-5 適用済み  -> deg
#   accRoll                未適用         -> 1e-5 deg
#   iTOW / version         フィールド定義がずれているため値は信用できない（未使用）
# ---------------------------------------------------------------------------
MM_TO_M = 1e-3
PVT_ACC_TO_M = 1e-2          # mm*0.1 で来るので 0.01 倍で m
ATT_ACC_ROLL_TO_DEG = 1e-5   # accRoll だけ未変換

# ---------------------------------------------------------------------------
# CFG-VALSET の設定キー。この基板は Raspberry Pi の UART (/dev/serial0) にしか
# つながないので、受信機側の出力ポートは UART1 固定とする。
# 他ポートを使う場合のキーは CFG-MSGOUT-* が I2C→UART1→UART2→USB→SPI で +1、
# CFG-<port>OUTPROT-* が同じ順で +0x20000 の規則で並んでいる。
# ---------------------------------------------------------------------------
KEY_MSGOUT_UBX_NAV_PVT_UART1 = 0x20910007
KEY_MSGOUT_UBX_NAV_ATT_UART1 = 0x20910020
KEY_UART1OUTPROT_UBX = 0x10740001
KEY_UART1OUTPROT_NMEA = 0x10740002
KEY_RATE_MEAS = 0x30210001   # U2, 測位周期 [ms]
KEY_RATE_NAV = 0x30210002    # U2, 測位周期あたりの解算回数

CFG_LAYER_RAM = 1            # RAM のみ。フラッシュは書き換えない

# CFG-VALSET の値の長さはキーの bit28-30 (storage size) で決まる
_VALUE_SIZE = {0x1: 1, 0x2: 1, 0x3: 2, 0x4: 4, 0x5: 8}


def valset_value_bytes(key, value):
    """設定キーに対応する長さのリトルエンディアン値を返す。"""
    size = _VALUE_SIZE[(key >> 28) & 0x07]
    return int(value).to_bytes(size, 'little')


def euler_to_quaternion(roll, pitch, yaw):
    """ロール・ピッチ・ヨー [rad] を (x, y, z, w) のクォータニオンに変換する。"""
    cr, sr = math.cos(roll * 0.5), math.sin(roll * 0.5)
    cp, sp = math.cos(pitch * 0.5), math.sin(pitch * 0.5)
    cy, sy = math.cos(yaw * 0.5), math.sin(yaw * 0.5)
    return (
        sr * cp * cy - cr * sp * sy,
        cr * sp * cy + sr * cp * sy,
        cr * cp * sy - sr * sp * cy,
        cr * cp * cy + sr * sp * sy,
    )


FIX_TYPE_NAMES = {
    0: 'no fix',
    1: 'dead reckoning only',
    2: '2D',
    3: '3D',
    4: 'GNSS + dead reckoning',
    5: 'time only',
}


class ZedF9rNode(Node):
    """ZED-F9R から NAV-PVT / NAV-ATT を取得して配信するノード。"""

    def __init__(self):
        super().__init__('zed_f9r_node')

        # --- 接続 ---
        self.port = self.declare_parameter('port', '/dev/serial0').value
        self.baudrate = self.declare_parameter('baudrate', 38400).value
        self.serial_timeout = self.declare_parameter('serial_timeout', 1.0).value

        # --- 受信機設定 ---
        self.configure_receiver = self.declare_parameter('configure_receiver', True).value
        self.disable_nmea = self.declare_parameter('disable_nmea', True).value
        self.measurement_rate_ms = self.declare_parameter('measurement_rate_ms', 100).value

        # --- 出力 ---
        self.frame_id = self.declare_parameter('frame_id', 'gps_link').value
        self.imu_frame_id = self.declare_parameter('imu_frame_id', 'imu_link').value
        self.publish_imu = self.declare_parameter('publish_imu', True).value
        self.use_msl_altitude = self.declare_parameter('use_msl_altitude', False).value
        self.rate_hz = self.declare_parameter('rate_hz', 10.0).value
        self.wait_time_ms = self.declare_parameter('wait_time_ms', 1000).value

        fix_topic = self.declare_parameter('fix_topic', '/fix').value
        vel_topic = self.declare_parameter('vel_topic', '/fix_velocity').value
        imu_topic = self.declare_parameter('imu_topic', '/imu/data').value

        self.fix_pub = self.create_publisher(NavSatFix, fix_topic, 10)
        self.vel_pub = self.create_publisher(TwistWithCovarianceStamped, vel_topic, 10)
        self.imu_pub = self.create_publisher(Imu, imu_topic, 10)

        self.serial = serial.Serial(
            self.port, baudrate=self.baudrate, timeout=self.serial_timeout)
        # UbloxGps は生成時に受信スレッドを起動する
        self.gps = UbloxGps(self.serial)

        if self.configure_receiver:
            self._configure()

        self._running = True
        self._thread = threading.Thread(target=self._worker, daemon=True)
        self._thread.start()

        self.get_logger().info(
            f'zed_f9r_node started (port={self.port}, baudrate={self.baudrate})')

    # -- 受信機設定 -------------------------------------------------------
    def _configure(self):
        """必要な UBX メッセージを CFG-VALSET (RAM) で定期出力に設定する。"""
        if not 25 <= self.measurement_rate_ms <= 65535:
            self.get_logger().warn(
                f'measurement_rate_ms={self.measurement_rate_ms} is out of range, '
                'falling back to 100ms')
            self.measurement_rate_ms = 100

        items = [
            ('CFG-RATE-MEAS', KEY_RATE_MEAS, self.measurement_rate_ms),
            ('CFG-RATE-NAV', KEY_RATE_NAV, 1),
            ('CFG-UART1OUTPROT-UBX', KEY_UART1OUTPROT_UBX, 1),
            ('CFG-UART1OUTPROT-NMEA', KEY_UART1OUTPROT_NMEA,
             0 if self.disable_nmea else 1),
            ('CFG-MSGOUT-UBX_NAV_PVT_UART1', KEY_MSGOUT_UBX_NAV_PVT_UART1, 1),
            ('CFG-MSGOUT-UBX_NAV_ATT_UART1', KEY_MSGOUT_UBX_NAV_ATT_UART1,
             1 if self.publish_imu else 0),
        ]

        for name, key, value in items:
            try:
                # 受信機は CFG-VALSET に UBX-ACK-ACK を返すが、ライブラリの
                # ubx_set_val() は CFG-VALSET の応答を待つため必ずタイムアウトする。
                # wait_time=0 で待たずに送信し、ACK は _check_ack() でまとめて確認する。
                self.gps.ubx_set_val(
                    key, valset_value_bytes(key, value),
                    layer=CFG_LAYER_RAM, wait_time=0)
            except Exception as exc:  # noqa: BLE001 - ライブラリは任意の例外を投げる
                self.get_logger().error(f'Failed to send {name} (0x{key:08X}): {exc}')
                return

        if self._check_ack():
            self.get_logger().info(
                'Receiver configured: UART1, NAV-PVT=on, '
                f'NAV-ATT={"on" if self.publish_imu else "off"}, '
                f'NMEA={"off" if self.disable_nmea else "on"}, '
                f'rate={self.measurement_rate_ms}ms')
        else:
            self.get_logger().warn(
                'No UBX-ACK-ACK for CFG-VALSET. Continuing with the current '
                'receiver settings.')

        # 定期出力にしたメッセージはポーリング不要になるので、ライブラリを
        # auto モードに切り替えて「届いた最新パケットを待つ」動作にする。
        # set_auto_msg() が送る CFG-MSG は Gen 9 では非対応で NAK されるが、
        # ライブラリ内部の auto 登録は行われるため実害はない。
        self.gps.set_auto_msg('NAV', 'PVT', 1, wait_time=0)
        if self.publish_imu:
            self.gps.set_auto_msg('NAV', 'ATT', 1, wait_time=0)

    def _check_ack(self, timeout_s=1.5):
        """CFG-VALSET に対する ACK-ACK / ACK-NAK が届いたかを確認する。"""
        deadline = time.monotonic() + timeout_s
        while time.monotonic() < deadline:
            acks = self.gps.packets.get('ACK', {})
            if 'ACK' in acks:
                return True
            if 'NAK' in acks:
                self.get_logger().error('Receiver rejected the configuration (UBX-ACK-NAK)')
                return False
            time.sleep(0.05)
        return False

    # -- 受信ループ -------------------------------------------------------
    def _worker(self):
        period = 1.0 / self.rate_hz if self.rate_hz > 0 else 0.0
        while self._running:
            started = time.monotonic()
            try:
                coords = self.gps.geo_coords(wait_time=self.wait_time_ms)
                if coords is not None:
                    self._publish_fix(coords)
                elif self._running:
                    self.get_logger().warn(
                        'No NAV-PVT from the receiver', throttle_duration_sec=5.0)

                if self.publish_imu:
                    att = self.gps.veh_attitude(wait_time=self.wait_time_ms)
                    if att is not None:
                        self._publish_attitude(att)
            except Exception as exc:  # noqa: BLE001 - ライブラリは任意の例外を投げる
                if self._running:
                    self.get_logger().error(f'Failed to read from the receiver: {exc}')
                    time.sleep(0.5)
                continue

            remaining = period - (time.monotonic() - started)
            if remaining > 0:
                time.sleep(remaining)

    # -- 配信 -------------------------------------------------------------
    def _publish_fix(self, coords):
        """NAV-PVT から NavSatFix と TwistWithCovarianceStamped を配信する。"""
        stamp = self.get_clock().now().to_msg()
        flags = coords.flags

        fix = NavSatFix()
        fix.header.stamp = stamp
        fix.header.frame_id = self.frame_id

        if coords.fixType == 0 or not flags.gnssFixOK:
            fix.status.status = NavSatStatus.STATUS_NO_FIX
        elif flags.carrSoln != 0 or flags.diffSoln:
            # RTK Float/Fix と DGNSS は補強測位として GBAS 扱いにする
            fix.status.status = NavSatStatus.STATUS_GBAS_FIX
        else:
            fix.status.status = NavSatStatus.STATUS_FIX
        fix.status.service = (
            NavSatStatus.SERVICE_GPS | NavSatStatus.SERVICE_GLONASS |
            NavSatStatus.SERVICE_GALILEO | NavSatStatus.SERVICE_COMPASS)

        fix.latitude = float(coords.lat)
        fix.longitude = float(coords.lon)
        height_mm = coords.hMSL if self.use_msl_altitude else coords.height
        fix.altitude = float(height_mm) * MM_TO_M

        h_sigma = float(coords.hAcc) * PVT_ACC_TO_M
        v_sigma = float(coords.vAcc) * PVT_ACC_TO_M
        fix.position_covariance[0] = h_sigma ** 2
        fix.position_covariance[4] = h_sigma ** 2
        fix.position_covariance[8] = v_sigma ** 2
        fix.position_covariance_type = NavSatFix.COVARIANCE_TYPE_DIAGONAL_KNOWN

        # NED (受信機) -> ENU (ROS)
        vel = TwistWithCovarianceStamped()
        vel.header.stamp = stamp
        vel.header.frame_id = self.frame_id
        vel.twist.twist.linear.x = float(coords.velE) * MM_TO_M
        vel.twist.twist.linear.y = float(coords.velN) * MM_TO_M
        vel.twist.twist.linear.z = -float(coords.velD) * MM_TO_M
        s_sigma = float(coords.sAcc) * MM_TO_M
        vel.twist.covariance[0] = s_sigma ** 2
        vel.twist.covariance[7] = s_sigma ** 2
        vel.twist.covariance[14] = s_sigma ** 2
        vel.twist.covariance[21] = -1.0  # 角速度は NAV-PVT に含まれない

        self.fix_pub.publish(fix)
        self.vel_pub.publish(vel)

        self.get_logger().info(
            f'NAV-PVT: {FIX_TYPE_NAMES.get(coords.fixType, "unknown")}, '
            f'satellites={coords.numSV}, carrSoln={flags.carrSoln}, '
            f'hAcc={h_sigma:.2f}m',
            throttle_duration_sec=5.0)

    def _publish_attitude(self, att):
        """NAV-ATT から Imu（姿勢のみ）を配信する。"""
        roll = math.radians(att.roll)
        pitch = math.radians(att.pitch)
        # heading は北基準・時計回りなので、ENU のヨー角（東基準・反時計回り）に直す
        yaw = math.radians(90.0 - att.heading)

        acc_roll = math.radians(att.accRoll * ATT_ACC_ROLL_TO_DEG)
        acc_pitch = math.radians(att.accPitch)
        acc_heading = math.radians(att.accHeading)

        imu = Imu()
        imu.header.stamp = self.get_clock().now().to_msg()
        imu.header.frame_id = self.imu_frame_id
        (imu.orientation.x, imu.orientation.y,
         imu.orientation.z, imu.orientation.w) = euler_to_quaternion(roll, pitch, yaw)
        imu.orientation_covariance[0] = acc_roll ** 2
        imu.orientation_covariance[4] = acc_pitch ** 2
        imu.orientation_covariance[8] = acc_heading ** 2
        # 角速度・加速度は NAV-ATT に含まれない（UBX-ESF-RAW が必要）
        imu.angular_velocity_covariance[0] = -1.0
        imu.linear_acceleration_covariance[0] = -1.0

        self.imu_pub.publish(imu)

    # -- 終了処理 ---------------------------------------------------------
    def destroy_node(self):
        self._running = False
        thread = getattr(self, '_thread', None)
        if thread is not None and thread.is_alive():
            thread.join(timeout=2.0)
        gps = getattr(self, 'gps', None)
        if gps is not None:
            try:
                gps.stop()
            except Exception:  # noqa: BLE001
                pass
        port = getattr(self, 'serial', None)
        if port is not None and port.is_open:
            port.close()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = None
    try:
        node = ZedF9rNode()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if node is not None:
            node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()

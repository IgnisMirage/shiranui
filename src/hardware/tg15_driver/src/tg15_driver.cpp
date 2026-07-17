#include "tg15_driver/tg15_driver.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <asm/termbits.h>  // termios2 (BOTHER によるカスタムボーレート用)

#include <cmath>
#include <cstring>

using namespace std::chrono_literals;

namespace tg15_driver
{

TG15Driver::TG15Driver(const rclcpp::NodeOptions & options)
: Node("tg15_driver", options)
{
  port_ = this->declare_parameter("port", "/dev/ttyUSB0");
  baudrate_ = this->declare_parameter("baudrate", 512000);
  frame_id_ = this->declare_parameter("frame_id", "laser_frame");
  range_min_ = this->declare_parameter("range_min", 0.05);
  range_max_ = this->declare_parameter("range_max", 15.0);
  angle_min_deg_1_ = this->declare_parameter("angle_min_deg_1", 0.0);
  angle_max_deg_1_ = this->declare_parameter("angle_max_deg_1", 40.0);
  angle_min_deg_2_ = this->declare_parameter("angle_min_deg_2", 320.0);
  angle_max_deg_2_ = this->declare_parameter("angle_max_deg_2", 360.0);
  scan_frequency_ = this->declare_parameter("scan_frequency", 10.0);
  reversion_ = this->declare_parameter("reversion", true);
  inverted_ = this->declare_parameter("inverted", true);
  invalid_range_is_inf_ = this->declare_parameter("invalid_range_is_inf", false);

  scan_pub_ = this->create_publisher<sensor_msgs::msg::LaserScan>(
    "scan", rclcpp::SensorDataQoS());

  using std::placeholders::_1;
  using std::placeholders::_2;
  start_service_ = this->create_service<std_srvs::srv::Empty>(
    "start_scan", std::bind(&TG15Driver::on_start_scan, this, _1, _2));
  stop_service_ = this->create_service<std_srvs::srv::Empty>(
    "stop_scan", std::bind(&TG15Driver::on_stop_scan, this, _1, _2));

  if (!open_serial()) {
    RCLCPP_ERROR(this->get_logger(), "Failed to open serial port %s", port_.c_str());
    return;
  }

  // スキャンモード中は他コマンドを受け付けないため（マニュアル4章）、
  // 必ず停止させてから情報取得・周波数設定を行う
  send_command(CMD_STOP);
  std::this_thread::sleep_for(100ms);
  flush_input();

  query_device_info();
  query_health_status();
  adjust_scan_frequency();

  if (!start_scan()) {
    RCLCPP_ERROR(this->get_logger(), "Failed to start scan");
    return;
  }

  running_ = true;
  scan_thread_ = std::thread(&TG15Driver::scan_loop, this);
}

TG15Driver::~TG15Driver()
{
  running_ = false;
  if (scan_thread_.joinable()) {
    scan_thread_.join();
  }
  if (serial_fd_ >= 0) {
    stop_scan();
    close_serial();
  }
  RCLCPP_INFO(this->get_logger(), "TG15 driver shutdown");
}

// ---------------------------------------------------------------------------
// シリアル通信
// ---------------------------------------------------------------------------

bool TG15Driver::open_serial()
{
  serial_fd_ = open(port_.c_str(), O_RDWR | O_NOCTTY);
  if (serial_fd_ < 0) {
    return false;
  }
  if (!set_baudrate(baudrate_)) {
    close_serial();
    return false;
  }
  RCLCPP_INFO(this->get_logger(), "Serial port %s opened (baudrate %d)",
              port_.c_str(), baudrate_);
  return true;
}

bool TG15Driver::set_baudrate(int baudrate)
{
  // 512000bps は標準の B 定数に無いため termios2 + BOTHER を使う
  struct termios2 tio;
  if (ioctl(serial_fd_, TCGETS2, &tio) < 0) {
    return false;
  }
  tio.c_cflag &= ~CBAUD;
  tio.c_cflag |= BOTHER;
  tio.c_ispeed = baudrate;
  tio.c_ospeed = baudrate;

  // 8N1・rawモード
  tio.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
  tio.c_cflag |= CS8 | CREAD | CLOCAL;
  tio.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK |
                   ISTRIP | INLCR | IGNCR | ICRNL);
  tio.c_oflag &= ~OPOST;
  tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);
  tio.c_cc[VMIN] = 0;
  tio.c_cc[VTIME] = 1;  // 100ms

  if (ioctl(serial_fd_, TCSETS2, &tio) < 0) {
    return false;
  }
  return true;
}

void TG15Driver::close_serial()
{
  if (serial_fd_ >= 0) {
    close(serial_fd_);
    serial_fd_ = -1;
  }
}

bool TG15Driver::send_command(uint8_t cmd)
{
  uint8_t buf[2] = {CMD_START_BYTE, cmd};
  return write(serial_fd_, buf, sizeof(buf)) == static_cast<ssize_t>(sizeof(buf));
}

bool TG15Driver::read_exact(uint8_t * buf, size_t len, int timeout_ms)
{
  size_t total = 0;
  while (total < len) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(serial_fd_, &fds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int ret = select(serial_fd_ + 1, &fds, nullptr, nullptr, &tv);
    if (ret <= 0) {
      return false;  // タイムアウトまたはエラー
    }
    ssize_t n = read(serial_fd_, buf + total, len - total);
    if (n < 0) {
      return false;
    }
    total += static_cast<size_t>(n);
  }
  return true;
}

void TG15Driver::flush_input()
{
  ioctl(serial_fd_, TCFLSH, TCIFLUSH);
}

// ---------------------------------------------------------------------------
// プロトコル処理
// ---------------------------------------------------------------------------

// 応答メッセージヘッダ: A5 5A | Length(30bit) | Mode(2bit) | TypeCode(8bit) の7バイト
bool TG15Driver::wait_response_header(uint8_t expected_type, uint32_t & length,
                                      uint8_t & mode, int timeout_ms)
{
  uint8_t header[7];
  if (!read_exact(header, sizeof(header), timeout_ms)) {
    return false;
  }
  if (header[0] != 0xA5 || header[1] != 0x5A) {
    RCLCPP_WARN(this->get_logger(), "Invalid response start sign: %02X %02X",
                header[0], header[1]);
    return false;
  }
  // 6バイト目の下位6bitがレングス上位、上位2bitが応答モード
  length = static_cast<uint32_t>(header[2]) |
           (static_cast<uint32_t>(header[3]) << 8) |
           (static_cast<uint32_t>(header[4]) << 16) |
           (static_cast<uint32_t>(header[5] & 0x3F) << 24);
  mode = header[5] >> 6;
  uint8_t type_code = header[6];
  if (type_code != expected_type) {
    RCLCPP_WARN(this->get_logger(), "Unexpected type code: 0x%02X (expected 0x%02X)",
                type_code, expected_type);
    return false;
  }
  return true;
}

// デバイス情報取得 [A5 90]: モデル(1B) + FW(2B) + HW(1B) + シリアル(16B)
bool TG15Driver::query_device_info()
{
  flush_input();
  if (!send_command(CMD_DEVICE_INFO)) {
    return false;
  }
  uint32_t length = 0;
  uint8_t mode = 0;
  if (!wait_response_header(0x04, length, mode, 1000) || length != 20) {
    RCLCPP_WARN(this->get_logger(), "No device info response");
    return false;
  }
  uint8_t content[20];
  if (!read_exact(content, sizeof(content), 1000)) {
    return false;
  }
  uint8_t model = content[0];
  uint8_t fw_major = content[1];
  uint8_t fw_minor = content[2];
  uint8_t hw_version = content[3];
  char serial_str[33] = {0};
  for (int i = 0; i < 16; ++i) {
    snprintf(&serial_str[i * 2], 3, "%02X", content[4 + i]);
  }
  // モデル番号: TG15=100, TG30=101, TG50=102
  const char * model_name =
    (model == 100) ? "TG15" : (model == 101) ? "TG30" : (model == 102) ? "TG50" : "Unknown";
  RCLCPP_INFO(this->get_logger(),
              "Device: %s (model %d), FW %d.%d, HW %d, SN %s",
              model_name, model, fw_major, fw_minor, hw_version, serial_str);
  return true;
}

// ヘルスステータス取得 [A5 91]: ステータス(1B) + エラーコード(2B)
bool TG15Driver::query_health_status()
{
  flush_input();
  if (!send_command(CMD_HEALTH_STATUS)) {
    return false;
  }
  uint32_t length = 0;
  uint8_t mode = 0;
  if (!wait_response_header(0x06, length, mode, 1000) || length != 3) {
    RCLCPP_WARN(this->get_logger(), "No health status response");
    return false;
  }
  uint8_t content[3];
  if (!read_exact(content, sizeof(content), 1000)) {
    return false;
  }
  uint8_t status = content[0];
  uint16_t error_code = static_cast<uint16_t>(content[1]) |
                        (static_cast<uint16_t>(content[2]) << 8);
  if (status == 0) {
    RCLCPP_INFO(this->get_logger(), "Health status: OK");
  } else {
    RCLCPP_WARN(this->get_logger(), "Health status: %s (error code 0x%04X)",
                status == 1 ? "WARNING" : "ERROR", error_code);
  }
  return status != 2;
}

// 設定スキャン周波数取得 [A5 0D]: 4バイト応答 F = AnswerData / 100 [Hz]
bool TG15Driver::get_scan_frequency(double & freq_hz)
{
  flush_input();
  if (!send_command(CMD_FREQ_GET)) {
    return false;
  }
  uint32_t length = 0;
  uint8_t mode = 0;
  if (!wait_response_header(0x04, length, mode, 1000) || length != 4) {
    return false;
  }
  uint8_t content[4];
  if (!read_exact(content, sizeof(content), 1000)) {
    return false;
  }
  uint32_t raw = static_cast<uint32_t>(content[0]) |
                 (static_cast<uint32_t>(content[1]) << 8) |
                 (static_cast<uint32_t>(content[2]) << 16) |
                 (static_cast<uint32_t>(content[3]) << 24);
  freq_hz = raw / 100.0;
  return true;
}

// スキャン周波数を scan_frequency_ に合わせる [A5 09/0A/0B/0C]
bool TG15Driver::adjust_scan_frequency()
{
  double current = 0.0;
  if (!get_scan_frequency(current)) {
    RCLCPP_WARN(this->get_logger(), "Failed to get scan frequency");
    return false;
  }
  RCLCPP_INFO(this->get_logger(), "Current scan frequency: %.1f Hz (target %.1f Hz)",
              current, scan_frequency_);

  // 1Hz刻み → 0.1Hz刻みの順で目標値へ近づける
  auto step_command = [this](uint8_t cmd) {
    flush_input();
    if (!send_command(cmd)) {
      return false;
    }
    uint32_t length = 0;
    uint8_t mode = 0;
    if (!wait_response_header(0x04, length, mode, 1000) || length != 4) {
      return false;
    }
    uint8_t content[4];
    return read_exact(content, sizeof(content), 1000);
  };

  double diff = scan_frequency_ - current;
  int whole_steps = static_cast<int>(std::round(std::abs(diff)));
  for (int i = 0; i < whole_steps; ++i) {
    if (!step_command(diff > 0 ? CMD_FREQ_INC_1HZ : CMD_FREQ_DEC_1HZ)) {
      return false;
    }
  }
  if (!get_scan_frequency(current)) {
    return false;
  }
  diff = scan_frequency_ - current;
  int tenth_steps = static_cast<int>(std::round(std::abs(diff) * 10.0));
  for (int i = 0; i < tenth_steps; ++i) {
    if (!step_command(diff > 0 ? CMD_FREQ_INC_01HZ : CMD_FREQ_DEC_01HZ)) {
      return false;
    }
  }
  if (get_scan_frequency(current)) {
    RCLCPP_INFO(this->get_logger(), "Scan frequency set to %.1f Hz", current);
  }
  return true;
}

// スキャン開始 [A5 60]: 応答ヘッダ A5 5A 05 00 00 40 81 の後、点群パケットが連続する
bool TG15Driver::start_scan()
{
  flush_input();
  if (!send_command(CMD_SCAN)) {
    return false;
  }
  uint32_t length = 0;
  uint8_t mode = 0;
  if (!wait_response_header(0x81, length, mode, 2000)) {
    return false;
  }
  if (mode != 0x1) {
    RCLCPP_WARN(this->get_logger(), "Scan response mode is not continuous: %d", mode);
    return false;
  }
  revolution_points_.clear();
  revolution_start_time_ = this->now();
  scanning_ = true;
  RCLCPP_INFO(this->get_logger(), "Scan started");
  return true;
}

void TG15Driver::stop_scan()
{
  scanning_ = false;
  send_command(CMD_STOP);
  std::this_thread::sleep_for(100ms);
  flush_input();
}

// ---------------------------------------------------------------------------
// スキャンデータ処理
// ---------------------------------------------------------------------------

void TG15Driver::scan_loop()
{
  while (running_) {
    if (!scanning_) {
      std::this_thread::sleep_for(100ms);
      continue;
    }
    if (!parse_packet()) {
      // 同期喪失などは parse_packet 内で処理するため、ここでは軽く待つのみ
      std::this_thread::sleep_for(1ms);
    }
  }
}

// 点群パケット1個を読み取って解釈する
// PH(2B)=0x55AA | CT(1B) | LSN(1B) | FSA(2B) | LSA(2B) | CS(2B) | S1..Sn(2B each)
bool TG15Driver::parse_packet()
{
  // ヘッダ同期: 0xAA 0x55 (リトルエンディアンで下位が先)
  uint8_t b = 0;
  if (!read_exact(&b, 1, 500)) {
    return false;
  }
  if (b != 0xAA) {
    return false;
  }
  if (!read_exact(&b, 1, 500) || b != 0x55) {
    return false;
  }

  uint8_t header[8];  // CT, LSN, FSA(2), LSA(2), CS(2)
  if (!read_exact(header, sizeof(header), 500)) {
    return false;
  }
  uint8_t ct = header[0];
  uint8_t lsn = header[1];
  uint16_t fsa_raw = static_cast<uint16_t>(header[2]) | (static_cast<uint16_t>(header[3]) << 8);
  uint16_t lsa_raw = static_cast<uint16_t>(header[4]) | (static_cast<uint16_t>(header[5]) << 8);
  uint16_t cs = static_cast<uint16_t>(header[6]) | (static_cast<uint16_t>(header[7]) << 8);

  if (lsn == 0) {
    return false;
  }

  std::vector<uint8_t> samples(static_cast<size_t>(lsn) * 2);
  if (!read_exact(samples.data(), samples.size(), 500)) {
    return false;
  }

  // チェックサム: CS を除く16bitワードのXOR（PH, FSA, S1..Sn, CT|LSN<<8, LSA）
  uint16_t xor_sum = 0x55AA;
  xor_sum ^= fsa_raw;
  for (size_t i = 0; i < lsn; ++i) {
    xor_sum ^= static_cast<uint16_t>(samples[i * 2]) |
               (static_cast<uint16_t>(samples[i * 2 + 1]) << 8);
  }
  xor_sum ^= static_cast<uint16_t>(ct) | (static_cast<uint16_t>(lsn) << 8);
  xor_sum ^= lsa_raw;
  if (xor_sum != cs) {
    RCLCPP_DEBUG(this->get_logger(), "Checksum mismatch: calc 0x%04X != recv 0x%04X",
                 xor_sum, cs);
    return false;
  }

  bool is_start_packet = (ct & 0x01) != 0;

  if (is_start_packet) {
    // ゼロパケット: 1回転の先頭。CT上位7bitに回転周波数情報が入る
    // Freq = (((CT & 0xFE) >> 1) + 30) / 10 [Hz]
    current_frequency_ = (((ct & 0xFE) >> 1) + 30) / 10.0;

    // 前の回転分を発行
    if (!revolution_points_.empty()) {
      publish_scan(revolution_start_time_);
      revolution_points_.clear();
    }
    revolution_start_time_ = this->now();
  }

  // 角度: Angle = Rshiftbit(raw, 1) / 64 [deg]
  double fsa = (fsa_raw >> 1) / 64.0;
  double lsa = (lsa_raw >> 1) / 64.0;
  double diff = lsa - fsa;
  if (diff < 0) {
    diff += 360.0;  // 時計回りの角度差
  }

  for (size_t i = 0; i < lsn; ++i) {
    double distance_mm =
      static_cast<double>(static_cast<uint16_t>(samples[i * 2]) |
                          (static_cast<uint16_t>(samples[i * 2 + 1]) << 8));
    double angle_deg = (lsn > 1) ? fsa + diff * static_cast<double>(i) / (lsn - 1) : fsa;
    if (angle_deg >= 360.0) {
      angle_deg -= 360.0;
    }
    revolution_points_.push_back({angle_deg, distance_mm});
  }
  return true;
}

void TG15Driver::publish_scan(const rclcpp::Time & stamp)
{
  const size_t n = revolution_points_.size();
  if (n < 2) {
    return;
  }

  auto msg = std::make_unique<sensor_msgs::msg::LaserScan>();
  msg->header.stamp = stamp;
  msg->header.frame_id = frame_id_;
  msg->angle_min = -M_PI;
  msg->angle_max = M_PI;
  msg->angle_increment = 2.0 * M_PI / static_cast<double>(n);
  msg->range_min = range_min_;
  msg->range_max = range_max_;

  double freq = (current_frequency_ > 0.0) ? current_frequency_ : scan_frequency_;
  msg->scan_time = 1.0 / freq;
  msg->time_increment = msg->scan_time / static_cast<double>(n);

  const float invalid = invalid_range_is_inf_
    ? std::numeric_limits<float>::infinity() : 0.0f;
  msg->ranges.assign(n, invalid);

  for (const auto & p : revolution_points_) {
    double angle_deg = p.angle_deg;

    // 使用角度範囲フィルタ（2つの独立した範囲を対応）
    bool in_range1 = (angle_deg >= angle_min_deg_1_ && angle_deg <= angle_max_deg_1_);
    bool in_range2 = (angle_deg >= angle_min_deg_2_ && angle_deg <= angle_max_deg_2_);
    if (!in_range1 && !in_range2) {
      continue;  // 両範囲外なら無視
    }

    if (reversion_) {
      angle_deg += 180.0;
    }
    double angle_rad = angle_deg * M_PI / 180.0;
    if (inverted_) {
      angle_rad = -angle_rad;  // ライダーは時計回り、ROSは反時計回り正
    }
    // [-π, π) に正規化
    angle_rad = std::fmod(angle_rad + M_PI, 2.0 * M_PI);
    if (angle_rad < 0) {
      angle_rad += 2.0 * M_PI;
    }
    angle_rad -= M_PI;

    double range_m = p.distance_mm / 1000.0;
    if (range_m < range_min_ || range_m > range_max_) {
      continue;  // 距離0（無効点）や範囲外はスキップ
    }
    int index = static_cast<int>((angle_rad - msg->angle_min) / msg->angle_increment);
    if (index >= 0 && index < static_cast<int>(n)) {
      msg->ranges[index] = static_cast<float>(range_m);
    }
  }

  scan_pub_->publish(std::move(msg));
}

// ---------------------------------------------------------------------------
// サービスコールバック
// ---------------------------------------------------------------------------

void TG15Driver::on_start_scan(const std::shared_ptr<std_srvs::srv::Empty::Request>,
                               std::shared_ptr<std_srvs::srv::Empty::Response>)
{
  if (!scanning_) {
    start_scan();
  }
}

void TG15Driver::on_stop_scan(const std::shared_ptr<std_srvs::srv::Empty::Request>,
                              std::shared_ptr<std_srvs::srv::Empty::Response>)
{
  if (scanning_) {
    stop_scan();
    RCLCPP_INFO(this->get_logger(), "Scan stopped");
  }
}

}  // namespace tg15_driver

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(tg15_driver::TG15Driver)

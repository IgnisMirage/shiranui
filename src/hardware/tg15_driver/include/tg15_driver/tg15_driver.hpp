#ifndef TG15_DRIVER__TG15_DRIVER_HPP_
#define TG15_DRIVER__TG15_DRIVER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <std_srvs/srv/empty.hpp>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace tg15_driver
{

// YDLIDAR TG シリーズ システムコマンド (TG Series Development Manual 2.2)
enum TGCommand : uint8_t {
  CMD_START_BYTE       = 0xA5,
  CMD_SCAN             = 0x60,  // スキャン開始（連続応答）
  CMD_STOP             = 0x65,  // スキャン停止（応答なし）
  CMD_DEVICE_INFO      = 0x90,  // デバイス情報取得
  CMD_HEALTH_STATUS    = 0x91,  // ヘルスステータス取得
  CMD_FREQ_INC_01HZ    = 0x09,  // スキャン周波数 +0.1Hz
  CMD_FREQ_DEC_01HZ    = 0x0A,  // スキャン周波数 -0.1Hz
  CMD_FREQ_INC_1HZ     = 0x0B,  // スキャン周波数 +1Hz
  CMD_FREQ_DEC_1HZ     = 0x0C,  // スキャン周波数 -1Hz
  CMD_FREQ_GET         = 0x0D,  // 設定スキャン周波数取得
  CMD_POWER_PROTECT    = 0xD9,  // 電源断保護モード切替
  CMD_RESTART          = 0x80,  // ソフトリスタート（応答なし）
  CMD_ZERO_ANGLE       = 0x93   // ゼロ角度オフセット取得
};

class TG15Driver : public rclcpp::Node
{
public:
  explicit TG15Driver(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  virtual ~TG15Driver();

private:
  // シリアルポート
  int serial_fd_ = -1;
  std::atomic<bool> running_ = false;
  std::thread scan_thread_;

  // パラメータ
  std::string port_;            // シリアルポートデバイス
  int baudrate_;                // ボーレート（TG15は512000）
  std::string frame_id_;        // スキャンのフレームID
  double range_min_;            // 最小距離 [m]
  double range_max_;            // 最大距離 [m]
  double angle_min_deg_1_;      // 使用角度範囲1の下限 [deg]
  double angle_max_deg_1_;      // 使用角度範囲1の上限 [deg]
  double angle_min_deg_2_;      // 使用角度範囲2の下限 [deg]
  double angle_max_deg_2_;      // 使用角度範囲2の上限 [deg]
  double scan_frequency_;       // 目標スキャン周波数 [Hz]
  bool reversion_;              // 180度回転補正
  bool inverted_;               // 回転方向反転（CCW正のROS規約に合わせる）
  bool invalid_range_is_inf_;   // 無効点をinfにするか（falseなら0.0）

  rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr scan_pub_;
  rclcpp::Service<std_srvs::srv::Empty>::SharedPtr start_service_;
  rclcpp::Service<std_srvs::srv::Empty>::SharedPtr stop_service_;

  // 1回転分の点群バッファ
  struct ScanPoint {
    double angle_deg;     // ライダー座標系角度 [deg]
    double distance_mm;   // 距離 [mm]
  };
  std::vector<ScanPoint> revolution_points_;
  rclcpp::Time revolution_start_time_;
  double current_frequency_ = 0.0;  // ゼロパケットのCTから得た回転周波数 [Hz]
  std::atomic<bool> scanning_ = false;

  // シリアル通信
  bool open_serial();
  void close_serial();
  bool set_baudrate(int baudrate);
  bool send_command(uint8_t cmd);
  bool read_exact(uint8_t * buf, size_t len, int timeout_ms);
  void flush_input();

  // プロトコル処理
  bool start_scan();
  void stop_scan();
  bool wait_response_header(uint8_t expected_type, uint32_t & length, uint8_t & mode,
                            int timeout_ms);
  bool query_device_info();
  bool query_health_status();
  bool adjust_scan_frequency();
  bool get_scan_frequency(double & freq_hz);

  // スキャンデータ処理
  void scan_loop();
  bool parse_packet();
  void publish_scan(const rclcpp::Time & stamp);

  // サービスコールバック
  void on_start_scan(const std::shared_ptr<std_srvs::srv::Empty::Request> req,
                     std::shared_ptr<std_srvs::srv::Empty::Response> res);
  void on_stop_scan(const std::shared_ptr<std_srvs::srv::Empty::Request> req,
                    std::shared_ptr<std_srvs::srv::Empty::Response> res);
};

}  // namespace tg15_driver

#endif  // TG15_DRIVER__TG15_DRIVER_HPP_

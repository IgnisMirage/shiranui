#ifndef MID360_DRIVER_HPP
#define MID360_DRIVER_HPP

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include "mid360/livox_protocol.hpp"
#include "mid360/packet_codec.hpp"
#include "mid360_driver/msg/mid360_packet.hpp"

namespace mid360
{

class Mid360Driver : public rclcpp::Node
{
public:
  explicit Mid360Driver(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
  ~Mid360Driver() override;

  bool start();
  void stop();

private:
  bool createSocket(int& sock, uint16_t port);
  void closeSocket(int& sock);
  bool sendCommand(const std::string& lidar_ip, uint16_t lidar_port, const std::vector<uint8_t>& payload);
  void sendDiscoveryBroadcast();
  void sendLidarConfiguration(const std::string& lidar_ip, uint16_t lidar_cmd_port);
  void handleCommandPacket(const uint8_t* buffer, size_t size, uint16_t source_port);
  void handleDataPacket(const uint8_t* buffer, size_t size);
  void handlePointCloudPacket(const LivoxLidarEthernetPacket* packet);
  void handleImuPacket(const LivoxLidarEthernetPacket* packet);
  void ioLoop();
  void detectionLoop();
  void publishPointCloud();
  rclcpp::Time packetTimestamp(const LivoxLidarEthernetPacket* packet) const;

  std::string host_ip_;
  std::string lidar_ip_;
  std::string frame_id_;
  uint16_t push_msg_port_;
  uint16_t point_data_port_;
  uint16_t imu_data_port_;
  double publish_rate_;

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pointcloud_pub_;
  rclcpp::Publisher<mid360_driver::msg::Mid360Packet>::SharedPtr packet_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_pub_;
  rclcpp::TimerBase::SharedPtr publish_timer_;

  int detection_socket_;
  int point_socket_;
  int imu_socket_;
  int cmd_socket_;
  int push_socket_;

  std::thread io_thread_;
  std::thread detection_thread_;
  std::atomic<bool> running_;
  std::atomic<bool> configured_;
  std::atomic<uint32_t> seq_num_;

  std::mutex packets_mutex_;
  std::vector<RawPacketSnapshot> accumulated_packets_;
};

}  // namespace mid360

#endif  // MID360_DRIVER_HPP

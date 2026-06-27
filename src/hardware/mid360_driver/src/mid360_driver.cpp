#include "mid360/mid360_driver.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstring>

#include "mid360/sdk_protocol.hpp"

namespace mid360
{

Mid360Driver::Mid360Driver(const rclcpp::NodeOptions& options)
    : Node("mid360_driver", options),
      detection_socket_(-1),
      point_socket_(-1),
      imu_socket_(-1),
      cmd_socket_(-1),
      push_socket_(-1),
      running_(false),
      configured_(false),
      seq_num_(1)
{
  host_ip_ = declare_parameter<std::string>("host_ip", "192.168.1.50");
  lidar_ip_ = declare_parameter<std::string>("lidar_ip", "192.168.1.181");
  frame_id_ = declare_parameter<std::string>("frame_id", "mid360");
  int push_msg_port = kMid360HostPushMsgPort;
  int point_data_port = kMid360HostPointCloudPort;
  int imu_data_port = kMid360HostImuDataPort;
  push_msg_port = declare_parameter<int>("push_msg_port", push_msg_port);
  point_data_port = declare_parameter<int>("point_data_port", point_data_port);
  imu_data_port = declare_parameter<int>("imu_data_port", imu_data_port);
  push_msg_port_ = static_cast<uint16_t>(push_msg_port);
  point_data_port_ = static_cast<uint16_t>(point_data_port);
  imu_data_port_ = static_cast<uint16_t>(imu_data_port);
  publish_rate_ = declare_parameter<double>("publish_rate", 10.0);

  if (publish_rate_ > 100.0) {
    publish_rate_ = 100.0;
  } else if (publish_rate_ < 0.5) {
    publish_rate_ = 0.5;
  }
}

Mid360Driver::~Mid360Driver() { stop(); }

bool Mid360Driver::createSocket(int& sock, uint16_t port)
{
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    RCLCPP_ERROR(get_logger(), "failed to create UDP socket on port %u", port);
    return false;
  }

  int reuse = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (inet_pton(AF_INET, host_ip_.c_str(), &addr.sin_addr) != 1) {
    RCLCPP_ERROR(get_logger(), "invalid host_ip '%s'", host_ip_.c_str());
    close(sock);
    sock = -1;
    return false;
  }

  if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    RCLCPP_ERROR(get_logger(), "failed to bind UDP socket on %s:%u: %s (assign host_ip to a LiDAR-subnet NIC)",
                 host_ip_.c_str(), port, std::strerror(errno));
    close(sock);
    sock = -1;
    return false;
  }

  return true;
}

void Mid360Driver::closeSocket(int& sock)
{
  if (sock >= 0) {
    close(sock);
    sock = -1;
  }
}

bool Mid360Driver::sendCommand(const std::string& target_ip, uint16_t target_port,
                               const std::vector<uint8_t>& payload)
{
  if (detection_socket_ < 0) {
    return false;
  }

  sockaddr_in dest{};
  dest.sin_family = AF_INET;
  dest.sin_port = htons(target_port);
  if (inet_pton(AF_INET, target_ip.c_str(), &dest.sin_addr) != 1) {
    return false;
  }

  const ssize_t sent = sendto(detection_socket_, payload.data(), payload.size(), 0,
                              reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
  return sent == static_cast<ssize_t>(payload.size());
}

void Mid360Driver::sendDiscoveryBroadcast()
{
  SdkProtocol protocol;
  CommPacket packet{};
  packet.seq_num = seq_num_++;
  packet.cmd_id = kCommandIDLidarSearch;
  packet.cmd_type = kCommandTypeCmd;
  packet.sender_type = kHostSend;
  packet.data = nullptr;
  packet.data_len = 0;

  std::vector<uint8_t> buffer(kMaxCommandBufferSize, 0);
  uint32_t length = 0;
  if (!protocol.pack(buffer.data(), static_cast<uint32_t>(buffer.size()), &length, packet)) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "failed to pack discovery command");
    return;
  }

  sockaddr_in dest{};
  dest.sin_family = AF_INET;
  dest.sin_port = htons(kDetectionPort);
  dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  int broadcast = 1;
  setsockopt(detection_socket_, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

  sendto(detection_socket_, buffer.data(), length, 0, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
}

void Mid360Driver::sendLidarConfiguration(const std::string& target_lidar_ip, uint16_t lidar_cmd_port)
{
  std::vector<uint8_t> request;
  if (!buildMid360ConfigRequest(host_ip_, push_msg_port_, point_data_port_, imu_data_port_, request)) {
    RCLCPP_ERROR(get_logger(), "failed to build configuration request");
    return;
  }

  SdkProtocol protocol;
  CommPacket packet{};
  packet.seq_num = seq_num_++;
  packet.cmd_id = kCommandIDLidarWorkModeControl;
  packet.cmd_type = kCommandTypeCmd;
  packet.sender_type = kHostSend;
  packet.data = request.data();
  packet.data_len = static_cast<uint16_t>(request.size());

  std::vector<uint8_t> buffer(kMaxCommandBufferSize, 0);
  uint32_t length = 0;
  if (!protocol.pack(buffer.data(), static_cast<uint32_t>(buffer.size()), &length, packet)) {
    RCLCPP_ERROR(get_logger(), "failed to pack configuration command");
    return;
  }

  if (sendCommand(target_lidar_ip, lidar_cmd_port,
                  std::vector<uint8_t>(buffer.begin(), buffer.begin() + length))) {
    RCLCPP_INFO(get_logger(), "sent configuration to %s:%u", target_lidar_ip.c_str(), lidar_cmd_port);
  } else {
    RCLCPP_WARN(get_logger(), "failed to send configuration to %s:%u", target_lidar_ip.c_str(), lidar_cmd_port);
  }
}

void Mid360Driver::handleCommandPacket(const uint8_t* buffer, size_t size, uint16_t source_port)
{
  SdkProtocol protocol;
  CommPacket packet{};
  if (!protocol.parse(const_cast<uint8_t*>(buffer), static_cast<uint32_t>(size), &packet)) {
    return;
  }

  if (packet.cmd_id == kCommandIDLidarSearch && packet.cmd_type == kCommandTypeAck &&
      packet.data_len >= sizeof(DetectionData)) {
    const auto* detection = reinterpret_cast<const DetectionData*>(packet.data);
    if (detection->ret_code != 0) {
      return;
    }

    char ip_str[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, detection->lidar_ip, ip_str, sizeof(ip_str));

    if (!lidar_ip_.empty() && lidar_ip_ != ip_str) {
      return;
    }

    RCLCPP_INFO(get_logger(), "detected LiDAR %s (type=%u, cmd_port=%u)", ip_str, detection->dev_type,
                detection->cmd_port);

    if (!configured_.exchange(true)) {
      sendLidarConfiguration(ip_str, detection->cmd_port);
    }
    return;
  }

  if (packet.cmd_id == kCommandIDLidarWorkModeControl && packet.cmd_type == kCommandTypeAck &&
      packet.data_len >= sizeof(LivoxLidarAsyncControlResponse)) {
    const auto* response = reinterpret_cast<const LivoxLidarAsyncControlResponse*>(packet.data);
    if (response->ret_code == 0 && response->error_key == 0) {
      RCLCPP_INFO(get_logger(), "LiDAR configuration acknowledged (port=%u)", source_port);
      configured_ = true;
    } else {
      RCLCPP_WARN(get_logger(), "LiDAR configuration failed (ret=%u, error_key=%u)", response->ret_code,
                  response->error_key);
      configured_ = false;
    }
  }
}

void Mid360Driver::handlePointCloudPacket(const LivoxLidarEthernetPacket* packet)
{
  RawPacketSnapshot snapshot;
  if (!snapshotFromLivoxPacket(packet, snapshot)) {
    if (packet != nullptr && packet->dot_num > 0) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "unsupported point cloud data_type=%u",
                           packet->data_type);
    }
    return;
  }

  std::lock_guard<std::mutex> lock(packets_mutex_);
  accumulated_packets_.push_back(std::move(snapshot));
}

void Mid360Driver::handleImuPacket(const LivoxLidarEthernetPacket* packet)
{
  if (packet->data_type != kLivoxLidarImuData) {
    return;
  }

  const auto* imu_raw = reinterpret_cast<const LivoxLidarImuRawPoint*>(packet->data);
  sensor_msgs::msg::Imu imu_msg;
  imu_msg.header.stamp = packetTimestamp(packet);
  imu_msg.header.frame_id = frame_id_;
  imu_msg.angular_velocity.x = imu_raw->gyro_x;
  imu_msg.angular_velocity.y = imu_raw->gyro_y;
  imu_msg.angular_velocity.z = imu_raw->gyro_z;
  imu_msg.linear_acceleration.x = imu_raw->acc_x * kStandardGravity;
  imu_msg.linear_acceleration.y = imu_raw->acc_y * kStandardGravity;
  imu_msg.linear_acceleration.z = imu_raw->acc_z * kStandardGravity;
  imu_pub_->publish(imu_msg);
}

void Mid360Driver::handleDataPacket(const uint8_t* buffer, size_t size)
{
  if (size < kEthPacketHeaderSize) {
    return;
  }

  const auto* packet = reinterpret_cast<const LivoxLidarEthernetPacket*>(buffer);
  if (packet->data_type == kLivoxLidarImuData) {
    handleImuPacket(packet);
  } else {
    handlePointCloudPacket(packet);
  }
}

rclcpp::Time Mid360Driver::packetTimestamp(const LivoxLidarEthernetPacket* packet) const
{
  if (packet->time_type == static_cast<uint8_t>(TimestampType::kGptpOrPtp) ||
      packet->time_type == static_cast<uint8_t>(TimestampType::kGps)) {
    uint64_t stamp_ns = 0;
    std::memcpy(&stamp_ns, packet->timestamp, sizeof(stamp_ns));
    return rclcpp::Time(stamp_ns);
  }
  return now();
}

void Mid360Driver::publishPointCloud()
{
  std::vector<RawPacketSnapshot> packets;
  {
    std::lock_guard<std::mutex> lock(packets_mutex_);
    packets.swap(accumulated_packets_);
  }

  if (packets.empty()) {
    return;
  }

  const rclcpp::Time stamp = now();
  packet_pub_->publish(toRosPacket(stamp, frame_id_, packets));

  const auto points = decodeRawPackets(packets);
  pointcloud_pub_->publish(toPointCloud2(points, stamp, frame_id_));
}

void Mid360Driver::ioLoop()
{
  uint8_t buffer[kMaxBufferSize];

  while (running_) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    int max_fd = -1;

    const int sockets[] = {detection_socket_, point_socket_, imu_socket_, cmd_socket_, push_socket_};
    for (int socket : sockets) {
      if (socket >= 0) {
        FD_SET(socket, &read_fds);
        max_fd = std::max(max_fd, socket);
      }
    }

    if (max_fd < 0) {
      break;
    }

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 200000;

    const int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
    if (ready <= 0) {
      continue;
    }

    for (int socket : sockets) {
      if (socket < 0 || !FD_ISSET(socket, &read_fds)) {
        continue;
      }

      sockaddr_in source{};
      socklen_t source_len = sizeof(source);
      const ssize_t received = recvfrom(socket, buffer, sizeof(buffer), 0,
                                        reinterpret_cast<sockaddr*>(&source), &source_len);
      if (received <= 0) {
        continue;
      }

      char source_ip[INET_ADDRSTRLEN] = {0};
      inet_ntop(AF_INET, &source.sin_addr, source_ip, sizeof(source_ip));
      if (source_ip == host_ip_) {
        continue;
      }

      const uint16_t source_port = ntohs(source.sin_port);
      if (buffer[0] == kSdkProtocolSof) {
        handleCommandPacket(buffer, static_cast<size_t>(received), source_port);
      } else {
        handleDataPacket(buffer, static_cast<size_t>(received));
      }
    }
  }
}

void Mid360Driver::detectionLoop()
{
  while (running_) {
    sendDiscoveryBroadcast();

    if (!configured_ && !lidar_ip_.empty()) {
      sendLidarConfiguration(lidar_ip_, kMid360LidarCmdPort);
    }

    for (int i = 0; i < 10 && running_; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
}

bool Mid360Driver::start()
{
  if (!createSocket(detection_socket_, kDetectionPort)) {
    return false;
  }
  if (!createSocket(point_socket_, point_data_port_)) {
    return false;
  }
  if (!createSocket(imu_socket_, imu_data_port_)) {
    return false;
  }
  if (!createSocket(cmd_socket_, kMid360HostCmdPort)) {
    return false;
  }
  if (!createSocket(push_socket_, push_msg_port_)) {
    return false;
  }

  pointcloud_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>("points", 10);
  packet_pub_ = create_publisher<mid360_driver::msg::Mid360Packet>("packet", 10);
  imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("imu", 100);
  publish_timer_ = create_wall_timer(std::chrono::duration<double>(1.0 / publish_rate_),
                                     [this]() { publishPointCloud(); });

  running_ = true;
  io_thread_ = std::thread(&Mid360Driver::ioLoop, this);
  detection_thread_ = std::thread(&Mid360Driver::detectionLoop, this);

  RCLCPP_INFO(get_logger(), "driver started (host=%s, lidar=%s)", host_ip_.c_str(), lidar_ip_.c_str());
  return true;
}

void Mid360Driver::stop()
{
  running_ = false;

  if (publish_timer_) {
    publish_timer_->cancel();
    publish_timer_.reset();
  }

  if (detection_thread_.joinable()) {
    detection_thread_.join();
  }
  if (io_thread_.joinable()) {
    io_thread_.join();
  }

  closeSocket(detection_socket_);
  closeSocket(point_socket_);
  closeSocket(imu_socket_);
  closeSocket(cmd_socket_);
  closeSocket(push_socket_);
}

}  // namespace mid360

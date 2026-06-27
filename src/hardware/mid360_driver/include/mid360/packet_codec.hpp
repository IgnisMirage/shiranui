#ifndef MID360_PACKET_CODEC_HPP
#define MID360_PACKET_CODEC_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <rclcpp/time.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include "mid360/livox_protocol.hpp"
#include "mid360_driver/msg/mid360_packet.hpp"
#include "mid360_driver/msg/mid360_raw_packet.hpp"

namespace mid360
{

struct RawPacketSnapshot
{
  uint8_t data_type = 0;
  uint16_t dot_num = 0;
  uint16_t time_interval = 0;
  uint16_t udp_cnt = 0;
  uint8_t frame_cnt = 0;
  uint8_t time_type = 0;
  uint8_t livox_timestamp[8] = {0};
  std::vector<uint8_t> data;
};

size_t pointStride(uint8_t data_type);
bool snapshotFromLivoxPacket(const LivoxLidarEthernetPacket* packet, RawPacketSnapshot& snapshot);
mid360_driver::msg::Mid360RawPacket toRosRawPacket(const RawPacketSnapshot& snapshot);
mid360_driver::msg::Mid360Packet toRosPacket(const rclcpp::Time& stamp, const std::string& frame_id,
                                             const std::vector<RawPacketSnapshot>& snapshots);
std::vector<PointXYZIT> decodeRawPacket(const RawPacketSnapshot& snapshot);
std::vector<PointXYZIT> decodeRawPackets(const std::vector<RawPacketSnapshot>& snapshots);
std::vector<PointXYZIT> decodeMid360Packet(const mid360_driver::msg::Mid360Packet& packet_msg);
sensor_msgs::msg::PointCloud2 toPointCloud2(const std::vector<PointXYZIT>& points, const rclcpp::Time& stamp,
                                            const std::string& frame_id);
sensor_msgs::msg::PointCloud2 mid360PacketToPointCloud2(const mid360_driver::msg::Mid360Packet& packet_msg);

}  // namespace mid360

#endif  // MID360_PACKET_CODEC_HPP

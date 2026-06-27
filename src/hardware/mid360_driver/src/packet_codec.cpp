#include "mid360/packet_codec.hpp"

#include <cstring>

#include <sensor_msgs/point_cloud2_iterator.hpp>

namespace mid360
{

size_t pointStride(uint8_t data_type)
{
  if (data_type == kLivoxLidarCartesianCoordinateHighData) {
    return sizeof(LivoxLidarCartesianHighRawPoint);
  }
  if (data_type == kLivoxLidarCartesianCoordinateLowData) {
    return sizeof(LivoxLidarCartesianLowRawPoint);
  }
  return 0;
}

bool snapshotFromLivoxPacket(const LivoxLidarEthernetPacket* packet, RawPacketSnapshot& snapshot)
{
  if (packet == nullptr || packet->dot_num == 0) {
    return false;
  }

  const size_t stride = pointStride(packet->data_type);
  if (stride == 0) {
    return false;
  }

  const size_t data_len = stride * packet->dot_num;
  snapshot.data_type = packet->data_type;
  snapshot.dot_num = packet->dot_num;
  snapshot.time_interval = packet->time_interval;
  snapshot.udp_cnt = packet->udp_cnt;
  snapshot.frame_cnt = packet->frame_cnt;
  snapshot.time_type = packet->time_type;
  std::memcpy(snapshot.livox_timestamp, packet->timestamp, sizeof(snapshot.livox_timestamp));
  snapshot.data.assign(packet->data, packet->data + data_len);
  return true;
}

mid360_driver::msg::Mid360RawPacket toRosRawPacket(const RawPacketSnapshot& snapshot)
{
  mid360_driver::msg::Mid360RawPacket raw_msg;
  raw_msg.data_type = snapshot.data_type;
  raw_msg.dot_num = snapshot.dot_num;
  raw_msg.time_interval = snapshot.time_interval;
  raw_msg.udp_cnt = snapshot.udp_cnt;
  raw_msg.frame_cnt = snapshot.frame_cnt;
  raw_msg.time_type = snapshot.time_type;
  std::memcpy(raw_msg.livox_timestamp.data(), snapshot.livox_timestamp, raw_msg.livox_timestamp.size());
  raw_msg.data = snapshot.data;
  return raw_msg;
}

mid360_driver::msg::Mid360Packet toRosPacket(const rclcpp::Time& stamp, const std::string& frame_id,
                                              const std::vector<RawPacketSnapshot>& snapshots)
{
  mid360_driver::msg::Mid360Packet packet_msg;
  packet_msg.header.stamp = stamp;
  packet_msg.header.frame_id = frame_id;
  packet_msg.packets.reserve(snapshots.size());
  for (const auto& snapshot : snapshots) {
    packet_msg.packets.push_back(toRosRawPacket(snapshot));
  }
  return packet_msg;
}

std::vector<PointXYZIT> decodeRawPacket(const RawPacketSnapshot& snapshot)
{
  std::vector<PointXYZIT> points;
  if (snapshot.dot_num == 0 || snapshot.data.empty()) {
    return points;
  }

  const size_t stride = pointStride(snapshot.data_type);
  if (stride == 0 || snapshot.data.size() < stride * snapshot.dot_num) {
    return points;
  }

  points.reserve(snapshot.dot_num);
  const uint8_t* raw = snapshot.data.data();

  if (snapshot.data_type == kLivoxLidarCartesianCoordinateHighData) {
    const auto* cartesian = reinterpret_cast<const LivoxLidarCartesianHighRawPoint*>(raw);
    for (uint32_t i = 0; i < snapshot.dot_num; ++i) {
      PointXYZIT point{};
      point.x = static_cast<float>(cartesian[i].x) / 1000.0f;
      point.y = static_cast<float>(cartesian[i].y) / 1000.0f;
      point.z = static_cast<float>(cartesian[i].z) / 1000.0f;
      point.intensity = static_cast<float>(cartesian[i].reflectivity);
      point.tag = cartesian[i].tag;
      point.line = static_cast<uint8_t>(i % kLineNumberMid360);
      points.push_back(point);
    }
  } else if (snapshot.data_type == kLivoxLidarCartesianCoordinateLowData) {
    const auto* cartesian = reinterpret_cast<const LivoxLidarCartesianLowRawPoint*>(raw);
    for (uint32_t i = 0; i < snapshot.dot_num; ++i) {
      PointXYZIT point{};
      point.x = static_cast<float>(cartesian[i].x) / 100.0f;
      point.y = static_cast<float>(cartesian[i].y) / 100.0f;
      point.z = static_cast<float>(cartesian[i].z) / 100.0f;
      point.intensity = static_cast<float>(cartesian[i].reflectivity);
      point.tag = cartesian[i].tag;
      point.line = static_cast<uint8_t>(i % kLineNumberMid360);
      points.push_back(point);
    }
  }

  return points;
}

std::vector<PointXYZIT> decodeRawPackets(const std::vector<RawPacketSnapshot>& snapshots)
{
  std::vector<PointXYZIT> points;
  for (const auto& snapshot : snapshots) {
    const auto packet_points = decodeRawPacket(snapshot);
    points.insert(points.end(), packet_points.begin(), packet_points.end());
  }
  return points;
}

RawPacketSnapshot snapshotFromRosRawPacket(const mid360_driver::msg::Mid360RawPacket& raw_msg)
{
  RawPacketSnapshot snapshot;
  snapshot.data_type = raw_msg.data_type;
  snapshot.dot_num = raw_msg.dot_num;
  snapshot.time_interval = raw_msg.time_interval;
  snapshot.udp_cnt = raw_msg.udp_cnt;
  snapshot.frame_cnt = raw_msg.frame_cnt;
  snapshot.time_type = raw_msg.time_type;
  std::memcpy(snapshot.livox_timestamp, raw_msg.livox_timestamp.data(), raw_msg.livox_timestamp.size());
  snapshot.data = raw_msg.data;
  return snapshot;
}

std::vector<PointXYZIT> decodeMid360Packet(const mid360_driver::msg::Mid360Packet& packet_msg)
{
  std::vector<RawPacketSnapshot> snapshots;
  snapshots.reserve(packet_msg.packets.size());
  for (const auto& raw_msg : packet_msg.packets) {
    snapshots.push_back(snapshotFromRosRawPacket(raw_msg));
  }
  return decodeRawPackets(snapshots);
}

sensor_msgs::msg::PointCloud2 toPointCloud2(const std::vector<PointXYZIT>& points, const rclcpp::Time& stamp,
                                            const std::string& frame_id)
{
  sensor_msgs::msg::PointCloud2 cloud_msg;
  cloud_msg.header.stamp = stamp;
  cloud_msg.header.frame_id = frame_id;
  cloud_msg.height = 1;
  cloud_msg.width = static_cast<uint32_t>(points.size());
  cloud_msg.is_dense = false;
  cloud_msg.is_bigendian = false;
  cloud_msg.point_step = 20;
  cloud_msg.row_step = cloud_msg.point_step * cloud_msg.width;

  cloud_msg.fields.resize(6);
  const char* names[] = {"x", "y", "z", "intensity", "tag", "line"};
  const uint8_t offsets[] = {0, 4, 8, 12, 16, 17};
  const uint8_t datatypes[] = {
      sensor_msgs::msg::PointField::FLOAT32, sensor_msgs::msg::PointField::FLOAT32,
      sensor_msgs::msg::PointField::FLOAT32, sensor_msgs::msg::PointField::FLOAT32,
      sensor_msgs::msg::PointField::UINT8,   sensor_msgs::msg::PointField::UINT8};
  for (int i = 0; i < 6; ++i) {
    cloud_msg.fields[i].name = names[i];
    cloud_msg.fields[i].offset = offsets[i];
    cloud_msg.fields[i].datatype = datatypes[i];
    cloud_msg.fields[i].count = 1;
  }

  cloud_msg.data.resize(cloud_msg.row_step);
  sensor_msgs::PointCloud2Iterator<float> iter_x(cloud_msg, "x");
  sensor_msgs::PointCloud2Iterator<float> iter_y(cloud_msg, "y");
  sensor_msgs::PointCloud2Iterator<float> iter_z(cloud_msg, "z");
  sensor_msgs::PointCloud2Iterator<float> iter_intensity(cloud_msg, "intensity");
  sensor_msgs::PointCloud2Iterator<uint8_t> iter_tag(cloud_msg, "tag");
  sensor_msgs::PointCloud2Iterator<uint8_t> iter_line(cloud_msg, "line");

  for (const auto& point : points) {
    *iter_x = point.x;
    *iter_y = point.y;
    *iter_z = point.z;
    *iter_intensity = point.intensity;
    *iter_tag = point.tag;
    *iter_line = point.line;
    ++iter_x;
    ++iter_y;
    ++iter_z;
    ++iter_intensity;
    ++iter_tag;
    ++iter_line;
  }

  return cloud_msg;
}

sensor_msgs::msg::PointCloud2 mid360PacketToPointCloud2(const mid360_driver::msg::Mid360Packet& packet_msg)
{
  const auto points = decodeMid360Packet(packet_msg);
  return toPointCloud2(points, packet_msg.header.stamp, packet_msg.header.frame_id);
}

}  // namespace mid360

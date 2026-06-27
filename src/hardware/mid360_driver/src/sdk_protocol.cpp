#include "mid360/sdk_protocol.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <sstream>

#include "mid360/crc.hpp"

namespace mid360
{

bool SdkProtocol::pack(uint8_t* buffer, uint32_t buffer_size, uint32_t* out_length, const CommPacket& packet) const
{
  auto* sdk_packet = reinterpret_cast<SdkPacket*>(buffer);
  const uint32_t total_length = packet.data_len + kPacketWrapperSize;
  if (total_length > buffer_size)
  {
    return false;
  }

  sdk_packet->sof = kSdkProtocolSof;
  sdk_packet->version = kSdkVer;
  sdk_packet->length = static_cast<uint16_t>(total_length);
  sdk_packet->seq_num = packet.seq_num & 0xFFFF;
  sdk_packet->cmd_id = packet.cmd_id;
  sdk_packet->cmd_type = packet.cmd_type;
  sdk_packet->sender_type = packet.sender_type;
  std::memset(sdk_packet->rsvd, 0, sizeof(sdk_packet->rsvd));

  sdk_packet->crc16_h = crc16Ccitt(buffer, 18);
  if (packet.data_len == 0)
  {
    sdk_packet->crc32_d = 0;
  }
  else
  {
    sdk_packet->crc32_d = crc32(packet.data, packet.data_len);
    std::memcpy(sdk_packet->data, packet.data, packet.data_len);
  }

  *out_length = total_length;
  return true;
}

bool SdkProtocol::parse(uint8_t* buffer, uint32_t buffer_size, CommPacket* packet) const
{
  if (buffer_size < kPacketWrapperSize)
  {
    return false;
  }

  auto* sdk_packet = reinterpret_cast<SdkPacket*>(buffer);
  if (sdk_packet->sof != kSdkProtocolSof || sdk_packet->version != kSdkVer)
  {
    return false;
  }

  if (sdk_packet->crc16_h != crc16Ccitt(buffer, 18))
  {
    return false;
  }

  const uint16_t data_len = sdk_packet->length - kPacketWrapperSize;
  const uint32_t expected_crc = data_len == 0 ? 0 : crc32(sdk_packet->data, data_len);
  if (sdk_packet->crc32_d != expected_crc)
  {
    return false;
  }

  packet->protocol = 0;
  packet->version = sdk_packet->version;
  packet->seq_num = sdk_packet->seq_num;
  packet->cmd_id = sdk_packet->cmd_id;
  packet->cmd_type = sdk_packet->cmd_type;
  packet->sender_type = sdk_packet->sender_type;
  packet->data = sdk_packet->data;
  packet->data_len = data_len;
  return true;
}

bool parseIpAddress(const std::string& ip, uint8_t out[4])
{
  std::stringstream ss(ip);
  std::string token;
  for (int i = 0; i < 4; ++i)
  {
    if (!std::getline(ss, token, '.'))
    {
      return false;
    }
    out[i] = static_cast<uint8_t>(std::stoi(token));
  }
  return true;
}

bool buildMid360ConfigRequest(const std::string& host_ip,
                              uint16_t push_msg_port,
                              uint16_t point_data_port,
                              uint16_t imu_data_port,
                              std::vector<uint8_t>& request)
{
  uint8_t host_ip_bytes[4] = {0};
  if (!parseIpAddress(host_ip, host_ip_bytes))
  {
    return false;
  }

  request.assign(kMaxCommandBufferSize, 0);
  uint16_t req_len = 0;

  const uint16_t key_num = 3;
  std::memcpy(&request[req_len], &key_num, sizeof(key_num));
  req_len = static_cast<uint16_t>(sizeof(key_num) + sizeof(uint16_t));

  auto appendHostIpKv = [&](uint16_t key, uint16_t host_port, uint16_t lidar_port) {
    auto* kv = reinterpret_cast<LivoxLidarKeyValueParam*>(&request[req_len]);
    kv->key = key;
    kv->length = sizeof(uint8_t) * 8;
    auto* host_info = reinterpret_cast<HostIpInfoValue*>(kv->value);
    std::memcpy(host_info->host_ip, host_ip_bytes, sizeof(host_ip_bytes));
    host_info->host_port = host_port;
    host_info->lidar_port = lidar_port;
    req_len = static_cast<uint16_t>(req_len + sizeof(LivoxLidarKeyValueParam) - 1 + sizeof(HostIpInfoValue));
  };

  appendHostIpKv(kKeyStateInfoHostIpCfg, push_msg_port, kMid360LidarPushMsgPort);
  appendHostIpKv(kKeyLidarPointDataHostIpCfg, point_data_port, kMid360LidarPointCloudPort);
  appendHostIpKv(kKeyLidarImuHostIpCfg, imu_data_port, kMid360LidarImuDataPort);

  request.resize(req_len);
  return true;
}

}  // namespace mid360

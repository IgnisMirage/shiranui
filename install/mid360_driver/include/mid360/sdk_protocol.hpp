#ifndef MID360_SDK_PROTOCOL_HPP
#define MID360_SDK_PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "mid360/livox_protocol.hpp"

namespace mid360
{

struct CommPacket
{
  uint8_t protocol;
  uint8_t version;
  uint32_t seq_num;
  uint16_t cmd_id;
  uint8_t cmd_type;
  uint8_t sender_type;
  uint8_t* data;
  uint16_t data_len;
};

class SdkProtocol
{
public:
  bool pack(uint8_t* buffer, uint32_t buffer_size, uint32_t* out_length, const CommPacket& packet) const;
  bool parse(uint8_t* buffer, uint32_t buffer_size, CommPacket* packet) const;

private:
  static constexpr uint32_t kPacketWrapperSize = 24;
};

bool parseIpAddress(const std::string& ip, uint8_t out[4]);
bool buildMid360ConfigRequest(const std::string& host_ip,
                              uint16_t push_msg_port,
                              uint16_t point_data_port,
                              uint16_t imu_data_port,
                              std::vector<uint8_t>& request);

}  // namespace mid360

#endif  // MID360_SDK_PROTOCOL_HPP

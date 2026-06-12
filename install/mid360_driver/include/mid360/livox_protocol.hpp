#ifndef MID360_LIVOX_PROTOCOL_HPP
#define MID360_LIVOX_PROTOCOL_HPP

#include <cstdint>

namespace mid360
{

#pragma pack(push, 1)

struct LivoxLidarEthernetPacket
{
  uint8_t version;
  uint16_t length;
  uint16_t time_interval;
  uint16_t dot_num;
  uint16_t udp_cnt;
  uint8_t frame_cnt;
  uint8_t data_type;
  uint8_t time_type;
  uint8_t rsvd[12];
  uint32_t crc32;
  uint8_t timestamp[8];
  uint8_t data[1];
};

struct LivoxLidarImuRawPoint
{
  float gyro_x;
  float gyro_y;
  float gyro_z;
  float acc_x;
  float acc_y;
  float acc_z;
};

struct LivoxLidarCartesianHighRawPoint
{
  int32_t x;
  int32_t y;
  int32_t z;
  uint8_t reflectivity;
  uint8_t tag;
};

struct LivoxLidarCartesianLowRawPoint
{
  int16_t x;
  int16_t y;
  int16_t z;
  uint8_t reflectivity;
  uint8_t tag;
};

struct HostIpInfoValue
{
  uint8_t host_ip[4];
  uint16_t host_port;
  uint16_t lidar_port;
};

struct LivoxLidarKeyValueParam
{
  uint16_t key;
  uint16_t length;
  uint8_t value[1];
};

struct DetectionData
{
  uint8_t ret_code;
  uint8_t dev_type;
  char sn[16];
  uint8_t lidar_ip[4];
  uint16_t cmd_port;
};

struct LivoxLidarAsyncControlResponse
{
  uint8_t ret_code;
  uint16_t error_key;
};

struct SdkPacket
{
  uint8_t sof;
  uint8_t version;
  uint16_t length;
  uint32_t seq_num;
  uint16_t cmd_id;
  uint8_t cmd_type;
  uint8_t sender_type;
  char rsvd[6];
  uint16_t crc16_h;
  uint32_t crc32_d;
  uint8_t data[1];
};

#pragma pack(pop)

constexpr uint8_t kSdkProtocolSof = 0xAA;
constexpr uint8_t kSdkVer = 0;
constexpr uint32_t kEthPacketHeaderSize = 36;

constexpr uint16_t kDetectionPort = 56000;
constexpr uint16_t kMid360LidarCmdPort = 56100;
constexpr uint16_t kMid360LidarPushMsgPort = 56200;
constexpr uint16_t kMid360LidarPointCloudPort = 56300;
constexpr uint16_t kMid360LidarImuDataPort = 56400;

constexpr uint16_t kMid360HostCmdPort = 56101;
constexpr uint16_t kMid360HostPushMsgPort = 56201;
constexpr uint16_t kMid360HostPointCloudPort = 56301;
constexpr uint16_t kMid360HostImuDataPort = 56401;

constexpr uint16_t kCommandIDLidarSearch = 0x0000;
constexpr uint16_t kCommandIDLidarWorkModeControl = 0x0100;

constexpr uint8_t kCommandTypeCmd = 0;
constexpr uint8_t kCommandTypeAck = 1;
constexpr uint8_t kHostSend = 0;

constexpr uint16_t kKeyStateInfoHostIpCfg = 0x0005;
constexpr uint16_t kKeyLidarPointDataHostIpCfg = 0x0006;
constexpr uint16_t kKeyLidarImuHostIpCfg = 0x0007;

constexpr uint8_t kLivoxLidarImuData = 0;
constexpr uint8_t kLivoxLidarCartesianCoordinateHighData = 0x01;
constexpr uint8_t kLivoxLidarCartesianCoordinateLowData = 0x02;

constexpr uint8_t kLivoxLidarTypeMid360 = 9;
constexpr uint8_t kLineNumberMid360 = 4;

// Livox MID360 IMU acc is in g; sensor_msgs/Imu linear_acceleration expects m/s^2.
constexpr float kStandardGravity = 9.80665f;

constexpr uint32_t kMaxBufferSize = 8192;
constexpr uint32_t kMaxCommandBufferSize = 1400;

enum class TimestampType : uint8_t
{
  kNoSync = 0,
  kGptpOrPtp = 1,
  kGps = 2,
};

struct PointXYZIT
{
  float x;
  float y;
  float z;
  float intensity;
  uint8_t tag;
  uint8_t line;
};

}  // namespace mid360

#endif  // MID360_LIVOX_PROTOCOL_HPP

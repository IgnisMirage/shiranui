// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_raw_packet.h"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__STRUCT_H_
#define MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

// Include directives for member types
// Member 'data'
#include "rosidl_runtime_c/primitives_sequence.h"

/// Struct defined in msg/Mid360RawPacket in the package mid360_driver.
/**
  * One Livox MID-360 point-cloud UDP payload (compact raw form for rosbag recording)
 */
typedef struct mid360_driver__msg__Mid360RawPacket
{
  uint8_t data_type;
  uint16_t dot_num;
  uint16_t time_interval;
  uint16_t udp_cnt;
  uint8_t frame_cnt;
  uint8_t time_type;
  uint8_t livox_timestamp[8];
  rosidl_runtime_c__uint8__Sequence data;
} mid360_driver__msg__Mid360RawPacket;

// Struct for a sequence of mid360_driver__msg__Mid360RawPacket.
typedef struct mid360_driver__msg__Mid360RawPacket__Sequence
{
  mid360_driver__msg__Mid360RawPacket * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mid360_driver__msg__Mid360RawPacket__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__STRUCT_H_

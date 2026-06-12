// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mid360_driver:msg/Mid360Packet.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_packet.h"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_PACKET__STRUCT_H_
#define MID360_DRIVER__MSG__DETAIL__MID360_PACKET__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Constants defined in the message

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__struct.h"
// Member 'packets'
#include "mid360_driver/msg/detail/mid360_raw_packet__struct.h"

/// Struct defined in msg/Mid360Packet in the package mid360_driver.
/**
  * Aggregated Livox MID-360 scan frame (reversible to sensor_msgs/PointCloud2)
 */
typedef struct mid360_driver__msg__Mid360Packet
{
  std_msgs__msg__Header header;
  mid360_driver__msg__Mid360RawPacket__Sequence packets;
} mid360_driver__msg__Mid360Packet;

// Struct for a sequence of mid360_driver__msg__Mid360Packet.
typedef struct mid360_driver__msg__Mid360Packet__Sequence
{
  mid360_driver__msg__Mid360Packet * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mid360_driver__msg__Mid360Packet__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_PACKET__STRUCT_H_

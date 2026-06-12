// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from mid360_driver:msg/CustomMsg.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/custom_msg.h"


#ifndef MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__STRUCT_H_
#define MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__STRUCT_H_

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
// Member 'points'
#include "mid360_driver/msg/detail/custom_point__struct.h"

/// Struct defined in msg/CustomMsg in the package mid360_driver.
/**
  * Livox publish pointcloud msg format.
 */
typedef struct mid360_driver__msg__CustomMsg
{
  /// ROS standard message header
  std_msgs__msg__Header header;
  /// The time of first point
  uint64_t timebase;
  /// Total number of pointclouds
  uint32_t point_num;
  /// Lidar device id number
  uint8_t lidar_id;
  /// Reserved use
  uint8_t rsvd[3];
  /// Pointcloud data
  mid360_driver__msg__CustomPoint__Sequence points;
} mid360_driver__msg__CustomMsg;

// Struct for a sequence of mid360_driver__msg__CustomMsg.
typedef struct mid360_driver__msg__CustomMsg__Sequence
{
  mid360_driver__msg__CustomMsg * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} mid360_driver__msg__CustomMsg__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__STRUCT_H_

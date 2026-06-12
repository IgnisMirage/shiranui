// generated from rosidl_typesupport_fastrtps_cpp/resource/idl__rosidl_typesupport_fastrtps_cpp.hpp.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

#ifndef MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__ROSIDL_TYPESUPPORT_FASTRTPS_CPP_HPP_
#define MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__ROSIDL_TYPESUPPORT_FASTRTPS_CPP_HPP_

#include <cstddef>
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "mid360_driver/msg/rosidl_typesupport_fastrtps_cpp__visibility_control.h"
#include "mid360_driver/msg/detail/mid360_raw_packet__struct.hpp"

#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
#  pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
# endif
#endif
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

#include "fastcdr/Cdr.h"

namespace mid360_driver
{

namespace msg
{

namespace typesupport_fastrtps_cpp
{

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
cdr_serialize(
  const mid360_driver::msg::Mid360RawPacket & ros_message,
  eprosima::fastcdr::Cdr & cdr);

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  mid360_driver::msg::Mid360RawPacket & ros_message);

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
get_serialized_size(
  const mid360_driver::msg::Mid360RawPacket & ros_message,
  size_t current_alignment);

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
max_serialized_size_Mid360RawPacket(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
cdr_serialize_key(
  const mid360_driver::msg::Mid360RawPacket & ros_message,
  eprosima::fastcdr::Cdr &);

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
get_serialized_size_key(
  const mid360_driver::msg::Mid360RawPacket & ros_message,
  size_t current_alignment);

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
max_serialized_size_key_Mid360RawPacket(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

}  // namespace typesupport_fastrtps_cpp

}  // namespace msg

}  // namespace mid360_driver

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
const rosidl_message_type_support_t *
  ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, mid360_driver, msg, Mid360RawPacket)();

#ifdef __cplusplus
}
#endif

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__ROSIDL_TYPESUPPORT_FASTRTPS_CPP_HPP_

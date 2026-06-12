// generated from rosidl_typesupport_fastrtps_c/resource/idl__rosidl_typesupport_fastrtps_c.h.em
// with input from mid360_driver:msg/Mid360Packet.idl
// generated code does not contain a copyright notice
#ifndef MID360_DRIVER__MSG__DETAIL__MID360_PACKET__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
#define MID360_DRIVER__MSG__DETAIL__MID360_PACKET__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_


#include <stddef.h>
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "mid360_driver/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "mid360_driver/msg/detail/mid360_packet__struct.h"
#include "fastcdr/Cdr.h"

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
bool cdr_serialize_mid360_driver__msg__Mid360Packet(
  const mid360_driver__msg__Mid360Packet * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
bool cdr_deserialize_mid360_driver__msg__Mid360Packet(
  eprosima::fastcdr::Cdr &,
  mid360_driver__msg__Mid360Packet * ros_message);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
size_t get_serialized_size_mid360_driver__msg__Mid360Packet(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
size_t max_serialized_size_mid360_driver__msg__Mid360Packet(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
bool cdr_serialize_key_mid360_driver__msg__Mid360Packet(
  const mid360_driver__msg__Mid360Packet * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
size_t get_serialized_size_key_mid360_driver__msg__Mid360Packet(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
size_t max_serialized_size_key_mid360_driver__msg__Mid360Packet(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, mid360_driver, msg, Mid360Packet)();

#ifdef __cplusplus
}
#endif

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_PACKET__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_

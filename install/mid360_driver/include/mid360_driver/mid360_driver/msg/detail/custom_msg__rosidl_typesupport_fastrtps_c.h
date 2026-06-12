// generated from rosidl_typesupport_fastrtps_c/resource/idl__rosidl_typesupport_fastrtps_c.h.em
// with input from mid360_driver:msg/CustomMsg.idl
// generated code does not contain a copyright notice
#ifndef MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_
#define MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_


#include <stddef.h>
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_interface/macros.h"
#include "mid360_driver/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "mid360_driver/msg/detail/custom_msg__struct.h"
#include "fastcdr/Cdr.h"

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
bool cdr_serialize_mid360_driver__msg__CustomMsg(
  const mid360_driver__msg__CustomMsg * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
bool cdr_deserialize_mid360_driver__msg__CustomMsg(
  eprosima::fastcdr::Cdr &,
  mid360_driver__msg__CustomMsg * ros_message);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
size_t get_serialized_size_mid360_driver__msg__CustomMsg(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
size_t max_serialized_size_mid360_driver__msg__CustomMsg(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
bool cdr_serialize_key_mid360_driver__msg__CustomMsg(
  const mid360_driver__msg__CustomMsg * ros_message,
  eprosima::fastcdr::Cdr & cdr);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
size_t get_serialized_size_key_mid360_driver__msg__CustomMsg(
  const void * untyped_ros_message,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
size_t max_serialized_size_key_mid360_driver__msg__CustomMsg(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment);

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_mid360_driver
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, mid360_driver, msg, CustomMsg)();

#ifdef __cplusplus
}
#endif

#endif  // MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__ROSIDL_TYPESUPPORT_FASTRTPS_C_H_

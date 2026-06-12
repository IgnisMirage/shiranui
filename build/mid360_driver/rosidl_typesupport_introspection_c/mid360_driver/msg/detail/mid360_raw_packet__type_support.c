// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "mid360_driver/msg/detail/mid360_raw_packet__rosidl_typesupport_introspection_c.h"
#include "mid360_driver/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "mid360_driver/msg/detail/mid360_raw_packet__functions.h"
#include "mid360_driver/msg/detail/mid360_raw_packet__struct.h"


// Include directives for member types
// Member `data`
#include "rosidl_runtime_c/primitives_sequence_functions.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  mid360_driver__msg__Mid360RawPacket__init(message_memory);
}

void mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_fini_function(void * message_memory)
{
  mid360_driver__msg__Mid360RawPacket__fini(message_memory);
}

size_t mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__size_function__Mid360RawPacket__livox_timestamp(
  const void * untyped_member)
{
  (void)untyped_member;
  return 8;
}

const void * mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_const_function__Mid360RawPacket__livox_timestamp(
  const void * untyped_member, size_t index)
{
  const uint8_t * member =
    (const uint8_t *)(untyped_member);
  return &member[index];
}

void * mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_function__Mid360RawPacket__livox_timestamp(
  void * untyped_member, size_t index)
{
  uint8_t * member =
    (uint8_t *)(untyped_member);
  return &member[index];
}

void mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__fetch_function__Mid360RawPacket__livox_timestamp(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const uint8_t * item =
    ((const uint8_t *)
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_const_function__Mid360RawPacket__livox_timestamp(untyped_member, index));
  uint8_t * value =
    (uint8_t *)(untyped_value);
  *value = *item;
}

void mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__assign_function__Mid360RawPacket__livox_timestamp(
  void * untyped_member, size_t index, const void * untyped_value)
{
  uint8_t * item =
    ((uint8_t *)
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_function__Mid360RawPacket__livox_timestamp(untyped_member, index));
  const uint8_t * value =
    (const uint8_t *)(untyped_value);
  *item = *value;
}

size_t mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__size_function__Mid360RawPacket__data(
  const void * untyped_member)
{
  const rosidl_runtime_c__uint8__Sequence * member =
    (const rosidl_runtime_c__uint8__Sequence *)(untyped_member);
  return member->size;
}

const void * mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_const_function__Mid360RawPacket__data(
  const void * untyped_member, size_t index)
{
  const rosidl_runtime_c__uint8__Sequence * member =
    (const rosidl_runtime_c__uint8__Sequence *)(untyped_member);
  return &member->data[index];
}

void * mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_function__Mid360RawPacket__data(
  void * untyped_member, size_t index)
{
  rosidl_runtime_c__uint8__Sequence * member =
    (rosidl_runtime_c__uint8__Sequence *)(untyped_member);
  return &member->data[index];
}

void mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__fetch_function__Mid360RawPacket__data(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const uint8_t * item =
    ((const uint8_t *)
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_const_function__Mid360RawPacket__data(untyped_member, index));
  uint8_t * value =
    (uint8_t *)(untyped_value);
  *value = *item;
}

void mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__assign_function__Mid360RawPacket__data(
  void * untyped_member, size_t index, const void * untyped_value)
{
  uint8_t * item =
    ((uint8_t *)
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_function__Mid360RawPacket__data(untyped_member, index));
  const uint8_t * value =
    (const uint8_t *)(untyped_value);
  *item = *value;
}

bool mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__resize_function__Mid360RawPacket__data(
  void * untyped_member, size_t size)
{
  rosidl_runtime_c__uint8__Sequence * member =
    (rosidl_runtime_c__uint8__Sequence *)(untyped_member);
  rosidl_runtime_c__uint8__Sequence__fini(member);
  return rosidl_runtime_c__uint8__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_message_member_array[8] = {
  {
    "data_type",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360RawPacket, data_type),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "dot_num",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360RawPacket, dot_num),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "time_interval",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360RawPacket, time_interval),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "udp_cnt",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360RawPacket, udp_cnt),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "frame_cnt",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360RawPacket, frame_cnt),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "time_type",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360RawPacket, time_type),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "livox_timestamp",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    true,  // is array
    8,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360RawPacket, livox_timestamp),  // bytes offset in struct
    NULL,  // default value
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__size_function__Mid360RawPacket__livox_timestamp,  // size() function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_const_function__Mid360RawPacket__livox_timestamp,  // get_const(index) function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_function__Mid360RawPacket__livox_timestamp,  // get(index) function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__fetch_function__Mid360RawPacket__livox_timestamp,  // fetch(index, &value) function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__assign_function__Mid360RawPacket__livox_timestamp,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "data",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360RawPacket, data),  // bytes offset in struct
    NULL,  // default value
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__size_function__Mid360RawPacket__data,  // size() function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_const_function__Mid360RawPacket__data,  // get_const(index) function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__get_function__Mid360RawPacket__data,  // get(index) function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__fetch_function__Mid360RawPacket__data,  // fetch(index, &value) function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__assign_function__Mid360RawPacket__data,  // assign(index, value) function pointer
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__resize_function__Mid360RawPacket__data  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_message_members = {
  "mid360_driver__msg",  // message namespace
  "Mid360RawPacket",  // message name
  8,  // number of fields
  sizeof(mid360_driver__msg__Mid360RawPacket),
  false,  // has_any_key_member_
  mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_message_member_array,  // message members
  mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_init_function,  // function to initialize message memory (memory has to be allocated)
  mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_message_type_support_handle = {
  0,
  &mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_message_members,
  get_message_typesupport_handle_function,
  &mid360_driver__msg__Mid360RawPacket__get_type_hash,
  &mid360_driver__msg__Mid360RawPacket__get_type_description,
  &mid360_driver__msg__Mid360RawPacket__get_type_description_sources,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_mid360_driver
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mid360_driver, msg, Mid360RawPacket)() {
  if (!mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_message_type_support_handle.typesupport_identifier) {
    mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &mid360_driver__msg__Mid360RawPacket__rosidl_typesupport_introspection_c__Mid360RawPacket_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif

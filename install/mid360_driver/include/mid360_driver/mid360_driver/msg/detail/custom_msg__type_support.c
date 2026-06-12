// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from mid360_driver:msg/CustomMsg.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "mid360_driver/msg/detail/custom_msg__rosidl_typesupport_introspection_c.h"
#include "mid360_driver/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "mid360_driver/msg/detail/custom_msg__functions.h"
#include "mid360_driver/msg/detail/custom_msg__struct.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/header.h"
// Member `header`
#include "std_msgs/msg/detail/header__rosidl_typesupport_introspection_c.h"
// Member `points`
#include "mid360_driver/msg/custom_point.h"
// Member `points`
#include "mid360_driver/msg/detail/custom_point__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  mid360_driver__msg__CustomMsg__init(message_memory);
}

void mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_fini_function(void * message_memory)
{
  mid360_driver__msg__CustomMsg__fini(message_memory);
}

size_t mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__size_function__CustomMsg__rsvd(
  const void * untyped_member)
{
  (void)untyped_member;
  return 3;
}

const void * mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_const_function__CustomMsg__rsvd(
  const void * untyped_member, size_t index)
{
  const uint8_t * member =
    (const uint8_t *)(untyped_member);
  return &member[index];
}

void * mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_function__CustomMsg__rsvd(
  void * untyped_member, size_t index)
{
  uint8_t * member =
    (uint8_t *)(untyped_member);
  return &member[index];
}

void mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__fetch_function__CustomMsg__rsvd(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const uint8_t * item =
    ((const uint8_t *)
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_const_function__CustomMsg__rsvd(untyped_member, index));
  uint8_t * value =
    (uint8_t *)(untyped_value);
  *value = *item;
}

void mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__assign_function__CustomMsg__rsvd(
  void * untyped_member, size_t index, const void * untyped_value)
{
  uint8_t * item =
    ((uint8_t *)
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_function__CustomMsg__rsvd(untyped_member, index));
  const uint8_t * value =
    (const uint8_t *)(untyped_value);
  *item = *value;
}

size_t mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__size_function__CustomMsg__points(
  const void * untyped_member)
{
  const mid360_driver__msg__CustomPoint__Sequence * member =
    (const mid360_driver__msg__CustomPoint__Sequence *)(untyped_member);
  return member->size;
}

const void * mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_const_function__CustomMsg__points(
  const void * untyped_member, size_t index)
{
  const mid360_driver__msg__CustomPoint__Sequence * member =
    (const mid360_driver__msg__CustomPoint__Sequence *)(untyped_member);
  return &member->data[index];
}

void * mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_function__CustomMsg__points(
  void * untyped_member, size_t index)
{
  mid360_driver__msg__CustomPoint__Sequence * member =
    (mid360_driver__msg__CustomPoint__Sequence *)(untyped_member);
  return &member->data[index];
}

void mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__fetch_function__CustomMsg__points(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const mid360_driver__msg__CustomPoint * item =
    ((const mid360_driver__msg__CustomPoint *)
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_const_function__CustomMsg__points(untyped_member, index));
  mid360_driver__msg__CustomPoint * value =
    (mid360_driver__msg__CustomPoint *)(untyped_value);
  *value = *item;
}

void mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__assign_function__CustomMsg__points(
  void * untyped_member, size_t index, const void * untyped_value)
{
  mid360_driver__msg__CustomPoint * item =
    ((mid360_driver__msg__CustomPoint *)
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_function__CustomMsg__points(untyped_member, index));
  const mid360_driver__msg__CustomPoint * value =
    (const mid360_driver__msg__CustomPoint *)(untyped_value);
  *item = *value;
}

bool mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__resize_function__CustomMsg__points(
  void * untyped_member, size_t size)
{
  mid360_driver__msg__CustomPoint__Sequence * member =
    (mid360_driver__msg__CustomPoint__Sequence *)(untyped_member);
  mid360_driver__msg__CustomPoint__Sequence__fini(member);
  return mid360_driver__msg__CustomPoint__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_member_array[6] = {
  {
    "header",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__CustomMsg, header),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "timebase",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT64,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__CustomMsg, timebase),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "point_num",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT32,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__CustomMsg, point_num),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "lidar_id",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__CustomMsg, lidar_id),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "rsvd",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is key
    true,  // is array
    3,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__CustomMsg, rsvd),  // bytes offset in struct
    NULL,  // default value
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__size_function__CustomMsg__rsvd,  // size() function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_const_function__CustomMsg__rsvd,  // get_const(index) function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_function__CustomMsg__rsvd,  // get(index) function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__fetch_function__CustomMsg__rsvd,  // fetch(index, &value) function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__assign_function__CustomMsg__rsvd,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "points",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__CustomMsg, points),  // bytes offset in struct
    NULL,  // default value
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__size_function__CustomMsg__points,  // size() function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_const_function__CustomMsg__points,  // get_const(index) function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__get_function__CustomMsg__points,  // get(index) function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__fetch_function__CustomMsg__points,  // fetch(index, &value) function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__assign_function__CustomMsg__points,  // assign(index, value) function pointer
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__resize_function__CustomMsg__points  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_members = {
  "mid360_driver__msg",  // message namespace
  "CustomMsg",  // message name
  6,  // number of fields
  sizeof(mid360_driver__msg__CustomMsg),
  false,  // has_any_key_member_
  mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_member_array,  // message members
  mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_init_function,  // function to initialize message memory (memory has to be allocated)
  mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_type_support_handle = {
  0,
  &mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_members,
  get_message_typesupport_handle_function,
  &mid360_driver__msg__CustomMsg__get_type_hash,
  &mid360_driver__msg__CustomMsg__get_type_description,
  &mid360_driver__msg__CustomMsg__get_type_description_sources,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_mid360_driver
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mid360_driver, msg, CustomMsg)() {
  mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_member_array[0].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, std_msgs, msg, Header)();
  mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_member_array[5].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mid360_driver, msg, CustomPoint)();
  if (!mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_type_support_handle.typesupport_identifier) {
    mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &mid360_driver__msg__CustomMsg__rosidl_typesupport_introspection_c__CustomMsg_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif

// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from mid360_driver:msg/Mid360Packet.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "mid360_driver/msg/detail/mid360_packet__rosidl_typesupport_introspection_c.h"
#include "mid360_driver/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "mid360_driver/msg/detail/mid360_packet__functions.h"
#include "mid360_driver/msg/detail/mid360_packet__struct.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/header.h"
// Member `header`
#include "std_msgs/msg/detail/header__rosidl_typesupport_introspection_c.h"
// Member `packets`
#include "mid360_driver/msg/mid360_raw_packet.h"
// Member `packets`
#include "mid360_driver/msg/detail/mid360_raw_packet__rosidl_typesupport_introspection_c.h"

#ifdef __cplusplus
extern "C"
{
#endif

void mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  mid360_driver__msg__Mid360Packet__init(message_memory);
}

void mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_fini_function(void * message_memory)
{
  mid360_driver__msg__Mid360Packet__fini(message_memory);
}

size_t mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__size_function__Mid360Packet__packets(
  const void * untyped_member)
{
  const mid360_driver__msg__Mid360RawPacket__Sequence * member =
    (const mid360_driver__msg__Mid360RawPacket__Sequence *)(untyped_member);
  return member->size;
}

const void * mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__get_const_function__Mid360Packet__packets(
  const void * untyped_member, size_t index)
{
  const mid360_driver__msg__Mid360RawPacket__Sequence * member =
    (const mid360_driver__msg__Mid360RawPacket__Sequence *)(untyped_member);
  return &member->data[index];
}

void * mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__get_function__Mid360Packet__packets(
  void * untyped_member, size_t index)
{
  mid360_driver__msg__Mid360RawPacket__Sequence * member =
    (mid360_driver__msg__Mid360RawPacket__Sequence *)(untyped_member);
  return &member->data[index];
}

void mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__fetch_function__Mid360Packet__packets(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const mid360_driver__msg__Mid360RawPacket * item =
    ((const mid360_driver__msg__Mid360RawPacket *)
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__get_const_function__Mid360Packet__packets(untyped_member, index));
  mid360_driver__msg__Mid360RawPacket * value =
    (mid360_driver__msg__Mid360RawPacket *)(untyped_value);
  *value = *item;
}

void mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__assign_function__Mid360Packet__packets(
  void * untyped_member, size_t index, const void * untyped_value)
{
  mid360_driver__msg__Mid360RawPacket * item =
    ((mid360_driver__msg__Mid360RawPacket *)
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__get_function__Mid360Packet__packets(untyped_member, index));
  const mid360_driver__msg__Mid360RawPacket * value =
    (const mid360_driver__msg__Mid360RawPacket *)(untyped_value);
  *item = *value;
}

bool mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__resize_function__Mid360Packet__packets(
  void * untyped_member, size_t size)
{
  mid360_driver__msg__Mid360RawPacket__Sequence * member =
    (mid360_driver__msg__Mid360RawPacket__Sequence *)(untyped_member);
  mid360_driver__msg__Mid360RawPacket__Sequence__fini(member);
  return mid360_driver__msg__Mid360RawPacket__Sequence__init(member, size);
}

static rosidl_typesupport_introspection_c__MessageMember mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_member_array[2] = {
  {
    "header",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360Packet, header),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "packets",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    NULL,  // members of sub message (initialized later)
    false,  // is key
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver__msg__Mid360Packet, packets),  // bytes offset in struct
    NULL,  // default value
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__size_function__Mid360Packet__packets,  // size() function pointer
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__get_const_function__Mid360Packet__packets,  // get_const(index) function pointer
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__get_function__Mid360Packet__packets,  // get(index) function pointer
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__fetch_function__Mid360Packet__packets,  // fetch(index, &value) function pointer
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__assign_function__Mid360Packet__packets,  // assign(index, value) function pointer
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__resize_function__Mid360Packet__packets  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_members = {
  "mid360_driver__msg",  // message namespace
  "Mid360Packet",  // message name
  2,  // number of fields
  sizeof(mid360_driver__msg__Mid360Packet),
  false,  // has_any_key_member_
  mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_member_array,  // message members
  mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_init_function,  // function to initialize message memory (memory has to be allocated)
  mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_type_support_handle = {
  0,
  &mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_members,
  get_message_typesupport_handle_function,
  &mid360_driver__msg__Mid360Packet__get_type_hash,
  &mid360_driver__msg__Mid360Packet__get_type_description,
  &mid360_driver__msg__Mid360Packet__get_type_description_sources,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_mid360_driver
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mid360_driver, msg, Mid360Packet)() {
  mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_member_array[0].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, std_msgs, msg, Header)();
  mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_member_array[1].members_ =
    ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, mid360_driver, msg, Mid360RawPacket)();
  if (!mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_type_support_handle.typesupport_identifier) {
    mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &mid360_driver__msg__Mid360Packet__rosidl_typesupport_introspection_c__Mid360Packet_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif

// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "mid360_driver/msg/detail/mid360_raw_packet__functions.h"
#include "mid360_driver/msg/detail/mid360_raw_packet__struct.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/message_type_support_decl.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

namespace mid360_driver
{

namespace msg
{

namespace rosidl_typesupport_introspection_cpp
{

void Mid360RawPacket_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) mid360_driver::msg::Mid360RawPacket(_init);
}

void Mid360RawPacket_fini_function(void * message_memory)
{
  auto typed_message = static_cast<mid360_driver::msg::Mid360RawPacket *>(message_memory);
  typed_message->~Mid360RawPacket();
}

size_t size_function__Mid360RawPacket__livox_timestamp(const void * untyped_member)
{
  (void)untyped_member;
  return 8;
}

const void * get_const_function__Mid360RawPacket__livox_timestamp(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint8_t, 8> *>(untyped_member);
  return &member[index];
}

void * get_function__Mid360RawPacket__livox_timestamp(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint8_t, 8> *>(untyped_member);
  return &member[index];
}

void fetch_function__Mid360RawPacket__livox_timestamp(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint8_t *>(
    get_const_function__Mid360RawPacket__livox_timestamp(untyped_member, index));
  auto & value = *reinterpret_cast<uint8_t *>(untyped_value);
  value = item;
}

void assign_function__Mid360RawPacket__livox_timestamp(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint8_t *>(
    get_function__Mid360RawPacket__livox_timestamp(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint8_t *>(untyped_value);
  item = value;
}

size_t size_function__Mid360RawPacket__data(const void * untyped_member)
{
  const auto * member = reinterpret_cast<const std::vector<uint8_t> *>(untyped_member);
  return member->size();
}

const void * get_const_function__Mid360RawPacket__data(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::vector<uint8_t> *>(untyped_member);
  return &member[index];
}

void * get_function__Mid360RawPacket__data(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::vector<uint8_t> *>(untyped_member);
  return &member[index];
}

void fetch_function__Mid360RawPacket__data(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint8_t *>(
    get_const_function__Mid360RawPacket__data(untyped_member, index));
  auto & value = *reinterpret_cast<uint8_t *>(untyped_value);
  value = item;
}

void assign_function__Mid360RawPacket__data(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint8_t *>(
    get_function__Mid360RawPacket__data(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint8_t *>(untyped_value);
  item = value;
}

void resize_function__Mid360RawPacket__data(void * untyped_member, size_t size)
{
  auto * member =
    reinterpret_cast<std::vector<uint8_t> *>(untyped_member);
  member->resize(size);
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember Mid360RawPacket_message_member_array[8] = {
  {
    "data_type",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360RawPacket, data_type),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "dot_num",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360RawPacket, dot_num),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "time_interval",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360RawPacket, time_interval),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "udp_cnt",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360RawPacket, udp_cnt),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "frame_cnt",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360RawPacket, frame_cnt),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "time_type",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360RawPacket, time_type),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "livox_timestamp",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    true,  // is array
    8,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360RawPacket, livox_timestamp),  // bytes offset in struct
    nullptr,  // default value
    size_function__Mid360RawPacket__livox_timestamp,  // size() function pointer
    get_const_function__Mid360RawPacket__livox_timestamp,  // get_const(index) function pointer
    get_function__Mid360RawPacket__livox_timestamp,  // get(index) function pointer
    fetch_function__Mid360RawPacket__livox_timestamp,  // fetch(index, &value) function pointer
    assign_function__Mid360RawPacket__livox_timestamp,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "data",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    false,  // is key
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360RawPacket, data),  // bytes offset in struct
    nullptr,  // default value
    size_function__Mid360RawPacket__data,  // size() function pointer
    get_const_function__Mid360RawPacket__data,  // get_const(index) function pointer
    get_function__Mid360RawPacket__data,  // get(index) function pointer
    fetch_function__Mid360RawPacket__data,  // fetch(index, &value) function pointer
    assign_function__Mid360RawPacket__data,  // assign(index, value) function pointer
    resize_function__Mid360RawPacket__data  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers Mid360RawPacket_message_members = {
  "mid360_driver::msg",  // message namespace
  "Mid360RawPacket",  // message name
  8,  // number of fields
  sizeof(mid360_driver::msg::Mid360RawPacket),
  false,  // has_any_key_member_
  Mid360RawPacket_message_member_array,  // message members
  Mid360RawPacket_init_function,  // function to initialize message memory (memory has to be allocated)
  Mid360RawPacket_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t Mid360RawPacket_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &Mid360RawPacket_message_members,
  get_message_typesupport_handle_function,
  &mid360_driver__msg__Mid360RawPacket__get_type_hash,
  &mid360_driver__msg__Mid360RawPacket__get_type_description,
  &mid360_driver__msg__Mid360RawPacket__get_type_description_sources,
};

}  // namespace rosidl_typesupport_introspection_cpp

}  // namespace msg

}  // namespace mid360_driver


namespace rosidl_typesupport_introspection_cpp
{

template<>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<mid360_driver::msg::Mid360RawPacket>()
{
  return &::mid360_driver::msg::rosidl_typesupport_introspection_cpp::Mid360RawPacket_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, mid360_driver, msg, Mid360RawPacket)() {
  return &::mid360_driver::msg::rosidl_typesupport_introspection_cpp::Mid360RawPacket_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif

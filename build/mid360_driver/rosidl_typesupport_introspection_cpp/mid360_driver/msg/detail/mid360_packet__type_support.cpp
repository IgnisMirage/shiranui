// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from mid360_driver:msg/Mid360Packet.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "mid360_driver/msg/detail/mid360_packet__functions.h"
#include "mid360_driver/msg/detail/mid360_packet__struct.hpp"
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

void Mid360Packet_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) mid360_driver::msg::Mid360Packet(_init);
}

void Mid360Packet_fini_function(void * message_memory)
{
  auto typed_message = static_cast<mid360_driver::msg::Mid360Packet *>(message_memory);
  typed_message->~Mid360Packet();
}

size_t size_function__Mid360Packet__packets(const void * untyped_member)
{
  const auto * member = reinterpret_cast<const std::vector<mid360_driver::msg::Mid360RawPacket> *>(untyped_member);
  return member->size();
}

const void * get_const_function__Mid360Packet__packets(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::vector<mid360_driver::msg::Mid360RawPacket> *>(untyped_member);
  return &member[index];
}

void * get_function__Mid360Packet__packets(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::vector<mid360_driver::msg::Mid360RawPacket> *>(untyped_member);
  return &member[index];
}

void fetch_function__Mid360Packet__packets(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const mid360_driver::msg::Mid360RawPacket *>(
    get_const_function__Mid360Packet__packets(untyped_member, index));
  auto & value = *reinterpret_cast<mid360_driver::msg::Mid360RawPacket *>(untyped_value);
  value = item;
}

void assign_function__Mid360Packet__packets(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<mid360_driver::msg::Mid360RawPacket *>(
    get_function__Mid360Packet__packets(untyped_member, index));
  const auto & value = *reinterpret_cast<const mid360_driver::msg::Mid360RawPacket *>(untyped_value);
  item = value;
}

void resize_function__Mid360Packet__packets(void * untyped_member, size_t size)
{
  auto * member =
    reinterpret_cast<std::vector<mid360_driver::msg::Mid360RawPacket> *>(untyped_member);
  member->resize(size);
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember Mid360Packet_message_member_array[2] = {
  {
    "header",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    ::rosidl_typesupport_introspection_cpp::get_message_type_support_handle<std_msgs::msg::Header>(),  // members of sub message
    false,  // is key
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360Packet, header),  // bytes offset in struct
    nullptr,  // default value
    nullptr,  // size() function pointer
    nullptr,  // get_const(index) function pointer
    nullptr,  // get(index) function pointer
    nullptr,  // fetch(index, &value) function pointer
    nullptr,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "packets",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_MESSAGE,  // type
    0,  // upper bound of string
    ::rosidl_typesupport_introspection_cpp::get_message_type_support_handle<mid360_driver::msg::Mid360RawPacket>(),  // members of sub message
    false,  // is key
    true,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(mid360_driver::msg::Mid360Packet, packets),  // bytes offset in struct
    nullptr,  // default value
    size_function__Mid360Packet__packets,  // size() function pointer
    get_const_function__Mid360Packet__packets,  // get_const(index) function pointer
    get_function__Mid360Packet__packets,  // get(index) function pointer
    fetch_function__Mid360Packet__packets,  // fetch(index, &value) function pointer
    assign_function__Mid360Packet__packets,  // assign(index, value) function pointer
    resize_function__Mid360Packet__packets  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers Mid360Packet_message_members = {
  "mid360_driver::msg",  // message namespace
  "Mid360Packet",  // message name
  2,  // number of fields
  sizeof(mid360_driver::msg::Mid360Packet),
  false,  // has_any_key_member_
  Mid360Packet_message_member_array,  // message members
  Mid360Packet_init_function,  // function to initialize message memory (memory has to be allocated)
  Mid360Packet_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t Mid360Packet_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &Mid360Packet_message_members,
  get_message_typesupport_handle_function,
  &mid360_driver__msg__Mid360Packet__get_type_hash,
  &mid360_driver__msg__Mid360Packet__get_type_description,
  &mid360_driver__msg__Mid360Packet__get_type_description_sources,
};

}  // namespace rosidl_typesupport_introspection_cpp

}  // namespace msg

}  // namespace mid360_driver


namespace rosidl_typesupport_introspection_cpp
{

template<>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<mid360_driver::msg::Mid360Packet>()
{
  return &::mid360_driver::msg::rosidl_typesupport_introspection_cpp::Mid360Packet_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, mid360_driver, msg, Mid360Packet)() {
  return &::mid360_driver::msg::rosidl_typesupport_introspection_cpp::Mid360Packet_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif

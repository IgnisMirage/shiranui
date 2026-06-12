// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mid360_driver:msg/Mid360Packet.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_packet.hpp"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_PACKET__BUILDER_HPP_
#define MID360_DRIVER__MSG__DETAIL__MID360_PACKET__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mid360_driver/msg/detail/mid360_packet__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mid360_driver
{

namespace msg
{

namespace builder
{

class Init_Mid360Packet_packets
{
public:
  explicit Init_Mid360Packet_packets(::mid360_driver::msg::Mid360Packet & msg)
  : msg_(msg)
  {}
  ::mid360_driver::msg::Mid360Packet packets(::mid360_driver::msg::Mid360Packet::_packets_type arg)
  {
    msg_.packets = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mid360_driver::msg::Mid360Packet msg_;
};

class Init_Mid360Packet_header
{
public:
  Init_Mid360Packet_header()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_Mid360Packet_packets header(::mid360_driver::msg::Mid360Packet::_header_type arg)
  {
    msg_.header = std::move(arg);
    return Init_Mid360Packet_packets(msg_);
  }

private:
  ::mid360_driver::msg::Mid360Packet msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mid360_driver::msg::Mid360Packet>()
{
  return mid360_driver::msg::builder::Init_Mid360Packet_header();
}

}  // namespace mid360_driver

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_PACKET__BUILDER_HPP_

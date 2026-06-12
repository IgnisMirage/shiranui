// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_raw_packet.hpp"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__BUILDER_HPP_
#define MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mid360_driver/msg/detail/mid360_raw_packet__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mid360_driver
{

namespace msg
{

namespace builder
{

class Init_Mid360RawPacket_data
{
public:
  explicit Init_Mid360RawPacket_data(::mid360_driver::msg::Mid360RawPacket & msg)
  : msg_(msg)
  {}
  ::mid360_driver::msg::Mid360RawPacket data(::mid360_driver::msg::Mid360RawPacket::_data_type arg)
  {
    msg_.data = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mid360_driver::msg::Mid360RawPacket msg_;
};

class Init_Mid360RawPacket_livox_timestamp
{
public:
  explicit Init_Mid360RawPacket_livox_timestamp(::mid360_driver::msg::Mid360RawPacket & msg)
  : msg_(msg)
  {}
  Init_Mid360RawPacket_data livox_timestamp(::mid360_driver::msg::Mid360RawPacket::_livox_timestamp_type arg)
  {
    msg_.livox_timestamp = std::move(arg);
    return Init_Mid360RawPacket_data(msg_);
  }

private:
  ::mid360_driver::msg::Mid360RawPacket msg_;
};

class Init_Mid360RawPacket_time_type
{
public:
  explicit Init_Mid360RawPacket_time_type(::mid360_driver::msg::Mid360RawPacket & msg)
  : msg_(msg)
  {}
  Init_Mid360RawPacket_livox_timestamp time_type(::mid360_driver::msg::Mid360RawPacket::_time_type_type arg)
  {
    msg_.time_type = std::move(arg);
    return Init_Mid360RawPacket_livox_timestamp(msg_);
  }

private:
  ::mid360_driver::msg::Mid360RawPacket msg_;
};

class Init_Mid360RawPacket_frame_cnt
{
public:
  explicit Init_Mid360RawPacket_frame_cnt(::mid360_driver::msg::Mid360RawPacket & msg)
  : msg_(msg)
  {}
  Init_Mid360RawPacket_time_type frame_cnt(::mid360_driver::msg::Mid360RawPacket::_frame_cnt_type arg)
  {
    msg_.frame_cnt = std::move(arg);
    return Init_Mid360RawPacket_time_type(msg_);
  }

private:
  ::mid360_driver::msg::Mid360RawPacket msg_;
};

class Init_Mid360RawPacket_udp_cnt
{
public:
  explicit Init_Mid360RawPacket_udp_cnt(::mid360_driver::msg::Mid360RawPacket & msg)
  : msg_(msg)
  {}
  Init_Mid360RawPacket_frame_cnt udp_cnt(::mid360_driver::msg::Mid360RawPacket::_udp_cnt_type arg)
  {
    msg_.udp_cnt = std::move(arg);
    return Init_Mid360RawPacket_frame_cnt(msg_);
  }

private:
  ::mid360_driver::msg::Mid360RawPacket msg_;
};

class Init_Mid360RawPacket_time_interval
{
public:
  explicit Init_Mid360RawPacket_time_interval(::mid360_driver::msg::Mid360RawPacket & msg)
  : msg_(msg)
  {}
  Init_Mid360RawPacket_udp_cnt time_interval(::mid360_driver::msg::Mid360RawPacket::_time_interval_type arg)
  {
    msg_.time_interval = std::move(arg);
    return Init_Mid360RawPacket_udp_cnt(msg_);
  }

private:
  ::mid360_driver::msg::Mid360RawPacket msg_;
};

class Init_Mid360RawPacket_dot_num
{
public:
  explicit Init_Mid360RawPacket_dot_num(::mid360_driver::msg::Mid360RawPacket & msg)
  : msg_(msg)
  {}
  Init_Mid360RawPacket_time_interval dot_num(::mid360_driver::msg::Mid360RawPacket::_dot_num_type arg)
  {
    msg_.dot_num = std::move(arg);
    return Init_Mid360RawPacket_time_interval(msg_);
  }

private:
  ::mid360_driver::msg::Mid360RawPacket msg_;
};

class Init_Mid360RawPacket_data_type
{
public:
  Init_Mid360RawPacket_data_type()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_Mid360RawPacket_dot_num data_type(::mid360_driver::msg::Mid360RawPacket::_data_type_type arg)
  {
    msg_.data_type = std::move(arg);
    return Init_Mid360RawPacket_dot_num(msg_);
  }

private:
  ::mid360_driver::msg::Mid360RawPacket msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mid360_driver::msg::Mid360RawPacket>()
{
  return mid360_driver::msg::builder::Init_Mid360RawPacket_data_type();
}

}  // namespace mid360_driver

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__BUILDER_HPP_

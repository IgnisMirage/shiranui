// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mid360_driver:msg/CustomMsg.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/custom_msg.hpp"


#ifndef MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__BUILDER_HPP_
#define MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mid360_driver/msg/detail/custom_msg__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mid360_driver
{

namespace msg
{

namespace builder
{

class Init_CustomMsg_points
{
public:
  explicit Init_CustomMsg_points(::mid360_driver::msg::CustomMsg & msg)
  : msg_(msg)
  {}
  ::mid360_driver::msg::CustomMsg points(::mid360_driver::msg::CustomMsg::_points_type arg)
  {
    msg_.points = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mid360_driver::msg::CustomMsg msg_;
};

class Init_CustomMsg_rsvd
{
public:
  explicit Init_CustomMsg_rsvd(::mid360_driver::msg::CustomMsg & msg)
  : msg_(msg)
  {}
  Init_CustomMsg_points rsvd(::mid360_driver::msg::CustomMsg::_rsvd_type arg)
  {
    msg_.rsvd = std::move(arg);
    return Init_CustomMsg_points(msg_);
  }

private:
  ::mid360_driver::msg::CustomMsg msg_;
};

class Init_CustomMsg_lidar_id
{
public:
  explicit Init_CustomMsg_lidar_id(::mid360_driver::msg::CustomMsg & msg)
  : msg_(msg)
  {}
  Init_CustomMsg_rsvd lidar_id(::mid360_driver::msg::CustomMsg::_lidar_id_type arg)
  {
    msg_.lidar_id = std::move(arg);
    return Init_CustomMsg_rsvd(msg_);
  }

private:
  ::mid360_driver::msg::CustomMsg msg_;
};

class Init_CustomMsg_point_num
{
public:
  explicit Init_CustomMsg_point_num(::mid360_driver::msg::CustomMsg & msg)
  : msg_(msg)
  {}
  Init_CustomMsg_lidar_id point_num(::mid360_driver::msg::CustomMsg::_point_num_type arg)
  {
    msg_.point_num = std::move(arg);
    return Init_CustomMsg_lidar_id(msg_);
  }

private:
  ::mid360_driver::msg::CustomMsg msg_;
};

class Init_CustomMsg_timebase
{
public:
  explicit Init_CustomMsg_timebase(::mid360_driver::msg::CustomMsg & msg)
  : msg_(msg)
  {}
  Init_CustomMsg_point_num timebase(::mid360_driver::msg::CustomMsg::_timebase_type arg)
  {
    msg_.timebase = std::move(arg);
    return Init_CustomMsg_point_num(msg_);
  }

private:
  ::mid360_driver::msg::CustomMsg msg_;
};

class Init_CustomMsg_header
{
public:
  Init_CustomMsg_header()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_CustomMsg_timebase header(::mid360_driver::msg::CustomMsg::_header_type arg)
  {
    msg_.header = std::move(arg);
    return Init_CustomMsg_timebase(msg_);
  }

private:
  ::mid360_driver::msg::CustomMsg msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mid360_driver::msg::CustomMsg>()
{
  return mid360_driver::msg::builder::Init_CustomMsg_header();
}

}  // namespace mid360_driver

#endif  // MID360_DRIVER__MSG__DETAIL__CUSTOM_MSG__BUILDER_HPP_

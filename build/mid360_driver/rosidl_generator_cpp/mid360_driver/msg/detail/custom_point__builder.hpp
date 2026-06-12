// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from mid360_driver:msg/CustomPoint.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/custom_point.hpp"


#ifndef MID360_DRIVER__MSG__DETAIL__CUSTOM_POINT__BUILDER_HPP_
#define MID360_DRIVER__MSG__DETAIL__CUSTOM_POINT__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "mid360_driver/msg/detail/custom_point__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace mid360_driver
{

namespace msg
{

namespace builder
{

class Init_CustomPoint_line
{
public:
  explicit Init_CustomPoint_line(::mid360_driver::msg::CustomPoint & msg)
  : msg_(msg)
  {}
  ::mid360_driver::msg::CustomPoint line(::mid360_driver::msg::CustomPoint::_line_type arg)
  {
    msg_.line = std::move(arg);
    return std::move(msg_);
  }

private:
  ::mid360_driver::msg::CustomPoint msg_;
};

class Init_CustomPoint_tag
{
public:
  explicit Init_CustomPoint_tag(::mid360_driver::msg::CustomPoint & msg)
  : msg_(msg)
  {}
  Init_CustomPoint_line tag(::mid360_driver::msg::CustomPoint::_tag_type arg)
  {
    msg_.tag = std::move(arg);
    return Init_CustomPoint_line(msg_);
  }

private:
  ::mid360_driver::msg::CustomPoint msg_;
};

class Init_CustomPoint_reflectivity
{
public:
  explicit Init_CustomPoint_reflectivity(::mid360_driver::msg::CustomPoint & msg)
  : msg_(msg)
  {}
  Init_CustomPoint_tag reflectivity(::mid360_driver::msg::CustomPoint::_reflectivity_type arg)
  {
    msg_.reflectivity = std::move(arg);
    return Init_CustomPoint_tag(msg_);
  }

private:
  ::mid360_driver::msg::CustomPoint msg_;
};

class Init_CustomPoint_z
{
public:
  explicit Init_CustomPoint_z(::mid360_driver::msg::CustomPoint & msg)
  : msg_(msg)
  {}
  Init_CustomPoint_reflectivity z(::mid360_driver::msg::CustomPoint::_z_type arg)
  {
    msg_.z = std::move(arg);
    return Init_CustomPoint_reflectivity(msg_);
  }

private:
  ::mid360_driver::msg::CustomPoint msg_;
};

class Init_CustomPoint_y
{
public:
  explicit Init_CustomPoint_y(::mid360_driver::msg::CustomPoint & msg)
  : msg_(msg)
  {}
  Init_CustomPoint_z y(::mid360_driver::msg::CustomPoint::_y_type arg)
  {
    msg_.y = std::move(arg);
    return Init_CustomPoint_z(msg_);
  }

private:
  ::mid360_driver::msg::CustomPoint msg_;
};

class Init_CustomPoint_x
{
public:
  explicit Init_CustomPoint_x(::mid360_driver::msg::CustomPoint & msg)
  : msg_(msg)
  {}
  Init_CustomPoint_y x(::mid360_driver::msg::CustomPoint::_x_type arg)
  {
    msg_.x = std::move(arg);
    return Init_CustomPoint_y(msg_);
  }

private:
  ::mid360_driver::msg::CustomPoint msg_;
};

class Init_CustomPoint_offset_time
{
public:
  Init_CustomPoint_offset_time()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_CustomPoint_x offset_time(::mid360_driver::msg::CustomPoint::_offset_time_type arg)
  {
    msg_.offset_time = std::move(arg);
    return Init_CustomPoint_x(msg_);
  }

private:
  ::mid360_driver::msg::CustomPoint msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::mid360_driver::msg::CustomPoint>()
{
  return mid360_driver::msg::builder::Init_CustomPoint_offset_time();
}

}  // namespace mid360_driver

#endif  // MID360_DRIVER__MSG__DETAIL__CUSTOM_POINT__BUILDER_HPP_

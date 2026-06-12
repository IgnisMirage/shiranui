// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mid360_driver:msg/Mid360Packet.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_packet.hpp"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_PACKET__TRAITS_HPP_
#define MID360_DRIVER__MSG__DETAIL__MID360_PACKET__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mid360_driver/msg/detail/mid360_packet__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__traits.hpp"
// Member 'packets'
#include "mid360_driver/msg/detail/mid360_raw_packet__traits.hpp"

namespace mid360_driver
{

namespace msg
{

inline void to_flow_style_yaml(
  const Mid360Packet & msg,
  std::ostream & out)
{
  out << "{";
  // member: header
  {
    out << "header: ";
    to_flow_style_yaml(msg.header, out);
    out << ", ";
  }

  // member: packets
  {
    if (msg.packets.size() == 0) {
      out << "packets: []";
    } else {
      out << "packets: [";
      size_t pending_items = msg.packets.size();
      for (auto item : msg.packets) {
        to_flow_style_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const Mid360Packet & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: header
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "header:\n";
    to_block_style_yaml(msg.header, out, indentation + 2);
  }

  // member: packets
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.packets.size() == 0) {
      out << "packets: []\n";
    } else {
      out << "packets:\n";
      for (auto item : msg.packets) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "-\n";
        to_block_style_yaml(item, out, indentation + 2);
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const Mid360Packet & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace mid360_driver

namespace rosidl_generator_traits
{

[[deprecated("use mid360_driver::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const mid360_driver::msg::Mid360Packet & msg,
  std::ostream & out, size_t indentation = 0)
{
  mid360_driver::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mid360_driver::msg::to_yaml() instead")]]
inline std::string to_yaml(const mid360_driver::msg::Mid360Packet & msg)
{
  return mid360_driver::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mid360_driver::msg::Mid360Packet>()
{
  return "mid360_driver::msg::Mid360Packet";
}

template<>
inline const char * name<mid360_driver::msg::Mid360Packet>()
{
  return "mid360_driver/msg/Mid360Packet";
}

template<>
struct has_fixed_size<mid360_driver::msg::Mid360Packet>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mid360_driver::msg::Mid360Packet>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mid360_driver::msg::Mid360Packet>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_PACKET__TRAITS_HPP_

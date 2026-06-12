// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_raw_packet.hpp"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__TRAITS_HPP_
#define MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "mid360_driver/msg/detail/mid360_raw_packet__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace mid360_driver
{

namespace msg
{

inline void to_flow_style_yaml(
  const Mid360RawPacket & msg,
  std::ostream & out)
{
  out << "{";
  // member: data_type
  {
    out << "data_type: ";
    rosidl_generator_traits::value_to_yaml(msg.data_type, out);
    out << ", ";
  }

  // member: dot_num
  {
    out << "dot_num: ";
    rosidl_generator_traits::value_to_yaml(msg.dot_num, out);
    out << ", ";
  }

  // member: time_interval
  {
    out << "time_interval: ";
    rosidl_generator_traits::value_to_yaml(msg.time_interval, out);
    out << ", ";
  }

  // member: udp_cnt
  {
    out << "udp_cnt: ";
    rosidl_generator_traits::value_to_yaml(msg.udp_cnt, out);
    out << ", ";
  }

  // member: frame_cnt
  {
    out << "frame_cnt: ";
    rosidl_generator_traits::value_to_yaml(msg.frame_cnt, out);
    out << ", ";
  }

  // member: time_type
  {
    out << "time_type: ";
    rosidl_generator_traits::value_to_yaml(msg.time_type, out);
    out << ", ";
  }

  // member: livox_timestamp
  {
    if (msg.livox_timestamp.size() == 0) {
      out << "livox_timestamp: []";
    } else {
      out << "livox_timestamp: [";
      size_t pending_items = msg.livox_timestamp.size();
      for (auto item : msg.livox_timestamp) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: data
  {
    if (msg.data.size() == 0) {
      out << "data: []";
    } else {
      out << "data: [";
      size_t pending_items = msg.data.size();
      for (auto item : msg.data) {
        rosidl_generator_traits::value_to_yaml(item, out);
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
  const Mid360RawPacket & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: data_type
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "data_type: ";
    rosidl_generator_traits::value_to_yaml(msg.data_type, out);
    out << "\n";
  }

  // member: dot_num
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "dot_num: ";
    rosidl_generator_traits::value_to_yaml(msg.dot_num, out);
    out << "\n";
  }

  // member: time_interval
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "time_interval: ";
    rosidl_generator_traits::value_to_yaml(msg.time_interval, out);
    out << "\n";
  }

  // member: udp_cnt
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "udp_cnt: ";
    rosidl_generator_traits::value_to_yaml(msg.udp_cnt, out);
    out << "\n";
  }

  // member: frame_cnt
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "frame_cnt: ";
    rosidl_generator_traits::value_to_yaml(msg.frame_cnt, out);
    out << "\n";
  }

  // member: time_type
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "time_type: ";
    rosidl_generator_traits::value_to_yaml(msg.time_type, out);
    out << "\n";
  }

  // member: livox_timestamp
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.livox_timestamp.size() == 0) {
      out << "livox_timestamp: []\n";
    } else {
      out << "livox_timestamp:\n";
      for (auto item : msg.livox_timestamp) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: data
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.data.size() == 0) {
      out << "data: []\n";
    } else {
      out << "data:\n";
      for (auto item : msg.data) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const Mid360RawPacket & msg, bool use_flow_style = false)
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
  const mid360_driver::msg::Mid360RawPacket & msg,
  std::ostream & out, size_t indentation = 0)
{
  mid360_driver::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use mid360_driver::msg::to_yaml() instead")]]
inline std::string to_yaml(const mid360_driver::msg::Mid360RawPacket & msg)
{
  return mid360_driver::msg::to_yaml(msg);
}

template<>
inline const char * data_type<mid360_driver::msg::Mid360RawPacket>()
{
  return "mid360_driver::msg::Mid360RawPacket";
}

template<>
inline const char * name<mid360_driver::msg::Mid360RawPacket>()
{
  return "mid360_driver/msg/Mid360RawPacket";
}

template<>
struct has_fixed_size<mid360_driver::msg::Mid360RawPacket>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<mid360_driver::msg::Mid360RawPacket>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<mid360_driver::msg::Mid360RawPacket>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__TRAITS_HPP_

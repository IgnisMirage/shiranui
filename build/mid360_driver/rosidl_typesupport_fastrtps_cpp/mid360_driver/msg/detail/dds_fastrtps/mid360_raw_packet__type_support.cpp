// generated from rosidl_typesupport_fastrtps_cpp/resource/idl__type_support.cpp.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice
#include "mid360_driver/msg/detail/mid360_raw_packet__rosidl_typesupport_fastrtps_cpp.hpp"
#include "mid360_driver/msg/detail/mid360_raw_packet__functions.h"
#include "mid360_driver/msg/detail/mid360_raw_packet__struct.hpp"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_fastrtps_cpp/identifier.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support_decl.hpp"
#include "rosidl_typesupport_fastrtps_cpp/serialization_helpers.hpp"
#include "rosidl_typesupport_fastrtps_cpp/wstring_conversion.hpp"
#include "fastcdr/Cdr.h"


// forward declaration of message dependencies and their conversion functions

namespace mid360_driver
{

namespace msg
{

namespace typesupport_fastrtps_cpp
{


bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
cdr_serialize(
  const mid360_driver::msg::Mid360RawPacket & ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  // Member: data_type
  cdr << ros_message.data_type;

  // Member: dot_num
  cdr << ros_message.dot_num;

  // Member: time_interval
  cdr << ros_message.time_interval;

  // Member: udp_cnt
  cdr << ros_message.udp_cnt;

  // Member: frame_cnt
  cdr << ros_message.frame_cnt;

  // Member: time_type
  cdr << ros_message.time_type;

  // Member: livox_timestamp
  {
    cdr << ros_message.livox_timestamp;
  }

  // Member: data
  {
    cdr << ros_message.data;
  }

  return true;
}

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  mid360_driver::msg::Mid360RawPacket & ros_message)
{
  // Member: data_type
  cdr >> ros_message.data_type;

  // Member: dot_num
  cdr >> ros_message.dot_num;

  // Member: time_interval
  cdr >> ros_message.time_interval;

  // Member: udp_cnt
  cdr >> ros_message.udp_cnt;

  // Member: frame_cnt
  cdr >> ros_message.frame_cnt;

  // Member: time_type
  cdr >> ros_message.time_type;

  // Member: livox_timestamp
  {
    cdr >> ros_message.livox_timestamp;
  }

  // Member: data
  {
    cdr >> ros_message.data;
  }

  return true;
}  // NOLINT(readability/fn_size)


size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
get_serialized_size(
  const mid360_driver::msg::Mid360RawPacket & ros_message,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // Member: data_type
  {
    size_t item_size = sizeof(ros_message.data_type);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: dot_num
  {
    size_t item_size = sizeof(ros_message.dot_num);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: time_interval
  {
    size_t item_size = sizeof(ros_message.time_interval);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: udp_cnt
  {
    size_t item_size = sizeof(ros_message.udp_cnt);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: frame_cnt
  {
    size_t item_size = sizeof(ros_message.frame_cnt);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: time_type
  {
    size_t item_size = sizeof(ros_message.time_type);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: livox_timestamp
  {
    size_t array_size = 8;
    size_t item_size = sizeof(ros_message.livox_timestamp[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: data
  {
    size_t array_size = ros_message.data.size();
    current_alignment += padding +
      eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    size_t item_size = sizeof(ros_message.data[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  return current_alignment - initial_alignment;
}


size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
max_serialized_size_Mid360RawPacket(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  size_t last_member_size = 0;
  (void)last_member_size;
  (void)padding;
  (void)wchar_size;

  full_bounded = true;
  is_plain = true;

  // Member: data_type
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // Member: dot_num
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }
  // Member: time_interval
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }
  // Member: udp_cnt
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }
  // Member: frame_cnt
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // Member: time_type
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // Member: livox_timestamp
  {
    size_t array_size = 8;
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // Member: data
  {
    size_t array_size = 0;
    full_bounded = false;
    is_plain = false;
    current_alignment += padding +
      eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = mid360_driver::msg::Mid360RawPacket;
    is_plain =
      (
      offsetof(DataType, data) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
cdr_serialize_key(
  const mid360_driver::msg::Mid360RawPacket & ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  // Member: data_type
  cdr << ros_message.data_type;

  // Member: dot_num
  cdr << ros_message.dot_num;

  // Member: time_interval
  cdr << ros_message.time_interval;

  // Member: udp_cnt
  cdr << ros_message.udp_cnt;

  // Member: frame_cnt
  cdr << ros_message.frame_cnt;

  // Member: time_type
  cdr << ros_message.time_type;

  // Member: livox_timestamp
  {
    cdr << ros_message.livox_timestamp;
  }

  // Member: data
  {
    cdr << ros_message.data;
  }

  return true;
}

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
get_serialized_size_key(
  const mid360_driver::msg::Mid360RawPacket & ros_message,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // Member: data_type
  {
    size_t item_size = sizeof(ros_message.data_type);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: dot_num
  {
    size_t item_size = sizeof(ros_message.dot_num);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: time_interval
  {
    size_t item_size = sizeof(ros_message.time_interval);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: udp_cnt
  {
    size_t item_size = sizeof(ros_message.udp_cnt);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: frame_cnt
  {
    size_t item_size = sizeof(ros_message.frame_cnt);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: time_type
  {
    size_t item_size = sizeof(ros_message.time_type);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: livox_timestamp
  {
    size_t array_size = 8;
    size_t item_size = sizeof(ros_message.livox_timestamp[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  // Member: data
  {
    size_t array_size = ros_message.data.size();
    current_alignment += padding +
      eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    size_t item_size = sizeof(ros_message.data[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  return current_alignment - initial_alignment;
}

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_mid360_driver
max_serialized_size_key_Mid360RawPacket(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  size_t last_member_size = 0;
  (void)last_member_size;
  (void)padding;
  (void)wchar_size;

  full_bounded = true;
  is_plain = true;

  // Member: data_type
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  // Member: dot_num
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }

  // Member: time_interval
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }

  // Member: udp_cnt
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }

  // Member: frame_cnt
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  // Member: time_type
  {
    size_t array_size = 1;
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  // Member: livox_timestamp
  {
    size_t array_size = 8;
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  // Member: data
  {
    size_t array_size = 0;
    full_bounded = false;
    is_plain = false;
    current_alignment += padding +
      eprosima::fastcdr::Cdr::alignment(current_alignment, padding);
    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = mid360_driver::msg::Mid360RawPacket;
    is_plain =
      (
      offsetof(DataType, data) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}


static bool _Mid360RawPacket__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  auto typed_message =
    static_cast<const mid360_driver::msg::Mid360RawPacket *>(
    untyped_ros_message);
  return cdr_serialize(*typed_message, cdr);
}

static bool _Mid360RawPacket__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  auto typed_message =
    static_cast<mid360_driver::msg::Mid360RawPacket *>(
    untyped_ros_message);
  return cdr_deserialize(cdr, *typed_message);
}

static uint32_t _Mid360RawPacket__get_serialized_size(
  const void * untyped_ros_message)
{
  auto typed_message =
    static_cast<const mid360_driver::msg::Mid360RawPacket *>(
    untyped_ros_message);
  return static_cast<uint32_t>(get_serialized_size(*typed_message, 0));
}

static size_t _Mid360RawPacket__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_Mid360RawPacket(full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}

static message_type_support_callbacks_t _Mid360RawPacket__callbacks = {
  "mid360_driver::msg",
  "Mid360RawPacket",
  _Mid360RawPacket__cdr_serialize,
  _Mid360RawPacket__cdr_deserialize,
  _Mid360RawPacket__get_serialized_size,
  _Mid360RawPacket__max_serialized_size,
  nullptr
};

static rosidl_message_type_support_t _Mid360RawPacket__handle = {
  rosidl_typesupport_fastrtps_cpp::typesupport_identifier,
  &_Mid360RawPacket__callbacks,
  get_message_typesupport_handle_function,
  &mid360_driver__msg__Mid360RawPacket__get_type_hash,
  &mid360_driver__msg__Mid360RawPacket__get_type_description,
  &mid360_driver__msg__Mid360RawPacket__get_type_description_sources,
};

}  // namespace typesupport_fastrtps_cpp

}  // namespace msg

}  // namespace mid360_driver

namespace rosidl_typesupport_fastrtps_cpp
{

template<>
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_EXPORT_mid360_driver
const rosidl_message_type_support_t *
get_message_type_support_handle<mid360_driver::msg::Mid360RawPacket>()
{
  return &mid360_driver::msg::typesupport_fastrtps_cpp::_Mid360RawPacket__handle;
}

}  // namespace rosidl_typesupport_fastrtps_cpp

#ifdef __cplusplus
extern "C"
{
#endif

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, mid360_driver, msg, Mid360RawPacket)() {
  return &mid360_driver::msg::typesupport_fastrtps_cpp::_Mid360RawPacket__handle;
}

#ifdef __cplusplus
}
#endif

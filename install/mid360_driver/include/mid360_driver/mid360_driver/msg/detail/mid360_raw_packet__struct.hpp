// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_raw_packet.hpp"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__STRUCT_HPP_
#define MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__mid360_driver__msg__Mid360RawPacket __attribute__((deprecated))
#else
# define DEPRECATED__mid360_driver__msg__Mid360RawPacket __declspec(deprecated)
#endif

namespace mid360_driver
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct Mid360RawPacket_
{
  using Type = Mid360RawPacket_<ContainerAllocator>;

  explicit Mid360RawPacket_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->data_type = 0;
      this->dot_num = 0;
      this->time_interval = 0;
      this->udp_cnt = 0;
      this->frame_cnt = 0;
      this->time_type = 0;
      std::fill<typename std::array<uint8_t, 8>::iterator, uint8_t>(this->livox_timestamp.begin(), this->livox_timestamp.end(), 0);
    }
  }

  explicit Mid360RawPacket_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : livox_timestamp(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->data_type = 0;
      this->dot_num = 0;
      this->time_interval = 0;
      this->udp_cnt = 0;
      this->frame_cnt = 0;
      this->time_type = 0;
      std::fill<typename std::array<uint8_t, 8>::iterator, uint8_t>(this->livox_timestamp.begin(), this->livox_timestamp.end(), 0);
    }
  }

  // field types and members
  using _data_type_type =
    uint8_t;
  _data_type_type data_type;
  using _dot_num_type =
    uint16_t;
  _dot_num_type dot_num;
  using _time_interval_type =
    uint16_t;
  _time_interval_type time_interval;
  using _udp_cnt_type =
    uint16_t;
  _udp_cnt_type udp_cnt;
  using _frame_cnt_type =
    uint8_t;
  _frame_cnt_type frame_cnt;
  using _time_type_type =
    uint8_t;
  _time_type_type time_type;
  using _livox_timestamp_type =
    std::array<uint8_t, 8>;
  _livox_timestamp_type livox_timestamp;
  using _data_type =
    std::vector<uint8_t, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<uint8_t>>;
  _data_type data;

  // setters for named parameter idiom
  Type & set__data_type(
    const uint8_t & _arg)
  {
    this->data_type = _arg;
    return *this;
  }
  Type & set__dot_num(
    const uint16_t & _arg)
  {
    this->dot_num = _arg;
    return *this;
  }
  Type & set__time_interval(
    const uint16_t & _arg)
  {
    this->time_interval = _arg;
    return *this;
  }
  Type & set__udp_cnt(
    const uint16_t & _arg)
  {
    this->udp_cnt = _arg;
    return *this;
  }
  Type & set__frame_cnt(
    const uint8_t & _arg)
  {
    this->frame_cnt = _arg;
    return *this;
  }
  Type & set__time_type(
    const uint8_t & _arg)
  {
    this->time_type = _arg;
    return *this;
  }
  Type & set__livox_timestamp(
    const std::array<uint8_t, 8> & _arg)
  {
    this->livox_timestamp = _arg;
    return *this;
  }
  Type & set__data(
    const std::vector<uint8_t, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<uint8_t>> & _arg)
  {
    this->data = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mid360_driver::msg::Mid360RawPacket_<ContainerAllocator> *;
  using ConstRawPtr =
    const mid360_driver::msg::Mid360RawPacket_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mid360_driver__msg__Mid360RawPacket
    std::shared_ptr<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mid360_driver__msg__Mid360RawPacket
    std::shared_ptr<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const Mid360RawPacket_ & other) const
  {
    if (this->data_type != other.data_type) {
      return false;
    }
    if (this->dot_num != other.dot_num) {
      return false;
    }
    if (this->time_interval != other.time_interval) {
      return false;
    }
    if (this->udp_cnt != other.udp_cnt) {
      return false;
    }
    if (this->frame_cnt != other.frame_cnt) {
      return false;
    }
    if (this->time_type != other.time_type) {
      return false;
    }
    if (this->livox_timestamp != other.livox_timestamp) {
      return false;
    }
    if (this->data != other.data) {
      return false;
    }
    return true;
  }
  bool operator!=(const Mid360RawPacket_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct Mid360RawPacket_

// alias to use template instance with default allocator
using Mid360RawPacket =
  mid360_driver::msg::Mid360RawPacket_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mid360_driver

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__STRUCT_HPP_

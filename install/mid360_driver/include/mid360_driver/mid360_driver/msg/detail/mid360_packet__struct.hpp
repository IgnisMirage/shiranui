// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from mid360_driver:msg/Mid360Packet.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_packet.hpp"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_PACKET__STRUCT_HPP_
#define MID360_DRIVER__MSG__DETAIL__MID360_PACKET__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/header__struct.hpp"
// Member 'packets'
#include "mid360_driver/msg/detail/mid360_raw_packet__struct.hpp"

#ifndef _WIN32
# define DEPRECATED__mid360_driver__msg__Mid360Packet __attribute__((deprecated))
#else
# define DEPRECATED__mid360_driver__msg__Mid360Packet __declspec(deprecated)
#endif

namespace mid360_driver
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct Mid360Packet_
{
  using Type = Mid360Packet_<ContainerAllocator>;

  explicit Mid360Packet_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_init)
  {
    (void)_init;
  }

  explicit Mid360Packet_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : header(_alloc, _init)
  {
    (void)_init;
  }

  // field types and members
  using _header_type =
    std_msgs::msg::Header_<ContainerAllocator>;
  _header_type header;
  using _packets_type =
    std::vector<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>>>;
  _packets_type packets;

  // setters for named parameter idiom
  Type & set__header(
    const std_msgs::msg::Header_<ContainerAllocator> & _arg)
  {
    this->header = _arg;
    return *this;
  }
  Type & set__packets(
    const std::vector<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>, typename std::allocator_traits<ContainerAllocator>::template rebind_alloc<mid360_driver::msg::Mid360RawPacket_<ContainerAllocator>>> & _arg)
  {
    this->packets = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    mid360_driver::msg::Mid360Packet_<ContainerAllocator> *;
  using ConstRawPtr =
    const mid360_driver::msg::Mid360Packet_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<mid360_driver::msg::Mid360Packet_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<mid360_driver::msg::Mid360Packet_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      mid360_driver::msg::Mid360Packet_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<mid360_driver::msg::Mid360Packet_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      mid360_driver::msg::Mid360Packet_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<mid360_driver::msg::Mid360Packet_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<mid360_driver::msg::Mid360Packet_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<mid360_driver::msg::Mid360Packet_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__mid360_driver__msg__Mid360Packet
    std::shared_ptr<mid360_driver::msg::Mid360Packet_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__mid360_driver__msg__Mid360Packet
    std::shared_ptr<mid360_driver::msg::Mid360Packet_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const Mid360Packet_ & other) const
  {
    if (this->header != other.header) {
      return false;
    }
    if (this->packets != other.packets) {
      return false;
    }
    return true;
  }
  bool operator!=(const Mid360Packet_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct Mid360Packet_

// alias to use template instance with default allocator
using Mid360Packet =
  mid360_driver::msg::Mid360Packet_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace mid360_driver

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_PACKET__STRUCT_HPP_

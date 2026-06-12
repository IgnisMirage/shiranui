// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mid360_driver:msg/Mid360Packet.idl
// generated code does not contain a copyright notice
#include "mid360_driver/msg/detail/mid360_packet__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/detail/header__functions.h"
// Member `packets`
#include "mid360_driver/msg/detail/mid360_raw_packet__functions.h"

bool
mid360_driver__msg__Mid360Packet__init(mid360_driver__msg__Mid360Packet * msg)
{
  if (!msg) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__init(&msg->header)) {
    mid360_driver__msg__Mid360Packet__fini(msg);
    return false;
  }
  // packets
  if (!mid360_driver__msg__Mid360RawPacket__Sequence__init(&msg->packets, 0)) {
    mid360_driver__msg__Mid360Packet__fini(msg);
    return false;
  }
  return true;
}

void
mid360_driver__msg__Mid360Packet__fini(mid360_driver__msg__Mid360Packet * msg)
{
  if (!msg) {
    return;
  }
  // header
  std_msgs__msg__Header__fini(&msg->header);
  // packets
  mid360_driver__msg__Mid360RawPacket__Sequence__fini(&msg->packets);
}

bool
mid360_driver__msg__Mid360Packet__are_equal(const mid360_driver__msg__Mid360Packet * lhs, const mid360_driver__msg__Mid360Packet * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__are_equal(
      &(lhs->header), &(rhs->header)))
  {
    return false;
  }
  // packets
  if (!mid360_driver__msg__Mid360RawPacket__Sequence__are_equal(
      &(lhs->packets), &(rhs->packets)))
  {
    return false;
  }
  return true;
}

bool
mid360_driver__msg__Mid360Packet__copy(
  const mid360_driver__msg__Mid360Packet * input,
  mid360_driver__msg__Mid360Packet * output)
{
  if (!input || !output) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__copy(
      &(input->header), &(output->header)))
  {
    return false;
  }
  // packets
  if (!mid360_driver__msg__Mid360RawPacket__Sequence__copy(
      &(input->packets), &(output->packets)))
  {
    return false;
  }
  return true;
}

mid360_driver__msg__Mid360Packet *
mid360_driver__msg__Mid360Packet__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__Mid360Packet * msg = (mid360_driver__msg__Mid360Packet *)allocator.allocate(sizeof(mid360_driver__msg__Mid360Packet), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mid360_driver__msg__Mid360Packet));
  bool success = mid360_driver__msg__Mid360Packet__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mid360_driver__msg__Mid360Packet__destroy(mid360_driver__msg__Mid360Packet * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mid360_driver__msg__Mid360Packet__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mid360_driver__msg__Mid360Packet__Sequence__init(mid360_driver__msg__Mid360Packet__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__Mid360Packet * data = NULL;

  if (size) {
    data = (mid360_driver__msg__Mid360Packet *)allocator.zero_allocate(size, sizeof(mid360_driver__msg__Mid360Packet), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mid360_driver__msg__Mid360Packet__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mid360_driver__msg__Mid360Packet__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
mid360_driver__msg__Mid360Packet__Sequence__fini(mid360_driver__msg__Mid360Packet__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      mid360_driver__msg__Mid360Packet__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

mid360_driver__msg__Mid360Packet__Sequence *
mid360_driver__msg__Mid360Packet__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__Mid360Packet__Sequence * array = (mid360_driver__msg__Mid360Packet__Sequence *)allocator.allocate(sizeof(mid360_driver__msg__Mid360Packet__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mid360_driver__msg__Mid360Packet__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mid360_driver__msg__Mid360Packet__Sequence__destroy(mid360_driver__msg__Mid360Packet__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mid360_driver__msg__Mid360Packet__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mid360_driver__msg__Mid360Packet__Sequence__are_equal(const mid360_driver__msg__Mid360Packet__Sequence * lhs, const mid360_driver__msg__Mid360Packet__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mid360_driver__msg__Mid360Packet__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mid360_driver__msg__Mid360Packet__Sequence__copy(
  const mid360_driver__msg__Mid360Packet__Sequence * input,
  mid360_driver__msg__Mid360Packet__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mid360_driver__msg__Mid360Packet);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mid360_driver__msg__Mid360Packet * data =
      (mid360_driver__msg__Mid360Packet *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mid360_driver__msg__Mid360Packet__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mid360_driver__msg__Mid360Packet__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mid360_driver__msg__Mid360Packet__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}

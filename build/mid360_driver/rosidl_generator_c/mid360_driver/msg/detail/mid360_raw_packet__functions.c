// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice
#include "mid360_driver/msg/detail/mid360_raw_packet__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `data`
#include "rosidl_runtime_c/primitives_sequence_functions.h"

bool
mid360_driver__msg__Mid360RawPacket__init(mid360_driver__msg__Mid360RawPacket * msg)
{
  if (!msg) {
    return false;
  }
  // data_type
  // dot_num
  // time_interval
  // udp_cnt
  // frame_cnt
  // time_type
  // livox_timestamp
  // data
  if (!rosidl_runtime_c__uint8__Sequence__init(&msg->data, 0)) {
    mid360_driver__msg__Mid360RawPacket__fini(msg);
    return false;
  }
  return true;
}

void
mid360_driver__msg__Mid360RawPacket__fini(mid360_driver__msg__Mid360RawPacket * msg)
{
  if (!msg) {
    return;
  }
  // data_type
  // dot_num
  // time_interval
  // udp_cnt
  // frame_cnt
  // time_type
  // livox_timestamp
  // data
  rosidl_runtime_c__uint8__Sequence__fini(&msg->data);
}

bool
mid360_driver__msg__Mid360RawPacket__are_equal(const mid360_driver__msg__Mid360RawPacket * lhs, const mid360_driver__msg__Mid360RawPacket * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // data_type
  if (lhs->data_type != rhs->data_type) {
    return false;
  }
  // dot_num
  if (lhs->dot_num != rhs->dot_num) {
    return false;
  }
  // time_interval
  if (lhs->time_interval != rhs->time_interval) {
    return false;
  }
  // udp_cnt
  if (lhs->udp_cnt != rhs->udp_cnt) {
    return false;
  }
  // frame_cnt
  if (lhs->frame_cnt != rhs->frame_cnt) {
    return false;
  }
  // time_type
  if (lhs->time_type != rhs->time_type) {
    return false;
  }
  // livox_timestamp
  for (size_t i = 0; i < 8; ++i) {
    if (lhs->livox_timestamp[i] != rhs->livox_timestamp[i]) {
      return false;
    }
  }
  // data
  if (!rosidl_runtime_c__uint8__Sequence__are_equal(
      &(lhs->data), &(rhs->data)))
  {
    return false;
  }
  return true;
}

bool
mid360_driver__msg__Mid360RawPacket__copy(
  const mid360_driver__msg__Mid360RawPacket * input,
  mid360_driver__msg__Mid360RawPacket * output)
{
  if (!input || !output) {
    return false;
  }
  // data_type
  output->data_type = input->data_type;
  // dot_num
  output->dot_num = input->dot_num;
  // time_interval
  output->time_interval = input->time_interval;
  // udp_cnt
  output->udp_cnt = input->udp_cnt;
  // frame_cnt
  output->frame_cnt = input->frame_cnt;
  // time_type
  output->time_type = input->time_type;
  // livox_timestamp
  for (size_t i = 0; i < 8; ++i) {
    output->livox_timestamp[i] = input->livox_timestamp[i];
  }
  // data
  if (!rosidl_runtime_c__uint8__Sequence__copy(
      &(input->data), &(output->data)))
  {
    return false;
  }
  return true;
}

mid360_driver__msg__Mid360RawPacket *
mid360_driver__msg__Mid360RawPacket__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__Mid360RawPacket * msg = (mid360_driver__msg__Mid360RawPacket *)allocator.allocate(sizeof(mid360_driver__msg__Mid360RawPacket), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mid360_driver__msg__Mid360RawPacket));
  bool success = mid360_driver__msg__Mid360RawPacket__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mid360_driver__msg__Mid360RawPacket__destroy(mid360_driver__msg__Mid360RawPacket * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mid360_driver__msg__Mid360RawPacket__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mid360_driver__msg__Mid360RawPacket__Sequence__init(mid360_driver__msg__Mid360RawPacket__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__Mid360RawPacket * data = NULL;

  if (size) {
    data = (mid360_driver__msg__Mid360RawPacket *)allocator.zero_allocate(size, sizeof(mid360_driver__msg__Mid360RawPacket), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mid360_driver__msg__Mid360RawPacket__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mid360_driver__msg__Mid360RawPacket__fini(&data[i - 1]);
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
mid360_driver__msg__Mid360RawPacket__Sequence__fini(mid360_driver__msg__Mid360RawPacket__Sequence * array)
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
      mid360_driver__msg__Mid360RawPacket__fini(&array->data[i]);
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

mid360_driver__msg__Mid360RawPacket__Sequence *
mid360_driver__msg__Mid360RawPacket__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__Mid360RawPacket__Sequence * array = (mid360_driver__msg__Mid360RawPacket__Sequence *)allocator.allocate(sizeof(mid360_driver__msg__Mid360RawPacket__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mid360_driver__msg__Mid360RawPacket__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mid360_driver__msg__Mid360RawPacket__Sequence__destroy(mid360_driver__msg__Mid360RawPacket__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mid360_driver__msg__Mid360RawPacket__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mid360_driver__msg__Mid360RawPacket__Sequence__are_equal(const mid360_driver__msg__Mid360RawPacket__Sequence * lhs, const mid360_driver__msg__Mid360RawPacket__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mid360_driver__msg__Mid360RawPacket__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mid360_driver__msg__Mid360RawPacket__Sequence__copy(
  const mid360_driver__msg__Mid360RawPacket__Sequence * input,
  mid360_driver__msg__Mid360RawPacket__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mid360_driver__msg__Mid360RawPacket);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mid360_driver__msg__Mid360RawPacket * data =
      (mid360_driver__msg__Mid360RawPacket *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mid360_driver__msg__Mid360RawPacket__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mid360_driver__msg__Mid360RawPacket__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mid360_driver__msg__Mid360RawPacket__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}

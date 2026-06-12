// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from mid360_driver:msg/CustomMsg.idl
// generated code does not contain a copyright notice
#include "mid360_driver/msg/detail/custom_msg__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


// Include directives for member types
// Member `header`
#include "std_msgs/msg/detail/header__functions.h"
// Member `points`
#include "mid360_driver/msg/detail/custom_point__functions.h"

bool
mid360_driver__msg__CustomMsg__init(mid360_driver__msg__CustomMsg * msg)
{
  if (!msg) {
    return false;
  }
  // header
  if (!std_msgs__msg__Header__init(&msg->header)) {
    mid360_driver__msg__CustomMsg__fini(msg);
    return false;
  }
  // timebase
  // point_num
  // lidar_id
  // rsvd
  // points
  if (!mid360_driver__msg__CustomPoint__Sequence__init(&msg->points, 0)) {
    mid360_driver__msg__CustomMsg__fini(msg);
    return false;
  }
  return true;
}

void
mid360_driver__msg__CustomMsg__fini(mid360_driver__msg__CustomMsg * msg)
{
  if (!msg) {
    return;
  }
  // header
  std_msgs__msg__Header__fini(&msg->header);
  // timebase
  // point_num
  // lidar_id
  // rsvd
  // points
  mid360_driver__msg__CustomPoint__Sequence__fini(&msg->points);
}

bool
mid360_driver__msg__CustomMsg__are_equal(const mid360_driver__msg__CustomMsg * lhs, const mid360_driver__msg__CustomMsg * rhs)
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
  // timebase
  if (lhs->timebase != rhs->timebase) {
    return false;
  }
  // point_num
  if (lhs->point_num != rhs->point_num) {
    return false;
  }
  // lidar_id
  if (lhs->lidar_id != rhs->lidar_id) {
    return false;
  }
  // rsvd
  for (size_t i = 0; i < 3; ++i) {
    if (lhs->rsvd[i] != rhs->rsvd[i]) {
      return false;
    }
  }
  // points
  if (!mid360_driver__msg__CustomPoint__Sequence__are_equal(
      &(lhs->points), &(rhs->points)))
  {
    return false;
  }
  return true;
}

bool
mid360_driver__msg__CustomMsg__copy(
  const mid360_driver__msg__CustomMsg * input,
  mid360_driver__msg__CustomMsg * output)
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
  // timebase
  output->timebase = input->timebase;
  // point_num
  output->point_num = input->point_num;
  // lidar_id
  output->lidar_id = input->lidar_id;
  // rsvd
  for (size_t i = 0; i < 3; ++i) {
    output->rsvd[i] = input->rsvd[i];
  }
  // points
  if (!mid360_driver__msg__CustomPoint__Sequence__copy(
      &(input->points), &(output->points)))
  {
    return false;
  }
  return true;
}

mid360_driver__msg__CustomMsg *
mid360_driver__msg__CustomMsg__create(void)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__CustomMsg * msg = (mid360_driver__msg__CustomMsg *)allocator.allocate(sizeof(mid360_driver__msg__CustomMsg), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(mid360_driver__msg__CustomMsg));
  bool success = mid360_driver__msg__CustomMsg__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
mid360_driver__msg__CustomMsg__destroy(mid360_driver__msg__CustomMsg * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    mid360_driver__msg__CustomMsg__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
mid360_driver__msg__CustomMsg__Sequence__init(mid360_driver__msg__CustomMsg__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__CustomMsg * data = NULL;

  if (size) {
    data = (mid360_driver__msg__CustomMsg *)allocator.zero_allocate(size, sizeof(mid360_driver__msg__CustomMsg), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = mid360_driver__msg__CustomMsg__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        mid360_driver__msg__CustomMsg__fini(&data[i - 1]);
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
mid360_driver__msg__CustomMsg__Sequence__fini(mid360_driver__msg__CustomMsg__Sequence * array)
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
      mid360_driver__msg__CustomMsg__fini(&array->data[i]);
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

mid360_driver__msg__CustomMsg__Sequence *
mid360_driver__msg__CustomMsg__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  mid360_driver__msg__CustomMsg__Sequence * array = (mid360_driver__msg__CustomMsg__Sequence *)allocator.allocate(sizeof(mid360_driver__msg__CustomMsg__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = mid360_driver__msg__CustomMsg__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
mid360_driver__msg__CustomMsg__Sequence__destroy(mid360_driver__msg__CustomMsg__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    mid360_driver__msg__CustomMsg__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
mid360_driver__msg__CustomMsg__Sequence__are_equal(const mid360_driver__msg__CustomMsg__Sequence * lhs, const mid360_driver__msg__CustomMsg__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!mid360_driver__msg__CustomMsg__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
mid360_driver__msg__CustomMsg__Sequence__copy(
  const mid360_driver__msg__CustomMsg__Sequence * input,
  mid360_driver__msg__CustomMsg__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(mid360_driver__msg__CustomMsg);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    mid360_driver__msg__CustomMsg * data =
      (mid360_driver__msg__CustomMsg *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!mid360_driver__msg__CustomMsg__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          mid360_driver__msg__CustomMsg__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!mid360_driver__msg__CustomMsg__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}

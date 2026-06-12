// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

// IWYU pragma: private, include "mid360_driver/msg/mid360_raw_packet.h"


#ifndef MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__FUNCTIONS_H_
#define MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/action_type_support_struct.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"
#include "rosidl_runtime_c/type_description/type_description__struct.h"
#include "rosidl_runtime_c/type_description/type_source__struct.h"
#include "rosidl_runtime_c/type_hash.h"
#include "rosidl_runtime_c/visibility_control.h"
#include "mid360_driver/msg/rosidl_generator_c__visibility_control.h"

#include "mid360_driver/msg/detail/mid360_raw_packet__struct.h"

/// Initialize msg/Mid360RawPacket message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * mid360_driver__msg__Mid360RawPacket
 * )) before or use
 * mid360_driver__msg__Mid360RawPacket__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
bool
mid360_driver__msg__Mid360RawPacket__init(mid360_driver__msg__Mid360RawPacket * msg);

/// Finalize msg/Mid360RawPacket message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
void
mid360_driver__msg__Mid360RawPacket__fini(mid360_driver__msg__Mid360RawPacket * msg);

/// Create msg/Mid360RawPacket message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * mid360_driver__msg__Mid360RawPacket__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
mid360_driver__msg__Mid360RawPacket *
mid360_driver__msg__Mid360RawPacket__create(void);

/// Destroy msg/Mid360RawPacket message.
/**
 * It calls
 * mid360_driver__msg__Mid360RawPacket__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
void
mid360_driver__msg__Mid360RawPacket__destroy(mid360_driver__msg__Mid360RawPacket * msg);

/// Check for msg/Mid360RawPacket message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
bool
mid360_driver__msg__Mid360RawPacket__are_equal(const mid360_driver__msg__Mid360RawPacket * lhs, const mid360_driver__msg__Mid360RawPacket * rhs);

/// Copy a msg/Mid360RawPacket message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
bool
mid360_driver__msg__Mid360RawPacket__copy(
  const mid360_driver__msg__Mid360RawPacket * input,
  mid360_driver__msg__Mid360RawPacket * output);

/// Retrieve pointer to the hash of the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
const rosidl_type_hash_t *
mid360_driver__msg__Mid360RawPacket__get_type_hash(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
const rosidl_runtime_c__type_description__TypeDescription *
mid360_driver__msg__Mid360RawPacket__get_type_description(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the single raw source text that defined this type.
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
const rosidl_runtime_c__type_description__TypeSource *
mid360_driver__msg__Mid360RawPacket__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support);

/// Retrieve pointer to the recursive raw sources that defined the description of this type.
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
const rosidl_runtime_c__type_description__TypeSource__Sequence *
mid360_driver__msg__Mid360RawPacket__get_type_description_sources(
  const rosidl_message_type_support_t * type_support);

/// Initialize array of msg/Mid360RawPacket messages.
/**
 * It allocates the memory for the number of elements and calls
 * mid360_driver__msg__Mid360RawPacket__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
bool
mid360_driver__msg__Mid360RawPacket__Sequence__init(mid360_driver__msg__Mid360RawPacket__Sequence * array, size_t size);

/// Finalize array of msg/Mid360RawPacket messages.
/**
 * It calls
 * mid360_driver__msg__Mid360RawPacket__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
void
mid360_driver__msg__Mid360RawPacket__Sequence__fini(mid360_driver__msg__Mid360RawPacket__Sequence * array);

/// Create array of msg/Mid360RawPacket messages.
/**
 * It allocates the memory for the array and calls
 * mid360_driver__msg__Mid360RawPacket__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
mid360_driver__msg__Mid360RawPacket__Sequence *
mid360_driver__msg__Mid360RawPacket__Sequence__create(size_t size);

/// Destroy array of msg/Mid360RawPacket messages.
/**
 * It calls
 * mid360_driver__msg__Mid360RawPacket__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
void
mid360_driver__msg__Mid360RawPacket__Sequence__destroy(mid360_driver__msg__Mid360RawPacket__Sequence * array);

/// Check for msg/Mid360RawPacket message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
bool
mid360_driver__msg__Mid360RawPacket__Sequence__are_equal(const mid360_driver__msg__Mid360RawPacket__Sequence * lhs, const mid360_driver__msg__Mid360RawPacket__Sequence * rhs);

/// Copy an array of msg/Mid360RawPacket messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
bool
mid360_driver__msg__Mid360RawPacket__Sequence__copy(
  const mid360_driver__msg__Mid360RawPacket__Sequence * input,
  mid360_driver__msg__Mid360RawPacket__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // MID360_DRIVER__MSG__DETAIL__MID360_RAW_PACKET__FUNCTIONS_H_

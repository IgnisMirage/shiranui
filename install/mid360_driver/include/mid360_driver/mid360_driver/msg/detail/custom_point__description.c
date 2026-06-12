// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from mid360_driver:msg/CustomPoint.idl
// generated code does not contain a copyright notice

#include "mid360_driver/msg/detail/custom_point__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
const rosidl_type_hash_t *
mid360_driver__msg__CustomPoint__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0xbe, 0xb2, 0x08, 0x70, 0xad, 0x2f, 0xc2, 0x1b,
      0xad, 0xce, 0x16, 0x9d, 0x00, 0xf8, 0x57, 0x21,
      0xa8, 0xd4, 0xa6, 0x32, 0x98, 0x56, 0x4d, 0x95,
      0x43, 0xb9, 0x53, 0x69, 0xcb, 0xe5, 0xf7, 0x44,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char mid360_driver__msg__CustomPoint__TYPE_NAME[] = "mid360_driver/msg/CustomPoint";

// Define type names, field names, and default values
static char mid360_driver__msg__CustomPoint__FIELD_NAME__offset_time[] = "offset_time";
static char mid360_driver__msg__CustomPoint__FIELD_NAME__x[] = "x";
static char mid360_driver__msg__CustomPoint__FIELD_NAME__y[] = "y";
static char mid360_driver__msg__CustomPoint__FIELD_NAME__z[] = "z";
static char mid360_driver__msg__CustomPoint__FIELD_NAME__reflectivity[] = "reflectivity";
static char mid360_driver__msg__CustomPoint__FIELD_NAME__tag[] = "tag";
static char mid360_driver__msg__CustomPoint__FIELD_NAME__line[] = "line";

static rosidl_runtime_c__type_description__Field mid360_driver__msg__CustomPoint__FIELDS[] = {
  {
    {mid360_driver__msg__CustomPoint__FIELD_NAME__offset_time, 11, 11},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT32,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__CustomPoint__FIELD_NAME__x, 1, 1},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__CustomPoint__FIELD_NAME__y, 1, 1},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__CustomPoint__FIELD_NAME__z, 1, 1},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_FLOAT,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__CustomPoint__FIELD_NAME__reflectivity, 12, 12},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__CustomPoint__FIELD_NAME__tag, 3, 3},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__CustomPoint__FIELD_NAME__line, 4, 4},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
mid360_driver__msg__CustomPoint__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {mid360_driver__msg__CustomPoint__TYPE_NAME, 29, 29},
      {mid360_driver__msg__CustomPoint__FIELDS, 7, 7},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "# Livox custom pointcloud format.\n"
  "\n"
  "uint32 offset_time      # offset time relative to the base time\n"
  "float32 x               # X axis, unit:m\n"
  "float32 y               # Y axis, unit:m\n"
  "float32 z               # Z axis, unit:m\n"
  "uint8 reflectivity      # reflectivity, 0~255\n"
  "uint8 tag               # livox tag\n"
  "uint8 line              # laser number in lidar";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
mid360_driver__msg__CustomPoint__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {mid360_driver__msg__CustomPoint__TYPE_NAME, 29, 29},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 352, 352},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
mid360_driver__msg__CustomPoint__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *mid360_driver__msg__CustomPoint__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}

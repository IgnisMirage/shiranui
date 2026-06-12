// generated from rosidl_generator_c/resource/idl__description.c.em
// with input from mid360_driver:msg/Mid360RawPacket.idl
// generated code does not contain a copyright notice

#include "mid360_driver/msg/detail/mid360_raw_packet__functions.h"

ROSIDL_GENERATOR_C_PUBLIC_mid360_driver
const rosidl_type_hash_t *
mid360_driver__msg__Mid360RawPacket__get_type_hash(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_type_hash_t hash = {1, {
      0x59, 0x38, 0x08, 0xa7, 0x75, 0x41, 0x52, 0x39,
      0xd6, 0xc3, 0x9e, 0x87, 0xbc, 0x61, 0xa0, 0x9e,
      0x3e, 0xd6, 0xc7, 0xae, 0x7c, 0x25, 0x3d, 0x87,
      0x6d, 0x75, 0x1d, 0xcb, 0x4f, 0x00, 0x25, 0xfd,
    }};
  return &hash;
}

#include <assert.h>
#include <string.h>

// Include directives for referenced types

// Hashes for external referenced types
#ifndef NDEBUG
#endif

static char mid360_driver__msg__Mid360RawPacket__TYPE_NAME[] = "mid360_driver/msg/Mid360RawPacket";

// Define type names, field names, and default values
static char mid360_driver__msg__Mid360RawPacket__FIELD_NAME__data_type[] = "data_type";
static char mid360_driver__msg__Mid360RawPacket__FIELD_NAME__dot_num[] = "dot_num";
static char mid360_driver__msg__Mid360RawPacket__FIELD_NAME__time_interval[] = "time_interval";
static char mid360_driver__msg__Mid360RawPacket__FIELD_NAME__udp_cnt[] = "udp_cnt";
static char mid360_driver__msg__Mid360RawPacket__FIELD_NAME__frame_cnt[] = "frame_cnt";
static char mid360_driver__msg__Mid360RawPacket__FIELD_NAME__time_type[] = "time_type";
static char mid360_driver__msg__Mid360RawPacket__FIELD_NAME__livox_timestamp[] = "livox_timestamp";
static char mid360_driver__msg__Mid360RawPacket__FIELD_NAME__data[] = "data";

static rosidl_runtime_c__type_description__Field mid360_driver__msg__Mid360RawPacket__FIELDS[] = {
  {
    {mid360_driver__msg__Mid360RawPacket__FIELD_NAME__data_type, 9, 9},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__Mid360RawPacket__FIELD_NAME__dot_num, 7, 7},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT16,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__Mid360RawPacket__FIELD_NAME__time_interval, 13, 13},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT16,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__Mid360RawPacket__FIELD_NAME__udp_cnt, 7, 7},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT16,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__Mid360RawPacket__FIELD_NAME__frame_cnt, 9, 9},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__Mid360RawPacket__FIELD_NAME__time_type, 9, 9},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__Mid360RawPacket__FIELD_NAME__livox_timestamp, 15, 15},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8_ARRAY,
      8,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
  {
    {mid360_driver__msg__Mid360RawPacket__FIELD_NAME__data, 4, 4},
    {
      rosidl_runtime_c__type_description__FieldType__FIELD_TYPE_UINT8_UNBOUNDED_SEQUENCE,
      0,
      0,
      {NULL, 0, 0},
    },
    {NULL, 0, 0},
  },
};

const rosidl_runtime_c__type_description__TypeDescription *
mid360_driver__msg__Mid360RawPacket__get_type_description(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static bool constructed = false;
  static const rosidl_runtime_c__type_description__TypeDescription description = {
    {
      {mid360_driver__msg__Mid360RawPacket__TYPE_NAME, 33, 33},
      {mid360_driver__msg__Mid360RawPacket__FIELDS, 8, 8},
    },
    {NULL, 0, 0},
  };
  if (!constructed) {
    constructed = true;
  }
  return &description;
}

static char toplevel_type_raw_source[] =
  "# One Livox MID-360 point-cloud UDP payload (compact raw form for rosbag recording)\n"
  "\n"
  "uint8 data_type\n"
  "uint16 dot_num\n"
  "uint16 time_interval\n"
  "uint16 udp_cnt\n"
  "uint8 frame_cnt\n"
  "uint8 time_type\n"
  "uint8[8] livox_timestamp\n"
  "uint8[] data";

static char msg_encoding[] = "msg";

// Define all individual source functions

const rosidl_runtime_c__type_description__TypeSource *
mid360_driver__msg__Mid360RawPacket__get_individual_type_description_source(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static const rosidl_runtime_c__type_description__TypeSource source = {
    {mid360_driver__msg__Mid360RawPacket__TYPE_NAME, 33, 33},
    {msg_encoding, 3, 3},
    {toplevel_type_raw_source, 222, 222},
  };
  return &source;
}

const rosidl_runtime_c__type_description__TypeSource__Sequence *
mid360_driver__msg__Mid360RawPacket__get_type_description_sources(
  const rosidl_message_type_support_t * type_support)
{
  (void)type_support;
  static rosidl_runtime_c__type_description__TypeSource sources[1];
  static const rosidl_runtime_c__type_description__TypeSource__Sequence source_sequence = {sources, 1, 1};
  static bool constructed = false;
  if (!constructed) {
    sources[0] = *mid360_driver__msg__Mid360RawPacket__get_individual_type_description_source(NULL),
    constructed = true;
  }
  return &source_sequence;
}

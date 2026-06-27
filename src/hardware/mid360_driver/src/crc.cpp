#include "mid360/crc.hpp"

namespace mid360
{

uint16_t crc16Ccitt(const uint8_t* data, size_t length)
{
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; ++i)
  {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (int bit = 0; bit < 8; ++bit)
    {
      if (crc & 0x8000)
      {
        crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
      }
      else
      {
        crc = static_cast<uint16_t>(crc << 1);
      }
    }
  }
  return crc;
}

uint32_t crc32(const uint8_t* data, size_t length)
{
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; ++i)
  {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit)
    {
      const uint32_t mask = -(crc & 1u);
      crc = (crc >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~crc;
}

}  // namespace mid360

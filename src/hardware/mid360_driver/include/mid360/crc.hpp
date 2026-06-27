#ifndef MID360_CRC_HPP
#define MID360_CRC_HPP

#include <cstddef>
#include <cstdint>

namespace mid360
{

uint16_t crc16Ccitt(const uint8_t* data, size_t length);
uint32_t crc32(const uint8_t* data, size_t length);

}  // namespace mid360

#endif  // MID360_CRC_HPP

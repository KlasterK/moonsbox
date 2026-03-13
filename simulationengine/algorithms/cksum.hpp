#ifndef MOOX_CKSUM_HPP
#define MOOX_CKSUM_HPP

#include <span>
#include <cstdint>

uint32_t crc32_zlib(std::span<const uint8_t> data);

#endif // MOOX_CKSUM_HPP


#pragma once

#include <seastar/core/temporary_buffer.hh>
#include <string>
#include <tuple>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h> // For ntohs (network to host short)

namespace DistributedLogger {

// Decodes a uint16_t from Big Endian
inline static std::tuple<uint16_t, int> DecodeUint16(const seastar::temporary_buffer<char>& packet, size_t offset = 0) {
    if (packet.size() - offset < 2) {
        return {0, -1};
    }

    uint16_t value;
    std::memcpy(&value, packet.get() + offset, 2);
    value = ntohs(value); // Convert from Big Endian to host endianness

    return {value, 2};
}

// Decodes a uint64_t from Big Endian
inline static std::tuple<uint64_t, int> DecodeUint64(const seastar::temporary_buffer<char>& packet, size_t offset = 0) {
    if (packet.size() - offset < 8) {
        return {0, -1};
    }

    uint64_t value;
    std::memcpy(&value, packet.get() + offset, 8);
    
    // Convert 64-bit Big Endian to host using compiler built-in
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    value = __builtin_bswap64(value);
#endif

    return {value, 8};
}

// Decodes a string prefixed by its uint16 length
inline static std::tuple<seastar::sstring, int> DecodeString(const seastar::temporary_buffer<char>& packet, size_t offset = 0) {
    int decoded = 0;

    // 1. Decode string length prefix
    auto [string_length, decoded_this_time] = DecodeUint16(packet, offset);
    decoded += decoded_this_time;

    // 2. Validate that packet has enough remaining bytes for the string contents
    if (packet.size() - offset < static_cast<size_t>(decoded + string_length)) {
        return { "", -1 };
    }

    // 3. Extract string directly from the buffer
    seastar::sstring value(packet.get() + offset + decoded, string_length);
    
    return {value, decoded + string_length};
}

} // namespace decoder


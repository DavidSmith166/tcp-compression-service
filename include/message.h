#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <vector>

namespace Message_Constants {
    static constexpr std::size_t HEADER_SIZE = sizeof(uint32_t) + 2 * sizeof(uint16_t);
    static constexpr std::size_t MESSAGE_SIZE = HEADER_SIZE + 65536;
    static constexpr uint32_t MAGIC_NUMBER = 0x53545259;
} // namespace Message_Constants


struct Header {
    uint32_t magic_number = Message_Constants::MAGIC_NUMBER;
    uint16_t payload_length;
    uint16_t code;
};

struct Message {
    Header header;
    std::vector<uint8_t> payload;
};

#endif // MESSAGE_H
#ifndef message_def
#define message_def

#include <cstdint>
#include <vector>

struct Header {
    uint32_t magic_number = 0x53545259;
    uint16_t payload_length;
    uint16_t code;
};

struct Message {
    Header header;
    std::vector<uint8_t> payload;
};

#endif // message_def
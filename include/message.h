#ifndef message_def
#define message_def

#include <cstdint>
#include <vector>

enum class Request_Code: uint16_t {
    PING = 1,
    GET_STATS = 2,
    RESET_STATS = 3,
    COMPRESS = 4
};

enum class Status_Code: uint16_t {
    OK = 0,
    UNKNOWN_ERROR = 1,
    TOO_LARGE = 2,
    UNSUPPORTED_TYPE = 3,
};

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
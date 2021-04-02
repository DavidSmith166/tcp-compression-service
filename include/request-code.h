#ifndef requests_def
#define requests_def

#include <cstdint>

enum class Request_Code: uint16_t {
    PING = 1,
    GET_STATS = 2,
    RESET_STATS = 3,
    COMPRESS = 4
};


#endif // requests_def
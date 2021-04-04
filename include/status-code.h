#ifndef STATUS_CODE_H
#define STATUS_CODE_H

#include <cstdint>

enum class Status_Code: uint16_t {
    OK = 0,
    UNKNOWN_ERROR = 1,
    TOO_LARGE = 2,
    UNSUPPORTED_TYPE = 3,
};

#endif // STATUS_CODE_H
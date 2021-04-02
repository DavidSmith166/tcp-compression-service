#ifndef status_code_def
#define status_code_def

#include <cstdint>

enum class Status_Code: uint16_t {
    OK = 0,
    UNKNOWN_ERROR = 1,
    TOO_LARGE = 2,
    UNSUPPORTED_TYPE = 3,
};

#endif // status_code_def
#ifndef service_def
#define service_def

#include <cstdint>

class Service {
    public:
        Service();

    private:
        uint32_t total_bytes_recieved;
        uint32_t total_bytes_sent;
        uint32_t compression_ratio;
};

#endif // service_def
#ifndef HELPERS_H
#define HELPERS_H

#include <cstdint>
#include <vector>

namespace Helpers {

    template<typename T>
    void add_bytes_to_payload(T* value_ptr, std::vector<char>* payload);

} // namespace Helpers

#endif // HELPERS_H
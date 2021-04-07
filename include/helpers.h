#ifndef HELPERS_H
#define HELPERS_H

#include <cstdint>
#include <vector>

namespace Helpers {
    
    template<typename T>
    void add_bytes_to_payload(T* value_ptr, std::vector<char>* payload) {

        auto value_as_byte_arr = static_cast<char*>(static_cast<void*>(value_ptr)); 
        
        for (std::size_t i = 0; i < sizeof(T); i++) {
            payload->push_back(value_as_byte_arr[i]);
        }

    }

} // namespace Helpers

#endif // HELPERS_H
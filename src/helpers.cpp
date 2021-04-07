#include <helpers.h>

template<typename T>
void Helpers::add_bytes_to_payload(T* value_ptr, std::vector<char>* payload) {
    auto value_as_byte_arr = static_cast<char*>(static_cast<void*>(value_ptr));
    for (std::size_t i = 0; i < sizeof(T); i++) {
        payload->push_back(value_as_byte_arr[i]);
    }
}
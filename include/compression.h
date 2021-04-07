#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <cstdint>
#include <cmath>
#include <message.h>
#include <optional>
#include <vector>
namespace Compression {

    static constexpr std::size_t COUNT_MAX_DIGITS = 5;
    static constexpr std::size_t COUNT_BUFFER_SIZE = COUNT_MAX_DIGITS + 1;

    std::optional<std::vector<char>> compress(const std::vector<char>& input);
    
} // namespace Compression

#endif // COMPRESSION_H
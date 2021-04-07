#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <cstdint>
#include <cmath>
#include <message.h>
#include <optional>
#include <vector>
namespace Compression {

    std::optional<std::vector<char>> compress(const std::vector<char>& input);

    static constexpr std::size_t COUNT_MAX_DIGITS = 5;
    static constexpr std::size_t COUNT_BUFFER_SIZE = COUNT_MAX_DIGITS + 1;

    using Buffer = std::array<char, COUNT_BUFFER_SIZE>;    

} // namespace Compression

#endif // COMPRESSION_H
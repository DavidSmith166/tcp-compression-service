#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <cstdint>
#include <cmath>
#include <message.h>
#include <optional>
#include <vector>
namespace Compression {

    std::optional<std::vector<char>> compress(const std::vector<char>& input);

} // namespace Compression

#endif // COMPRESSION_H
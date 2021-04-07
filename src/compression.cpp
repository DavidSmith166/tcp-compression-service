#include <cctype>
#include <compression.h>
#include <helpers.h>

void write_char(char c, std::size_t count, std::vector<char>* output) {

    if (count > 2) {

        std::size_t nbo_count = htonl(count);
        Helpers::add_bytes_to_payload(&nbo_count, output);
        output->push_back(c);

    } else {

        for (std::size_t i = 0; i < count; i++) {
            output->push_back(c);
        }

    }
}

std::optional<std::vector<char>> Compression::compress(const std::vector<char>& input) {

    if (input.empty()) {
        return std::vector<char>();
    }

    std::vector<char> output;
    char current_char = input[0];
    std::size_t count = 0;
    
    for (const char c: input) {

        if (!islower(c)) {
            return std::nullopt;
        }

        if (c == current_char) {
            ++count;
        } else {
            write_char(current_char, count, &output);
        }

    } 

    write_char(current_char, count, &output);

    return output;
}
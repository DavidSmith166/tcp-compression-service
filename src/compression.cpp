#include <cctype>
#include <compression.h>

void write_char(char c, std::size_t count, 
                std::array<char, Compression::COUNT_BUFFER_SIZE>* count_buffer,
                std::vector<char>* output) {

    if (count > 2) {

        sprintf(count_buffer->data(), "%lu", count);
        for (std::size_t i = 0; count_buffer->operator[](i); i++) {
            output->push_back(count_buffer->operator[](i));
        }
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
    std::array<char, COUNT_BUFFER_SIZE> count_buffer;
    char current_char = input[0];
    std::size_t count = 0;
    
    for (const char c: input) {

        if (!islower(c)) {
            return std::nullopt;
        }

        if (c == current_char) {
            ++count;
        } else {
            write_char(current_char, count, &count_buffer, &output);
        }

    } 

    write_char(current_char, count, &count_buffer, &output);

    return output;
}
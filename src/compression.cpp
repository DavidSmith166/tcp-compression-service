#include <cctype>
#include <cstring>
#include <compression.h>
#include <helpers.h>

void write_char(char c, std::size_t count, Compression::Buffer* count_buffer, std::vector<char>* output) {

    if (count > 2) {        

        sprintf(count_buffer->data(), "%lu", count);
        for (std::size_t i = 0; i < strlen(count_buffer->data()); i++) {
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
    Compression::Buffer count_buffer;

    std::size_t count = 0;
    char current_char = input[0];
    
    for (const char c: input) {

        if (!islower(c)) {
            return std::nullopt;
        }

        if (c == current_char) {
            ++count;
        } else {
            write_char(current_char, count, &count_buffer, &output);
            count = 1;
            current_char = c;
        }

    } 

    write_char(current_char, count, &count_buffer, &output);

    return output;
}
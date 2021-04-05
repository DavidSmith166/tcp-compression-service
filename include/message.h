#ifndef MESSAGE_H
#define MESSAGE_H

#include <algorithm>
#include <cstdint>
#include <netinet/in.h>
#include <status-code.h>
#include <vector>

namespace Message_Constants {
    static constexpr std::size_t HEADER_SIZE = sizeof(uint32_t) + 2 * sizeof(uint16_t);
    static constexpr std::size_t PAYLOAD_SIZE = 65536;
    static constexpr std::size_t MESSAGE_SIZE = HEADER_SIZE + PAYLOAD_SIZE;
    static constexpr uint32_t MAGIC_NUMBER = 0x53545259;
} // namespace Message_Constants
struct Header {

    void set_net_order() {
        this->magic_number = htonl(this->magic_number);
        this->payload_length = htons(this->payload_length);
        this->code = htons(this->code);
    }

    void set_host_order() {
        this->magic_number = ntohl(this->magic_number);
        this->payload_length = ntohs(this->payload_length);
        this->code = ntohs(this->code);
    }

    uint32_t magic_number = Message_Constants::MAGIC_NUMBER;
    uint16_t payload_length = 0;
    uint16_t code = static_cast<uint16_t>(Status_Code::OK);
};

struct Message {

    explicit Message(Header h): header(h) {} 
    
    Header header;
    std::vector<char> payload;
};

struct Network_Order_Message {

    explicit Network_Order_Message(Header h): header(h) {} 

    Header header;
    std::vector<char> payload;
};

#endif // MESSAGE_H
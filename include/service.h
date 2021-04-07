#ifndef SERVICE_H
#define SERVICE_H

#include <cstdint>
#include <condition_variable>
#include <message.h> 
#include <mutex>
#include <netinet/in.h>
#include <queue>
#include <raii_fd.h>
#include <status-code.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef VERBOSE
#   include <iostream>
#   define IF_VERBOSE(...) __VA_ARGS__
#else
#   define IF_VERBOSE(...)
#endif // VERBOSE

struct Job {

    Job() = default;

    Message msg;
    RAII_FD clientfd;
};

namespace Service_Constants
{

    static constexpr std::size_t DEFAULT_NUM_LISTENERS = 2;
    static constexpr std::size_t DEFAULT_NUM_WORKERS = 4;
    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 32;

    static constexpr uint16_t DEFAULT_PORT = 8000;
    static constexpr int DEFAULT_BACKLOG_SIZE = 10;

    static constexpr int MAX_EPOLL_EVENTS = 10;

    static constexpr std::size_t RECV_BUFFER_SIZE = Message_Constants::MESSAGE_SIZE;

    static constexpr uint16_t GET_STATS_PAYLOAD_SIZE = 2 * sizeof(uint32_t) + 1;

    using Buffer = std::array<uint8_t, Message_Constants::PAYLOAD_SIZE>;
    
} // namespace Service_Constants


class Service {

    public:

        Service();
        Service(size_t num_listeners, size_t num_workers,  uint16_t port, int backlog_size);

        void start();

    private:

        // Creates and configures server socket for the service
        std::pair<RAII_FD, struct sockaddr_in> create_server_socket();
        // Constructs a message with <error_code> and empty payload then calls respond
        void respond_with_error(int clientfd, Status_Code error_code);
        // Serializes and transmits header and payload defined in msg
        void respond(int clientfd, const Network_Order_Message& msg);

        // Thread function, waits on epoll and reads message from clients
        void accept_requests();

        // Attempts to read n bytes from clientfd to buffer, returns 0 on success
        bool recv_bytes(int clientfd, Service_Constants::Buffer* buffer, std::size_t n);
        // Attempts to create a Header by reading from clientfd, returns empty optional on failure
        std::optional<Header> create_header(int clientfd, Service_Constants::Buffer* buffer);
        // Attempts to create a Message by reading from clientfd, returns an empty optional on failure
        std::optional<Message> create_message(int clientfd, Header h, Service_Constants::Buffer* buffer);
        // Packages clientfd and msg into job stuct and enqueues it. This transfers ownership of clientfd
        void publish_message(RAII_FD clientfd, Message msg);
        // Attempts to read and publish a message from clientfd
        void handle_client(RAII_FD clientfd, Service_Constants::Buffer* buffer);

        // Thread function, waits on requests queue and services requests based on type
        void process_requests();

        // Responds to client with empty message and OK status
        void ping(const Job& job);
        // Responds to client with bytes sent/recieved and compression ratio
        void get_stats(const Job& job);
        // Resets bytes send/recieved and compression ratio to zero
        void reset_stats(const Job& job);
        // Responds to client with compressed version of their message payload
        void compress(const Job& job);

        std::vector<std::thread> listeners;
        std::vector<std::thread> workers;
        std::queue<Job> requests;
        std::mutex requests_lock;
        std::condition_variable waiting_workers;

        uint32_t total_bytes_recieved;
        uint32_t total_bytes_sent;
        uint8_t compression_ratio;
        std::mutex stats_lock;

        RAII_FD serverfd;
        RAII_FD epollfd;
        struct sockaddr_in addr;

        uint16_t port;
        int backlog_size;
        std::size_t num_listeners;
        std::size_t num_workers;

};

#endif // SERVICE_H
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
#endif

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
} // namespace Service_Constants


class Service {

    public:

        Service();
        Service(size_t num_listeners, size_t num_workers,  uint16_t port, int backlog_size);

        void start();

    private:

        std::pair<RAII_FD, struct sockaddr_in> create_server_socket();
        void respond_with_error(int clientfd, Status_Code error_code);
        void respond(int clientfd, const Network_Order_Message& msg);

        void accept_requests();

        bool recv_bytes(int clientfd, 
                        std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer,
                        std::size_t n);
        std::optional<Header> create_header(int clientfd, std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer);
        std::optional<Message> create_message(int clientfd, const Header& h,std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer);

        void publish_message(RAII_FD clientfd, Message msg);
        void handle_client(RAII_FD clientfd,  
						   std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer);

        void process_requests();

        void ping(const Job& job);
        void get_stats(const Job& job);
        void reset_stats(const Job& job);
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
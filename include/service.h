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

struct Job {
    Message msg;
    RAII_FD clientfd;
};

namespace Service_Constants
{
    static constexpr std::size_t DEFAULT_NUM_LISTENERS = 2;
    static constexpr std::size_t DEFAULT_NUM_WORKERS = 4;
    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 32;

    static constexpr int DEFAULT_PORT = 8000;
    static constexpr int DEFAULT_BACKLOG_SIZE = 10;

    static constexpr int MAX_EPOLL_EVENTS = 10;

    static constexpr std::size_t RECV_BUFFER_SIZE = Message_Constants::MESSAGE_SIZE;
} // namespace Service_Constants


class Service {
    public:
        Service();

        Service(size_t num_listeners, size_t num_workers,  int port, int backlog_size);

    private:

        void accept_requests();
        void process_requests();

        std::pair<RAII_FD, struct sockaddr_in> create_server_socket();

        std::optional<Message> create_message(int clientfd, std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer);
        void publish_message(RAII_FD clientfd, std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer);
        void respond_with_error(int clientfd, Status_Code error_code);
        void handle_client(int epollfd, int clientfd,  
						   std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer,
						   std::unordered_map<int, RAII_FD>* open_fds);

        std::vector<std::thread> listeners;
        std::vector<std::thread> workers;
        std::queue<Job> requests;
        std::mutex requests_lock;
        std::condition_variable waiting_workers;

        uint32_t total_bytes_recieved;
        uint32_t total_bytes_sent;
        uint32_t compression_ratio;

        RAII_FD serverfd;
        RAII_FD epollfd;
        struct sockaddr_in addr;

        int port;
        int backlog_size;
};

#endif // SERVICE_H
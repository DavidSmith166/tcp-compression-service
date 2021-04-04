#ifndef SERVICE_H
#define SERVICE_H

#include <cstdint>
#include <map>
#include <message.h> 
#include <queue>
#include <raii_fd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <vector>

struct Job {
    size_t buffer_slot;
    RAII_FD socket;
};

namespace Service_Constants
{
    static constexpr std::size_t DEFAULT_NUM_LISTENERS = 2;
    static constexpr std::size_t DEFAULT_NUM_WORKERS = 4;
    static constexpr std::size_t DEFAULT_BUFFER_SIZE = 32;

    static constexpr int DEFAULT_PORT = 8000;
    static constexpr int DEFAULT_BACKLOG_SIZE = 10;

    static constexpr int MAX_EPOLL_EVENTS = 10;

    static constexpr std::size_t RECV_BUFFER_SIZE = sizeof(Message);
} // namespace Service_Constants


class Service {
    public:
        Service();

        Service(size_t num_listeners, size_t num_workers,  int port, int backlog_size);

    private:

        void accept_requests();
        void process_requests();

        std::pair<int, struct sockaddr_in> create_server_socket();

        bool handle_client(int epollfd, int clientfd, 
                           std::map<int, std::vector<uint8_t>>* cache, 
						   std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* buffer,
						   std::map<int, RAII_FD>* open_fds);

        void add_client(int epollfd, int serverfd, 
                        const sockaddr_in& addr, epoll_event* epoll_ev, 
						std::map<int, RAII_FD>* open_fds);

        std::vector<Message> message_buffer;
        std::vector<std::size_t> free_slots;

        std::vector<std::thread> listeners;
        std::vector<std::thread> workers;
        std::queue<Job> requests;

        uint32_t total_bytes_recieved;
        uint32_t total_bytes_sent;
        uint32_t compression_ratio;

        int port;
        int backlog_size;
};

#endif // SERVICE_H
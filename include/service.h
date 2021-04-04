#ifndef SERVICE_H
#define SERVICE_H

#include <cstdint>
#include <message.h> 
#include <queue>
#include <sys/socket.h>
#include <thread>
#include <vector>

struct Job {
    size_t buffer_slot;
    int socket;
};

class Service {
    public:
        Service();

        Service(size_t num_listeners, size_t num_workers,  int port, int backlog_size);

    private:

        void accept_requests();
        void process_requests();

        std::pair<int, struct sockaddr_in> create_server_socket();
        void handle_client(int clientfd);
        void add_client(int epollfd, int serverfd, const sockaddr_in& addr, epoll_event& epoll_ev);

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
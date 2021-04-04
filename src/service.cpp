#include <service.h>
#include <netinet/in.h>
#include <algorithm>
#include <cassert>
#include <list>

#ifdef VERBOSE
#include <iostream>
#endif // VERBOSE

Service::Service() : Service(Service_Constants::DEFAULT_NUM_LISTENERS, 
							 Service_Constants::DEFAULT_NUM_WORKERS, 
							 Service_Constants::DEFAULT_PORT,
							 Service_Constants::DEFAULT_BACKLOG_SIZE) {}

Service::Service(std::size_t num_listners, std::size_t num_workers, int port, int backlog_size):
	port(port), backlog_size(backlog_size) {
	this->listeners.reserve(num_listners);
	for (std::size_t i = 0; i < num_listners; i++) {
		this->listeners[i] = std::thread(accept_requests);
	}

	this->workers.reserve(num_workers);
	for (std::size_t i = 0; i < num_workers; i++) {
		this->workers[i] = std::thread(process_requests);
	}
}

std::pair<int, sockaddr_in> Service::create_server_socket() {
	int serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd == -1) {
		throw std::runtime_error("Socket creation failed");
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htonl(this->port);

	int REUSE_TRUE = 1;
    if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEPORT, &REUSE_TRUE, sizeof(REUSE_TRUE))) {
		throw std::runtime_error("Sockopt failed");
	}

    if (bind(serverfd, (sockaddr *) &addr, sizeof(addr))) {
		throw std::runtime_error("Bind failed");
	}

	if (listen(serverfd, this->backlog_size)) {
		throw std::runtime_error("Listen failed");
	}

	return std::pair<int, sockaddr_in> {serverfd, addr};
}

void Service::add_client(int epollfd, int serverfd, 
                        const sockaddr_in& addr, epoll_event* epoll_ev, 
						std::map<int, RAII_FD>* open_fds) {

	int addrlen = sizeof(addr);

	int new_clientfd = accept(serverfd, (sockaddr*) &addr, (socklen_t*) &addrlen);
	if (new_clientfd == -1) {
		throw std::runtime_error("Accept connection failed");
	}

	open_fds->emplace(new_clientfd, new_clientfd);

	epoll_ev->data.fd = new_clientfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_clientfd, epoll_ev)) {
		throw std::runtime_error("Epoll CTL add new client failed");
	}

}

bool Service::handle_client(int epollfd, int clientfd, 
                            std::map<int, std::vector<uint8_t>>* cache, 
							std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* buffer,
							std::map<int, RAII_FD>* open_fds) {

	ssize_t num_bytes = recv(clientfd, buffer->data(), Service_Constants::RECV_BUFFER_SIZE, 0);
	this->total_bytes_recieved+= num_bytes;

	switch (num_bytes)
	{
	case -1:
		// recv failed, respond with error
		// TODO
		cache->erase(clientfd);
		open_fds->erase(clientfd);
		break;
	
	default:
		break;
	}


}
 
void Service::accept_requests() {

	std::map<int, RAII_FD> open_fds;

	const auto [serverfd, addr] = create_server_socket();  
	open_fds.emplace(serverfd, serverfd);

	epoll_event epoll_ev, epoll_events[Service_Constants::MAX_EPOLL_EVENTS];

	int epollfd = epoll_create1(EPOLLEXCLUSIVE);
	if (epollfd == -1) {
		throw std::runtime_error("Epoll create failed");
	}
	open_fds.emplace(epollfd, epollfd);

	epoll_ev.events = EPOLLIN;
	epoll_ev.data.fd = serverfd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &epoll_ev)) {
		throw std::runtime_error("Epoll CTL: listener socket failed");
	}

	int num_fds;
	std::map<int, std::vector<uint8_t>> cache;
	std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE> buffer;
	while (true) {

		num_fds = epoll_wait(epollfd, epoll_events, Service_Constants::MAX_EPOLL_EVENTS, -1);
		if (num_fds == -1) {
			throw std::runtime_error("Epoll wait failed");
		}

		for (int i = 0; i < num_fds; i++) {
			if (epoll_events[i].data.fd == serverfd) {

				add_client(epollfd, serverfd, addr, &epoll_ev, &open_fds);

			} else {

				handle_client(epollfd, epoll_events[i].data.fd, &cache, &buffer, &open_fds);

			}
		}
	}

}

void Service::process_requests() {
	
}
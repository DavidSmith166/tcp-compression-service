#include <service.h>
#include <algorithm>
#include <cassert>
#include <cstring>

#ifdef VERBOSE
#include <iostream>
#endif // VERBOSE

Service::Service() : Service(Service_Constants::DEFAULT_NUM_LISTENERS, 
							 Service_Constants::DEFAULT_NUM_WORKERS, 
							 Service_Constants::DEFAULT_PORT,
							 Service_Constants::DEFAULT_BACKLOG_SIZE) {}

Service::Service(std::size_t num_listeners, std::size_t num_workers, int port, int backlog_size):
	port(port), backlog_size(backlog_size) {

	auto [serverfd_temp, addr] = this->create_server_socket();
	this->serverfd = std::move(serverfd_temp);
	this->addr = addr;

	int epollfd_raw = epoll_create1(EPOLLEXCLUSIVE);
	if (epollfd_raw == -1) {
		throw std::runtime_error("Epoll create failed");
	}
	this->epollfd = RAII_FD(epollfd_raw);

	epoll_event epoll_ev;
	epoll_ev.events = EPOLLIN;
	epoll_ev.data.fd = this->serverfd.get();

	if (epoll_ctl(epollfd_raw, EPOLL_CTL_ADD, serverfd.get(), &epoll_ev)) {
		throw std::runtime_error("Epoll CTL: listener socket failed");
	}

	this->listeners.reserve(num_listeners);
	for (std::size_t i = 0; i < num_listeners; i++) {
		this->listeners[i] = std::thread(&Service::accept_requests, this);
	}

	this->workers.reserve(num_workers);
	for (std::size_t i = 0; i < num_workers; i++) {
		this->workers[i] = std::thread(&Service::process_requests, this);
	}
}

std::pair<RAII_FD, struct sockaddr_in> Service::create_server_socket() {
	int serverfd_raw = socket(AF_INET, SOCK_STREAM, 0);
	if (serverfd_raw == -1) {
		throw std::runtime_error("Socket creation failed");
	}

	RAII_FD serverfd(serverfd_raw);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htonl(this->port);

	int REUSE_TRUE = 1;
    if (setsockopt(serverfd.get(), SOL_SOCKET, SO_REUSEPORT, &REUSE_TRUE, sizeof(REUSE_TRUE)) == -1) {
		throw std::runtime_error("Sockopt failed");
	}

    if (bind(serverfd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
		throw std::runtime_error("Bind failed");
	}

	if (listen(serverfd.get(), this->backlog_size) == -1) {
		throw std::runtime_error("Listen failed");
	}

	return std::pair<RAII_FD, struct sockaddr_in> {std::move(serverfd), addr};
}

void Service::respond_with_error(int clientfd, Status_Code error_code) {
	std::array<uint8_t, Message_Constants::HEADER_SIZE> write_buffer;

	Header h;
	h.magic_number = htonl(h.magic_number);
	h.payload_length = htons(0);
    h.code = htons(static_cast<uint16_t>(error_code));

	uint8_t* write_head = write_buffer.data();

	memcpy(write_head, &h.magic_number, sizeof(h.magic_number));
	write_head += sizeof(h.magic_number);

	memcpy(write_head, &h.payload_length, sizeof(h.payload_length));
	write_head += sizeof(h.payload_length);

	memcpy(write_head, &h.code, sizeof(h.code));

	ssize_t bytes_sent = 0;
	if ((bytes_sent = write(clientfd, write_buffer.data(), Message_Constants::HEADER_SIZE)) != -1) {
		this->total_bytes_sent += bytes_sent;
	}
}
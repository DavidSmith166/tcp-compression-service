#include <algorithm>
#include <cassert>
#include <cstring>
#include <service.h>
#include <stdio.h>

Service::Service() : Service(Service_Constants::DEFAULT_NUM_LISTENERS, 
							 Service_Constants::DEFAULT_NUM_WORKERS, 
							 Service_Constants::DEFAULT_PORT,
							 Service_Constants::DEFAULT_BACKLOG_SIZE) {}

Service::Service(std::size_t num_listeners, std::size_t num_workers, uint16_t port, int backlog_size):
	port(port), backlog_size(backlog_size), num_listeners(num_listeners), num_workers(num_workers) {

	auto [serverfd_temp, addr] = this->create_server_socket();
	this->serverfd = std::move(serverfd_temp);
	this->addr = addr;

	int epollfd_raw = epoll_create(1);
	if (epollfd_raw == -1) {
		fprintf(stderr, "Errno: %d\n", errno);
		throw std::runtime_error("Epoll create failed");
	}
	this->epollfd = RAII_FD(epollfd_raw);

	epoll_event epoll_ev;
	epoll_ev.events = EPOLLIN | EPOLLEXCLUSIVE;
	epoll_ev.data.fd = this->serverfd.get();

	if (epoll_ctl(epollfd_raw, EPOLL_CTL_ADD, serverfd.get(), &epoll_ev)) {
		throw std::runtime_error("Epoll CTL: listener socket failed");
	}

}

void Service::start() {

	this->listeners.reserve(num_listeners);
	for (std::size_t i = 0; i < num_listeners; i++) {
		this->listeners[i] = std::thread(&Service::accept_requests, this);
	}

	this->workers.reserve(num_workers);
	for (std::size_t i = 0; i < num_workers; i++) {
		this->workers[i] = std::thread(&Service::process_requests, this);
	}
	
	for (std::size_t i = 0; i < num_listeners; i++) {
		this->listeners[i].join();
	}

	for (std::size_t i = 0; i < num_workers; i++) {
		this->workers[i].join();
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
	addr.sin_port = htons(this->port);

	// int REUSE_TRUE = 1;
    // if (setsockopt(serverfd.get(), SOL_SOCKET, SO_REUSEPORT, &REUSE_TRUE, sizeof(REUSE_TRUE)) == -1) {
	// 	throw std::runtime_error("Sockopt failed");
	// }

    if (bind(serverfd.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
		throw std::runtime_error("Bind failed");
	}

	if (listen(serverfd.get(), this->backlog_size) == -1) {
		throw std::runtime_error("Listen failed");
	}

	return std::pair<RAII_FD, struct sockaddr_in> {std::move(serverfd), addr};
}

void Service::respond_with_error(int clientfd, Status_Code error_code) {
	PRINT("Responding to client with error");

	assert(clientfd != -1);

	Header h;
	h.payload_length = 0;
    h.code = static_cast<uint16_t>(error_code);
	h.set_net_order();

	Network_Order_Message net_msg(h);
	this->respond(clientfd, net_msg);
	
}

void Service::respond(int clientfd, const Network_Order_Message& msg) {
	std::cout << "Responding to client " << clientfd << std::endl;

	assert(clientfd != -1);

	std::array<uint8_t, Message_Constants::HEADER_SIZE> write_buffer;

	uint8_t* write_head = write_buffer.data();

	memcpy(write_head, &msg.header.magic_number, sizeof(msg.header.magic_number));
	write_head += sizeof(msg.header.magic_number);

	memcpy(write_head, &msg.header.payload_length, sizeof(msg.header.payload_length));
	write_head += sizeof(msg.header.payload_length);

	memcpy(write_head, &msg.header.code, sizeof(msg.header.code));

	for (const uint8_t byte: write_buffer)
		fprintf(stderr, "%#02x ", byte);
	fprintf(stderr, "\n");

	ssize_t bytes_sent = 0;
	while ((bytes_sent = send(clientfd, write_buffer.data() + bytes_sent, sizeof(write_buffer) - bytes_sent, 0)) != -1) {
		this->stats_lock.lock();
		std::cout << "bytes sent " << bytes_sent << std::endl;
		this->total_bytes_sent += bytes_sent;
		this->stats_lock.unlock();

		if (bytes_sent == Message_Constants::HEADER_SIZE) {
			break;
		}
	}

	if (bytes_sent < sizeof(write_buffer)) {
		PRINT("Failed to write");
	}
}
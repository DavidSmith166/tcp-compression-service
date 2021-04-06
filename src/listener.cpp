#include <algorithm>
#include <cassert>
#include <list>
#include <message.h>
#include <netinet/in.h>
#include <optional>
#include <request-code.h>
#include <service.h>
#include <status-code.h>
#include <sys/ioctl.h>

void add_client(int epollfd, int serverfd, 
                struct sockaddr_in* addr, epoll_event* epoll_ev) {

	int addrlen = sizeof(struct sockaddr_in);

	int new_clientfd = accept(serverfd, reinterpret_cast<sockaddr*>(addr), reinterpret_cast<socklen_t*>(&addrlen));
	if (new_clientfd == -1) {
		throw std::runtime_error("Accept connection failed");
	}

	epoll_ev->events = EPOLLIN | EPOLLEXCLUSIVE;
	epoll_ev->data.fd = new_clientfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_clientfd, epoll_ev)) {
		throw std::runtime_error("Epoll CTL add new client failed");
	}

}

std::optional<Message> Service::create_message(int clientfd, const Header& h,std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer) {

	Message msg(h);

	if (h.payload_length > 0) {
		if (h.code != static_cast<uint16_t>(Request_Code::COMPRESS)) {
			this->respond_with_error(clientfd, Status_Code::UNSUPPORTED_TYPE);
			return std::nullopt;
		}
	}

	if (this->recv_bytes(clientfd, recv_buffer, h.payload_length)) {
		return std::nullopt;
	}

	auto start = recv_buffer->begin();
	auto end = start + h.payload_length;
	msg.payload.assign(start, end);

	PRINT("Constructed message");

	return msg;
}

std::optional<Header> Service::create_header(int clientfd, std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer) {
	assert(clientfd != -1);

	Header h;
	
	uint8_t* read_head = recv_buffer->data();
	
	h.magic_number = *reinterpret_cast<uint32_t*>(read_head);
	read_head += sizeof(h.magic_number);
	h.payload_length = *reinterpret_cast<uint16_t*>(read_head);
	read_head += sizeof(h.payload_length);
	h.code = *reinterpret_cast<uint16_t*>(read_head);
	h.set_host_order();

	// TODO
	std::cout << h.magic_number << " " << h.payload_length << " " << h.code << std::endl;

	if (h.magic_number != Message_Constants::MAGIC_NUMBER) {
		this->respond_with_error(clientfd, Status_Code::UNKNOWN_ERROR);
		return std::nullopt;
	}

	if (h.code < 1 or h.code > 4) {
		this->respond_with_error(clientfd, Status_Code::UNSUPPORTED_TYPE);
		return std::nullopt;
	}

	return h;

}

void Service::publish_message(RAII_FD clientfd, std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer) {
	assert(clientfd.get() != -1);

	auto msg_opt = this->create_message(clientfd.get(), recv_buffer);
	if (!msg_opt.has_value()) {
		return;
	}

	// TODO
	std::cout << "Pushed " << clientfd.get() << std::endl;
	assert(clientfd.get() != -1);

	Job job = {std::move(msg_opt.value()), std::move(clientfd)};

	std::lock_guard<std::mutex> guard(this->requests_lock);
	this->requests.emplace(std::move(job));
	this->waiting_workers.notify_one();
	PRINT("Pushed job to queue");
}

bool Service::recv_bytes(int clientfd, 
						 std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer,
						 std::size_t n) {
	ssize_t num_bytes = 0;
	std::size_t read_bytes = 0;

	while (read_bytes < n) {
		num_bytes = recv(clientfd, 
				    recv_buffer->data() + read_bytes, 
					Service_Constants::RECV_BUFFER_SIZE - read_bytes, 
					0);
		if (num_bytes == -1) {
			this->respond_with_error(clientfd, Status_Code::UNKNOWN_ERROR);
			return true;
		} else if (num_bytes == 0) {
			// the client closed the connection
			return true;
		}

		this->stats_lock.lock();
		this->total_bytes_recieved += num_bytes;
		this->stats_lock.unlock();
	}

	return false;
}

void Service::handle_client(int epollfd, RAII_FD clientfd, 
							std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer) {

	assert(clientfd.get() != -1);

	if (this->recv_bytes(clientfd.get(), recv_buffer, Message_Constants::HEADER_SIZE)) {
		return;
	}

	auto header_opt = this->create_header(clientfd.get(), recv_buffer);
	if (!header_opt.has_value()) {
		return;
	}
	Header h = header_opt.value();

	auto msg_opt = this->create_message(clientfd.get(), h, recv_buffer);
	if (!msg_opt.has_value()) {
		return;
	}
	Message msg = msg_opt.value();

	PRINT("Recieved message");

	this->publish_message(std::move(clientfd), recv_buffer);

}
 
void Service::accept_requests() {

	epoll_event epoll_ev, epoll_events[Service_Constants::MAX_EPOLL_EVENTS];

	int num_fds = 0;
	std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE> recv_buffer;
	while (true) {

		num_fds = epoll_wait(this->epollfd.get(), epoll_events, Service_Constants::MAX_EPOLL_EVENTS, -1);
		if (num_fds == -1) {
			throw std::runtime_error("Epoll wait failed");
		}

		for (int i = 0; i < num_fds; i++) {
			if (epoll_events[i].data.fd == this->serverfd.get()) {

				PRINT("Add client");
				add_client(this->epollfd.get(), this->serverfd.get(), &this->addr, &epoll_ev);

			} else {

				PRINT("Handle client");
				assert(epoll_events[i].data.fd != -1);
				epoll_ctl(this->epollfd.get(), EPOLL_CTL_DEL, epoll_events[i].data.fd, NULL);
				handle_client(this->epollfd.get(), std::move(RAII_FD(epoll_events[i].data.fd)), &recv_buffer);

			}
		}
	}
}
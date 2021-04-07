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

std::optional<Message> Service::create_message(int clientfd, Header h, Service_Constants::Buffer* buffer) {

	Message msg(std::move(h));

	if (h.payload_length > 0) {
		if (h.code != static_cast<uint16_t>(Request_Code::COMPRESS)) {
			this->respond_with_error(clientfd, Status_Code::UNSUPPORTED_TYPE);
			return std::nullopt;
		}
	}

	if (this->recv_bytes(clientfd, buffer, h.payload_length)) {
		return std::nullopt;
	}

	auto start = buffer->begin();
	auto end = start + h.payload_length;
	msg.payload.assign(start, end);

	return msg;
}


std::optional<Header> Service::create_header(int clientfd, Service_Constants::Buffer* buffer) {

	assert(clientfd != -1);

	if (this->recv_bytes(clientfd, buffer, Message_Constants::HEADER_SIZE)) {
		return std::nullopt;
	}

	Header h;
	
	uint8_t* read_head = buffer->data();
	
	h.magic_number = *reinterpret_cast<uint32_t*>(read_head);
	read_head += sizeof(h.magic_number);
	h.payload_length = *reinterpret_cast<uint16_t*>(read_head);
	read_head += sizeof(h.payload_length);
	h.code = *reinterpret_cast<uint16_t*>(read_head);
	h.set_host_order();

	IF_VERBOSE (
		printf("Message:\n- magic_number: %lu\n- payload_length: %u\n- code: %u\n", h.magic_number, h.payload_length, h.code);
	)

	if (h.magic_number != Message_Constants::MAGIC_NUMBER) {
		this->respond_with_error(clientfd, Status_Code::UNKNOWN_ERROR);
		return std::nullopt;
	}

	if (h.code < 1 or h.code > 4) {
		this->respond_with_error(clientfd, Status_Code::UNSUPPORTED_TYPE);
		return std::nullopt;
	}

	if (h.payload_length > Message_Constants::PAYLOAD_SIZE) {
		this->respond_with_error(clientfd, Status_Code::TOO_LARGE);
		return std::nullopt;
	}

	return h;
}


void Service::publish_message(RAII_FD clientfd, Message msg) {

	assert(clientfd.get() != -1);

	Job job = {std::move(msg), std::move(clientfd)};

	std::lock_guard<std::mutex> guard(this->requests_lock);
	this->requests.emplace(std::move(job));
	this->waiting_workers.notify_one();

	IF_VERBOSE (
		printf("Published job to queue\n");
	)

}

void Service::handle_client(RAII_FD clientfd, Service_Constants::Buffer* buffer) {

	assert(clientfd.get() != -1);

	auto header_opt = this->create_header(clientfd.get(), buffer);
	if (!header_opt.has_value()) {
		return;
	}
	Header h = header_opt.value();

	auto msg_opt = this->create_message(clientfd.get(), h, buffer);
	if (!msg_opt.has_value()) {
		return;
	}
	Message msg = msg_opt.value();

	this->publish_message(std::move(clientfd), std::move(msg));

}
 
void Service::accept_requests() {

	epoll_event epoll_ev, epoll_events[Service_Constants::MAX_EPOLL_EVENTS];

	int num_fds = 0;
	Service_Constants::Buffer buffer;
	while (true) {

		num_fds = epoll_wait(this->epollfd.get(), epoll_events, Service_Constants::MAX_EPOLL_EVENTS, -1);
		if (num_fds == -1) {
			throw std::runtime_error("Epoll wait failed");
		}

		for (int i = 0; i < num_fds; i++) {
			if (epoll_events[i].data.fd == this->serverfd.get()) {

				IF_VERBOSE (
					printf("Accepting client\n");
				)

				add_client(this->epollfd.get(), this->serverfd.get(), &this->addr, &epoll_ev);

			} else {

				IF_VERBOSE (
					printf("Handling client\n");
				)

				assert(epoll_events[i].data.fd != -1);
				epoll_ctl(this->epollfd.get(), EPOLL_CTL_DEL, epoll_events[i].data.fd, NULL);
				handle_client(std::move(RAII_FD(epoll_events[i].data.fd)), &buffer);

			}
		}
	}
}
#include <algorithm>
#include <cassert>
#include <list>
#include <message.h>
#include <optional>
#include <service.h>
#include <status-code.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

void add_client(int epollfd, int serverfd, 
                struct sockaddr_in* addr, epoll_event* epoll_ev, 
				std::unordered_map<int, RAII_FD>* open_fds) {

	int addrlen = sizeof(struct sockaddr_in);

	int new_clientfd = accept(serverfd, reinterpret_cast<sockaddr*>(addr), reinterpret_cast<socklen_t*>(&addrlen));
	if (new_clientfd == -1) {
		throw std::runtime_error("Accept connection failed");
	}

	open_fds->emplace(new_clientfd, new_clientfd);

	epoll_ev->data.fd = new_clientfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_clientfd, epoll_ev)) {
		throw std::runtime_error("Epoll CTL add new client failed");
	}

}

std::optional<Message> Service::create_message(int clientfd, std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer) {
	Header h;
	
	uint8_t* read_head = recv_buffer->data();
	
	h.magic_number = *reinterpret_cast<uint32_t*>(read_head);
	read_head += sizeof(h.magic_number);
	h.payload_length = *reinterpret_cast<uint16_t*>(read_head);
	read_head += sizeof(h.payload_length);
	h.code = *reinterpret_cast<uint16_t*>(read_head);

	h.set_host_order();

	if (h.magic_number != Message_Constants::MAGIC_NUMBER) {
		this->respond_with_error(clientfd, Status_Code::UNKNOWN_ERROR);
		return std::nullopt;
	}

	if (h.code < 1 or h.code > 4) {
		this->respond_with_error(clientfd, Status_Code::UNSUPPORTED_TYPE);
		return std::nullopt;
	}

	Message msg(h);

	auto start = recv_buffer->begin() + Message_Constants::HEADER_SIZE;
	auto end = start + h.payload_length;
	msg.payload.assign(start, end);

	return msg;
}

void Service::publish_message(RAII_FD clientfd, std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer) {

	auto msg_opt = this->create_message(clientfd.get(), recv_buffer);
	if (!msg_opt.has_value()) {
		return;
	}

	Job job = {std::move(msg_opt.value()), std::move(clientfd)};

	std::lock_guard<std::mutex> guard(this->requests_lock);
	this->requests.push(std::move(job));
	this->waiting_workers.notify_one();
}

void Service::handle_client(int epollfd, int clientfd, 
							std::array<uint8_t, Service_Constants::RECV_BUFFER_SIZE>* recv_buffer,
							std::unordered_map<int, RAII_FD>* open_fds) {

    // read bytes into the msg buffer until the client is done
	// then try to compose a message
	ssize_t num_bytes = 0;
	std::size_t used_bytes = 0;
	while ( (num_bytes = recv(clientfd, recv_buffer->data() + used_bytes, Service_Constants::RECV_BUFFER_SIZE - used_bytes, 0)) != 0) {

		if (num_bytes == -1) {
			this->respond_with_error(clientfd, Status_Code::UNKNOWN_ERROR);
			open_fds->erase(clientfd);
			return;
		}
		
		this->stats_lock.lock();
		this->total_bytes_recieved+= num_bytes;
		this->stats_lock.unlock();

		used_bytes += num_bytes;

		if (used_bytes == Service_Constants::RECV_BUFFER_SIZE) {

			int extra_bytes;
			ioctl(clientfd, FIONREAD, &extra_bytes);

			if (extra_bytes > 0) {
				this->respond_with_error(clientfd, Status_Code::TOO_LARGE);
				open_fds->erase(clientfd);
				return;
			}
		}

	}

	this->publish_message(std::move(open_fds->operator[](clientfd)), recv_buffer);
	open_fds->erase(clientfd);

	epoll_event epoll_ev;
	epoll_ev.data.fd = clientfd;
	epoll_ev.events = EPOLLIN | EPOLLEXCLUSIVE;

	epoll_ctl(epollfd, EPOLL_CTL_DEL, clientfd, &epoll_ev);
}
 
void Service::accept_requests() {

	std::unordered_map<int, RAII_FD> open_fds;

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
				add_client(this->epollfd.get(), this->serverfd.get(), &this->addr, &epoll_ev, &open_fds);

			} else {

				PRINT("Handle client");
				assert(epoll_events[i].data.fd != -1);
				handle_client(this->epollfd.get(), epoll_events[i].data.fd, &recv_buffer, &open_fds);

			}
		}
	}
}
#include <service.h>
#include <netinet/in.h>
#include <sys/epoll.h>

static constexpr size_t DEFAULT_NUM_LISTENERS = 2;
static constexpr size_t DEFAULT_NUM_WORKERS = 4;
static constexpr size_t DEFAULT_BUFFER_SIZE = 32;

static constexpr int DEFAULT_PORT = 8000;
static constexpr int DEFAULT_BACKLOG_SIZE = 10;

static constexpr int MAX_EPOLL_EVENTS = 10;

Service::Service() : Service(DEFAULT_NUM_LISTENERS, 
							 DEFAULT_NUM_WORKERS, 
							 DEFAULT_PORT,
							 DEFAULT_BACKLOG_SIZE) {}

Service::Service(size_t num_listners, size_t num_workers, int port, int backlog_size):
	port(port), backlog_size(backlog_size) {
	this->listeners.reserve(num_listners);
	for (size_t i = 0; i < num_listners; i++) {
		this->listeners[i] = std::thread(accept_requests);
	}

	this->workers.reserve(num_workers);
	for (size_t i = 0; i < num_workers; i++) {
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

void Service::add_client(int epollfd, int serverfd, const sockaddr_in& addr, epoll_event& epoll_ev) {
	int addrlen = sizeof(addr);
	int new_clientfd = accept(serverfd, (sockaddr*) &addr, (socklen_t*) &addrlen);
	if (new_clientfd == -1) {
		throw std::runtime_error("Accept connection failed");
	}

	epoll_ev.data.fd = new_clientfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, new_clientfd, &epoll_ev)) {
		throw std::runtime_error("Epoll CTL add new client failed");
	}
}
 
void Service::accept_requests() {

	const auto [serverfd, addr] = create_server_socket();  

	epoll_event epoll_ev, epoll_events[MAX_EPOLL_EVENTS];

	int epollfd = epoll_create1(EPOLLEXCLUSIVE);
	if (epollfd == -1) {
		throw std::runtime_error("Epoll create failed");
	}

	epoll_ev.events = EPOLLIN;
	epoll_ev.data.fd = serverfd;

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &epoll_ev)) {
		throw std::runtime_error("Epoll CTL: listener socket failed");
	}

	int num_fds;
	int new_clientfd;
	while (true) {

		num_fds = epoll_wait(epollfd, epoll_events, MAX_EPOLL_EVENTS, -1);
		if (num_fds == -1) {
			throw std::runtime_error("Epoll wait failed");
		}

		for (int i = 0; i < num_fds; i++) {
			if (epoll_events[i].data.fd == serverfd) {

				add_client(epollfd, serverfd, addr, epoll_ev);

			} else {

				handle_client(epoll_events[i].data.fd);

			}
		}
	}

}

void Service::process_requests() {
	
}
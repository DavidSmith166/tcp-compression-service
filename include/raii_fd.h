#ifndef RAII_FD_H
#define RAII_FD_H

#include <algorithm>
#include <unistd.h>
#include <iostream>

struct RAII_FD {

    RAII_FD() = default;

	explicit RAII_FD(int fd)
		: fd(fd) {}

    RAII_FD(RAII_FD&& other) noexcept {
        std::swap(this->fd, other.fd);
    }

    RAII_FD& operator=(RAII_FD&& other) noexcept {
        std::swap(this->fd, other.fd);
        return *this;
    }

	~RAII_FD() {
        if (this->fd != -1){
            std::cout << "Closed " << fd << std::endl;
		    close(this->fd);
        }
	}

    int get() const { return this->fd; }

	int fd = -1;
};

#endif // RAII_FD_H
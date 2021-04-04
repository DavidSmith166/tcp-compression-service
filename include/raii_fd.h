#ifndef RAII_FD_H
#define RAII_FD_H

#include <algorithm>
#include <unistd.h>

struct RAII_FD {

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
        if (fd != -1){
		    close(fd);
        }
	}

	int fd = -1;
};

#endif // RAII_FD_H
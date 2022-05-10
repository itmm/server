#line 59 "README.md"
#include "socket.h"

void Socket::add_to_select(
	fd_set *read, fd_set *write, fd_set *except, int &max
) {
	if (read && can_read_) { FD_SET(fd_, read); }
	if (write && can_write_) { FD_SET(fd_, write); }
	if (except) { FD_SET(fd_, except); }
	if (fd_ > max) { max = fd_; }
}

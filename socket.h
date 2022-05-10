#line 38 "README.md"
#pragma once

#include <sys/select.h>

class Socket {
		int fd_;
		bool can_read_ { true };
		bool can_write_ { false };
	public:
		Socket(int fd): fd_ { fd } { }
		int fd() const { return fd_; }
		void add_to_select(
			fd_set *read, fd_set *write, fd_set *except, int &max
		);
		
};

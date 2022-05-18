#line 86 "1_simple-server.md"
#pragma once

#include <string>
#include <sys/select.h>

class Socket {
		int fd_;
#line 228
		std::string header_;
		std::string reply_ { };
		int reply_pos_ { 0 };
#line 93
	public:
		Socket(int fd): fd_ { fd } { }
		int fd() const { return fd_; }
		void add_to_select(
			fd_set *read, fd_set *write, fd_set *except, int &max
		);
		void parse_request();
		void send_reply();
};

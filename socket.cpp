#line 107 "1_simple-server.md"
#include "socket.h"
#line 289
#include "err.h"
#include <sys/socket.h>
#line 274
#include <iostream>
#line 108

void Socket::add_to_select(
	fd_set *read, fd_set *write, fd_set *except, int &max
) {
#line 279
	if (read && reply_pos_ >= reply_.size()) { FD_SET(fd_, read); std::cout << fd_ << " can read\n"; }
	if (write && reply_pos_ < reply_.size()) { FD_SET(fd_, write); std::cout << fd_ << " can write\n"; }
	if (except) { FD_SET(fd_, except); }
	if (fd_ > max) { max = fd_; }
#line 112
}

void Socket::parse_request() {
#line 293
	char buffer[100];
	ssize_t got;
	do {
		got = recv(fd_, buffer, sizeof(buffer), MSG_DONTWAIT);
		if (got < 0) { ERR("error while reading from socket ", fd_); }
		if (got == 0) { };
		for (auto cur { buffer }, end { buffer + got }; cur != end; ++cur) {
			if (*cur == '\n') {
				if (header_ == "\r") {
					std::cout << "end of header detected\n";
					std::string content =
						"<!DOCTYPE html>\r\n"
						"<html>\r\n"
						"<head>\r\n"
						"\t<title>Server Fehler</title>\r\n"
						"</head>\r\n"
						"<body>\r\n"
						"\t<h1>Server Fehler</h1>\r\n"
						"</body></html>\r\n";

					reply_ =
						"HTTP/1.1 500 Internal Error\r\n"
						"Content-Type: text/html\r\n"
						"Content-Length: " + std::to_string(content.size()) +
						"\r\n\r\n" + content;
					reply_pos_ = 0;
				}
				header_.clear();
			} else { header_ += *cur; }
		}
	} while (got == sizeof(buffer));
#line 115
}

void Socket::send_reply() {
#line 331
	auto offset { reply_pos_ };
	auto len { reply_.size() - offset };
	auto got { send(fd_, reply_.c_str() + offset, len, MSG_DONTWAIT) };
	if (got < 0) { ERR("error while writing to socket ", fd_); }
	reply_pos_ += got;
#line 118
}

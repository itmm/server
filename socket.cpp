#line 63 "README.md"
#include "socket.h"

#include "err.h"

#include <iostream>
#include <sys/socket.h>

void Socket::add_to_select(
	fd_set *read, fd_set *write, fd_set *except, int &max
) {
std::cout << "pos == " << reply_pos_ << ", size == " << reply_.size() << "\n";
	if (read && reply_pos_ >= reply_.size()) { FD_SET(fd_, read); std::cout << fd_ << " can read\n"; }
	if (write && reply_pos_ < reply_.size()) { FD_SET(fd_, write); std::cout << fd_ << " can write\n"; }
	if (except) { FD_SET(fd_, except); }
	if (fd_ > max) { max = fd_; }
}

void Socket::parse_request() {
	char buffer[100];
	ssize_t got;
	do {
		got = recv(fd_, buffer, sizeof(buffer), MSG_DONTWAIT);
		if (got < 0) { err("error while reading from socket ", fd_); }
		if (got == 0) { };
		std::cout << "read " << got << " chars\n";
		for (auto cur { buffer }, end { buffer + got }; cur != end; ++cur) {
			if (*cur == '\n') {
				std::cout << "got header \"" << header_.substr(0, header_.size() - 1) << "\"\n";
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
	std::cout << "pre pos == " << reply_pos_ << ", size == " << reply_.size() << "\n";
				}
				header_.clear();
			} else { header_ += *cur; }
		}
	} while (got == sizeof(buffer));
}

void Socket::send_reply() {
	auto offset { reply_pos_ };
	auto len { reply_.size() - offset };
	std::cout << "offset == " << offset << ", len == " << len << "\n";
	auto got { send(fd_, reply_.c_str() + offset, len, MSG_DONTWAIT) };
	if (got < 0) { err("error while writing to socket ", fd_); }
	reply_pos_ += got;
}

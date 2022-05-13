`err.h`

```c++
#pragma once

#include <sstream>
#include <stdexcept>
#include <string>

class Error: public std::exception {
		std::string what_;
	public:
		Error(std::string what): what_ { what } { }
		const char *what() const noexcept override {
			return what_.c_str();
		}
};

inline void err_with_stream(std::ostringstream &out) {
	throw Error { out.str() };
}

template<typename ARG, typename... ARGS> inline void err_with_stream(
	std::ostringstream &out, ARG arg, ARGS... rest
) {
	out << arg;
	err_with_stream(out, rest...);
}

template<typename... ARGS> inline void err(ARGS... args) {
	std::ostringstream out;
	err_with_stream(out, args...);
}
```
`socket.h`

```c++
#pragma once

#include <string>
#include <sys/select.h>

class Socket {
		int fd_;
		std::string header_;
		std::string reply_ { };
		int reply_pos_ { 0 };
	public:
		Socket(int fd): fd_ { fd } { }
		int fd() const { return fd_; }
		void add_to_select(
			fd_set *read, fd_set *write, fd_set *except, int &max
		);
		void parse_request();
		void send_reply();
		
};
```

`socket.cpp`

```c++
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
	char buffer[2048];
	auto got { recv(fd_, buffer, sizeof(buffer), MSG_DONTWAIT) };
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
					"\r\n" + content;
				reply_pos_ = 0;
std::cout << "pre pos == " << reply_pos_ << ", size == " << reply_.size() << "\n";
			}
			header_.clear();
		} else { header_ += *cur; }
	}
}

void Socket::send_reply() {
	auto offset { reply_pos_ };
	auto len { reply_.size() - offset };
	std::cout << "offset == " << offset << ", len == " << len << "\n";
	auto got { send(fd_, reply_.c_str() + offset, len, MSG_DONTWAIT) };
	if (got < 0) { err("error while writing to socket ", fd_); }
	reply_pos_ += got;
}
```

`server.cpp`

```c++
#include "err.h"
#include "socket.h"

#include <iostream>
#include <map>
#include <netinet/in.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int port { 9869 }; // (int) (M_PI * M_PI * 1000)
int server { -1 };

std::map<int, Socket> open_sockets;

static inline void run_server() {
}

int main(int argc, const char *argv[]) {
	try {
		run_server();
	} catch (const Error &e) {
		std::cerr << "server failed: " << e.what() << "\n";
	}
	for (auto &[fd, sock]: open_sockets) {
		close(fd);
		std::cout << "close socket " << fd << "\n";
	}
	if (server >= 0) { close(server); }
}
```

```c++
// ...
std::map<int, Socket> open_sockets;

static inline void accept_connection() {
	sockaddr_in addr;
	auto casted_addr { reinterpret_cast<sockaddr *>(&addr) };
	socklen_t addr_len = sizeof(addr);
	int connection = accept(server, casted_addr, &addr_len);
	if (connection < 0) { err("can't get connection"); }
	open_sockets.insert({ connection, Socket { connection } });
	std::cout << "got connection " << connection << "\n";
}

static inline void perform_select() {
	fd_set read_set, write_set, except_set;
	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&except_set);
	FD_SET(server, &read_set);
	FD_SET(server, &except_set);
	int max { server };
	for (auto &[fd, sock]: open_sockets) {
		sock.add_to_select(&read_set, &write_set, &except_set, max);
	}

	int got { select(
		max + 1, &read_set, &write_set, &except_set, nullptr
	) };
	if (got < 0) { err("select error"); }

	if (FD_ISSET(server, &except_set)) {
		close(server);
		server = -1;
		err("server socket failed");
	} else if (FD_ISSET(server, &read_set)) {
		accept_connection();
	}
	
	for (auto &[fd, sock]: open_sockets) {
		if (FD_ISSET(fd, &except_set)) {
			open_sockets.erase(open_sockets.find(fd));
			close(fd);
			err("error on socket ", fd, "\n");
		} else if (FD_ISSET(fd, &read_set)) {
			sock.parse_request();
		} else if (FD_ISSET(fd, &write_set)) {
			sock.send_reply();
		}
	}
}

static inline void run_server() {
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0) { err("can't create server socket"); }

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	auto casted_addr { reinterpret_cast<sockaddr *>(&addr) };

	if (bind(server, casted_addr, sizeof(addr)) < 0) {
		err("can't bind server to port ", port);
	}

	if (listen(server, 10) < 0) { err("can't listen to server"); }

	std::cout << "server ready on port " << port << "\n";
	for (;;) { try {
		perform_select();
	} catch (const Error &e) {
		std::cerr << "error: " << e.what() << "\n";
		if (server < 0) { throw e; }
	} }
}
// ...
```


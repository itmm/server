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

void err_with_stream(std::ostringstream &out) {
	throw Error { out.str() };
}

template<typename ARG, typename... ARGS> void err_with_stream(
	std::ostringstream &out, ARG arg, ARGS... rest
) {
	out << arg;
	err_with_stream(out, rest...);
}

template<typename... ARGS> void err(ARGS... args) {
	std::ostringstream out;
	err_with_stream(out, args...);
}
```
`socket.h`

```c++
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
```

`socket.cpp`

```c++
#include "socket.h"

void Socket::add_to_select(
	fd_set *read, fd_set *write, fd_set *except, int &max
) {
	if (read && can_read_) { FD_SET(fd_, read); }
	if (write && can_write_) { FD_SET(fd_, write); }
	if (except) { FD_SET(fd_, except); }
	if (fd_ > max) { max = fd_; }
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

std::map<int, Socket> open_sockets;

int main(int argc, const char *argv[]) {
	int server { -1 };
	try {
		server = socket(AF_INET, SOCK_STREAM, 0);
		if (server < 0) { err("can't create server socket"); }
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(port);
		if (bind(server, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
			err("can't bind server to port ", port);
		}
		if (listen(server, 10) < 0) { err("can't listen to server"); }

		std::cout << "server ready on port " << port << "\n";
		fd_set read_set, write_set, except_set;
		for (;;) {
			try {
				FD_ZERO(&read_set);
				FD_ZERO(&write_set);
				FD_ZERO(&except_set);
				FD_SET(server, &read_set);
				FD_SET(server, &except_set);
				int max { server };
				for (auto [fd, sock]: open_sockets) {
					sock.add_to_select(
						&read_set, &write_set, &except_set, max
					);
				}
				int got { select(
					max + 1, &read_set, &write_set, &except_set, nullptr
				) };
				if (got < 0) { err("select error"); }
			
				if (FD_ISSET(server, &except_set)) {
					err("server socket failed");
				} else if (FD_ISSET(server, &read_set)) {
					auto addr_len { sizeof(addr) };
					int connection = accept(server, reinterpret_cast<sockaddr *>(&addr), reinterpret_cast<socklen_t*>(&addr_len));
					if (connection < 0) { err("can't get connection"); }
					open_sockets.insert({connection, Socket { connection }});
					std::cout << "got connection " << connection << "\n";
				}
				
				for (auto [fd, sock]: open_sockets) {
					if (FD_ISSET(fd, &except_set)) {
						open_sockets.erase(open_sockets.find(fd));
						close(fd);
						err("error on socket ", fd, "\n");
					}
					else if (FD_ISSET(fd, &read_set)) {
						char buffer[100];
						read(fd, buffer, sizeof(buffer));
					}
				}
			} catch (const Error &e) {
				std::cerr << "error processing socket: " << e.what() << "\n";
				if (server < 0) { throw e; }
			}
		}
	} catch (const Error &e) {
		std::cerr << "server failed: " << e.what() << "\n";
	}
	for (auto [fd, sock]: open_sockets) {
		close(fd);
		std::cout << "close socket " << fd << "\n";
	}
	if (server >= 0) { close(server); }
}
```

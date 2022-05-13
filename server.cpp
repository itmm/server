#line 126 "README.md"
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

#line 163
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

#line 142
static inline void run_server() {
#line 212
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
#line 143
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

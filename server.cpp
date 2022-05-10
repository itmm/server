#line 39 "README.md"
#include "err.h"

#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

int port { 9869 }; // (int) (M_PI * M_PI * 1000)

int main(int argc, const char *argv[]) {
	auto server { socket(AF_INET, SOCK_STREAM, 0) };
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
	for (;;) {
		auto addr_len { sizeof(addr) };
		auto connection { accept(server, reinterpret_cast<sockaddr *>(&addr), reinterpret_cast<socklen_t*>(&addr_len)) };
		if (connection < 0) { err("can't get connection"); }
		std::cout << "got connection\n";
		
		char buffer[100];
		read(connection, buffer, sizeof(buffer));

		std::string reply { "good talking to you" };
		send(connection, reply.c_str(), reply.size(), 0);
		close(connection);
		std::cout << "done connection\n";
	}
	close(server);
}

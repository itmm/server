# Einen einfachen Server erzeugen

Aufgabe des Servers ist es, ein paar hundert gleichzeitige Nutzer auszuhalten.
Er richtet sich erst einmal nicht an riesige Open-Source Projekte, sondern
kleinere Projekte, die von wenigen Teams durchgeführt werden.

Daher stelle ich hier einen Server vor, der in einem einzigen Prozess arbeitet
und die offenen Sockets asynchron bedient. Dies vereinfacht die Synchronisierung
enorm und ist für unsere Ansprüche erst einmal schnell genug.

Und erstaunlich effizient! Er braucht viel weniger Ressourcen als ein
synchroner Server, in dem jede Socket in ihrem eigenen Thread abgearbeitet
wird.

## Fehlerbehandlung

Die Fehlerbehandlung des Servers wird in der Datei `err.h` definiert.
Sie besteht fast immer daraus, dasse eine Exception `Error` geworfen wird:

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
```

Diese wird entweder abgefangen, oder beendet den Server.

Die Funktion `err` fasst ihre Argumente in einen `std::osstream` zusammen,
um die Exception dann mit deren String-Repräsentation zu werfen.

```c++
// ...
// err_with_stream functions
template<typename... ARGS> inline void err(ARGS... args) {
	std::ostringstream out;
	err_with_stream(out, args...);
}

```

Folgende Funktion setzen den String zusammen:

```c++
// ...
// err_with_stream functions
inline void err_with_stream(std::ostringstream &out) {
	throw Error { out.str() };
}

template<typename ARG, typename... ARGS> inline void err_with_stream(
	std::ostringstream &out, ARG arg, ARGS... rest
) {
	out << arg;
	err_with_stream(out, rest...);
}
// ...
```

Als kleine Vereinfachung gibt es das Makro `ERR`. Es fügt zusätzlich noch
die Aktuelle Datei mit Zeilennummer hinzu, damit der Ursprung der Exception
schnell im Editor gefunden werden kann.

```c++
// ...`
#define ERR(...) err(__FILE__, ':', __LINE__, ": ", __VA_ARGS__)
```

## Die `Socket` Schnittstelle

Der Server kann mehrere Verbindungen parallel offen haben.
Jede Verbindung wird in `socket.h` über eine
`Socket`-Instanz abgebildet.  Als Identifizierung dient der Filehandle `fd`:

```c++
#pragma once

#include <string>
#include <sys/select.h>

class Socket {
		int fd_;
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

void Socket::add_to_select(
	fd_set *read, fd_set *write, fd_set *except, int &max
) {
}

void Socket::parse_request() {
}

void Socket::send_reply() {
}
```

Bevor wir zur Implementierung der Klasse kommen, sehen wir uns ihre
Verwendung in `server.cpp` an. Der Server hat eine Liste von Verbindungen.
Beim beenden werden alle Sockets geschlossen.

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
		std::cerr << e.what() << "\n";
	}
	for (auto &[fd, sock]: open_sockets) {
		close(fd);
		std::cout << "close socket " << fd << "\n";
	}
	if (server >= 0) { close(server); }
}
```

`server.cpp`

```c++
// ...
std::map<int, Socket> open_sockets;

static inline void accept_connection() {
}

static inline void perform_select() {
}

static inline void run_server() {
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server < 0) { ERR("can't create server socket"); }

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	auto casted_addr { reinterpret_cast<sockaddr *>(&addr) };

	if (bind(server, casted_addr, sizeof(addr)) < 0) {
		ERR("can't bind server to port ", port);
	}

	if (listen(server, 10) < 0) { ERR("can't listen to server"); }

	std::cout << "server ready on port " << port << "\n";
	for (;;) { try {
		perform_select();
	} catch (const Error &e) {
		std::cerr << e.what() << "\n";
		if (server < 0) { throw e; }
	} }
}
// ...
```

`server.cpp`

```c++
// ...
static inline void accept_connection() {
	sockaddr_in addr;
	auto casted_addr { reinterpret_cast<sockaddr *>(&addr) };
	socklen_t addr_len = sizeof(addr);
	int connection = accept(server, casted_addr, &addr_len);
	if (connection < 0) { ERR("can't get connection"); }
	open_sockets.insert({ connection, Socket { connection } });
	std::cout << "got connection " << connection << "\n";
}
// ...
```

```c++
// ...
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
	if (got < 0) { ERR("select error"); }

	if (FD_ISSET(server, &except_set)) {
		close(server);
		server = -1;
		ERR("server socket failed");
	} else if (FD_ISSET(server, &read_set)) {
		accept_connection();
	}
	
	for (auto &[fd, sock]: open_sockets) {
		if (FD_ISSET(fd, &except_set)) {
			open_sockets.erase(open_sockets.find(fd));
			close(fd);
			ERR("error on socket ", fd, "\n");
		} else if (FD_ISSET(fd, &read_set)) {
			sock.parse_request();
		} else if (FD_ISSET(fd, &write_set)) {
			sock.send_reply();
		}
	}
}
// ...
```

`socket.h`

```c++
// ...
class Socket {
		int fd_;
		std::string header_;
		std::string reply_ { };
		int reply_pos_ { 0 };
	// ...
};
```

`socket.cpp`

```c++
// ...
#include "socket.h"
#include <iostream>
// ...
void Socket::add_to_select(
	fd_set *read, fd_set *write, fd_set *except, int &max
) {
	if (read && reply_pos_ >= reply_.size()) { FD_SET(fd_, read); std::cout << fd_ << " can read\n"; }
	if (write && reply_pos_ < reply_.size()) { FD_SET(fd_, write); std::cout << fd_ << " can write\n"; }
	if (except) { FD_SET(fd_, except); }
	if (fd_ > max) { max = fd_; }
}
// ...
```

```c++
#include "socket.h"
#include "err.h"
#include <sys/socket.h>
// ...
void Socket::parse_request() {
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
}
// ...
```

```c++
// ...
void Socket::send_reply() {
	auto offset { reply_pos_ };
	auto len { reply_.size() - offset };
	auto got { send(fd_, reply_.c_str() + offset, len, MSG_DONTWAIT) };
	if (got < 0) { ERR("error while writing to socket ", fd_); }
	reply_pos_ += got;
}
```


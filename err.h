#line 21 "1_simple-server.md"
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
#line 44
// err_with_stream functions
#line 57
inline void err_with_stream(std::ostringstream &out) {
	throw Error { out.str() };
}

template<typename ARG, typename... ARGS> inline void err_with_stream(
	std::ostringstream &out, ARG arg, ARGS... rest
) {
	out << arg;
	err_with_stream(out, rest...);
}
#line 45
template<typename... ARGS> inline void err(ARGS... args) {
	std::ostringstream out;
	err_with_stream(out, args...);
}

#line 76
#define ERR(...) err(__FILE__, ':', __LINE__, ": ", __VA_ARGS__)

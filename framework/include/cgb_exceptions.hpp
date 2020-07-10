#pragma once
#include <exekutor.hpp>

namespace xk
{
	class runtime_error : public std::runtime_error {
	public:
		explicit runtime_error (const std::string& what_arg);
		explicit runtime_error (const char* what_arg);
	};

	class logic_error : public std::logic_error {
	public:
		explicit logic_error (const std::string& what_arg);
		explicit logic_error (const char* what_arg);
	};
}

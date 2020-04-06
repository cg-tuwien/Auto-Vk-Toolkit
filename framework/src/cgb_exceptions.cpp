#include <cg_base.hpp>

namespace cgb
{
	runtime_error::runtime_error (const std::string& what_arg) : std::runtime_error(what_arg)
	{
		LOG_ERROR_EM(fmt::format("!RUNTIME ERROR! {}", what_arg));
	}
	
	runtime_error::runtime_error (const char* what_arg) : std::runtime_error(what_arg)
	{
		LOG_ERROR_EM(fmt::format("!RUNTIME ERROR! {}", what_arg));
	}

	logic_error::logic_error (const std::string& what_arg) : std::logic_error(what_arg)
	{
		LOG_ERROR_EM(fmt::format("!LOGIC ERROR! {}", what_arg));
	}
	
	logic_error::logic_error (const char* what_arg) : std::logic_error(what_arg)
	{
		LOG_ERROR_EM(fmt::format("!LOGIC ERROR! {}", what_arg));
	}
}

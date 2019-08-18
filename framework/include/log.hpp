#pragma once

namespace cgb
{
	// Define LOGGING_ON_SEPARATE_THREAD to have all the logging being transmitted and performed by a separate thread
	//#define LOGGING_ON_SEPARATE_THREAD

	// Define PRINT_STACKTRACE to have the stack trace printed for errors
	#define PRINT_STACKTRACE

	// LOG-LEVELS:
	// 0 ... nothing (except debug messages in DEBUG-mode)
	// 1 ... error messages only
	// 2 ... errors and warnings
	// 3 ... errors, warnings and infos
	// 4 ... everything
	#if !defined(LOG_LEVEL)
	#define LOG_LEVEL 4
	#endif

	enum struct log_type
	{
		error,
		warning,
		info,
		verbose,
		debug,
		debug_verbose,
		system
	};

	enum struct log_importance
	{
		normal,
		important
	};

	struct log_pack
	{
		std::string mMessage;
		log_type mLogType;
		log_importance mLogImportance;
		std::string mStacktrace;
	};
	
	extern void set_console_output_color(cgb::log_type level, cgb::log_importance importance);
	extern void set_console_output_color_for_stacktrace(cgb::log_type level, cgb::log_importance importance);
	extern void reset_console_output_color();
	extern void dispatch_log(cgb::log_pack pToBeLogged);
	
	#if LOG_LEVEL > 0
	#define LOG_ERROR(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "ERR:  ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::error, cgb::log_importance::normal })
	#define LOG_ERROR_EM(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "ERR:  ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::error, cgb::log_importance::important})
	#define LOG_ERROR__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "ERR:  ", msg), cgb::log_type::error, cgb::log_importance::normal })
	#define LOG_ERROR_EM__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "ERR:  ", msg), cgb::log_type::error, cgb::log_importance::important })
	#else
	#define LOG_ERROR(msg)
	#define LOG_ERROR_EM(msg)
	#define LOG_ERROR__(msg)
	#define LOG_ERROR_EM__(msg)
	#endif

	#if LOG_LEVEL > 1
	#define LOG_WARNING(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "WARN: ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::warning, cgb::log_importance::normal })
	#define LOG_WARNING_EM(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "WARN: ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::warning, cgb::log_importance::important })
	#define LOG_WARNING__(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "WARN: ", msg), cgb::log_type::warning, cgb::log_importance::normal })
	#define LOG_WARNING_EM__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "WARN: ", msg), cgb::log_type::warning, cgb::log_importance::important })
	#else 
	#define LOG_WARNING(msg)
	#define LOG_WARNING_EM(msg)
	#define LOG_WARNING__(msg)
	#define LOG_WARNING_EM__(msg)
	#endif

	#if LOG_LEVEL > 2
	#define LOG_INFO(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "INFO: ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::info, cgb::log_importance::normal })
	#define LOG_INFO_EM(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "INFO: ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::info, cgb::log_importance::important })
	#define LOG_INFO__(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "INFO: ", msg), cgb::log_type::info, cgb::log_importance::normal })
	#define LOG_INFO_EM__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "INFO: ", msg), cgb::log_type::info, cgb::log_importance::important })
	#else
	#define LOG_INFO(msg)
	#define LOG_INFO_EM(msg)
	#define LOG_INFO__(msg)
	#define LOG_INFO_EM__(msg)
	#endif

	#if LOG_LEVEL > 3
	#define LOG_VERBOSE(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "VRBS: ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::verbose, cgb::log_importance::normal })
	#define LOG_VERBOSE_EM(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "VRBS: ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::verbose, cgb::log_importance::important })
	#define LOG_VERBOSE__(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "VRBS: ", msg), cgb::log_type::verbose, cgb::log_importance::normal })
	#define LOG_VERBOSE_EM__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "VRBS: ", msg), cgb::log_type::verbose, cgb::log_importance::important })
	#else 
	#define LOG_VERBOSE(msg)
	#define LOG_VERBOSE_EM(msg)
	#define LOG_VERBOSE__(msg)
	#define LOG_VERBOSE_EM__(msg)
	#endif

	#ifdef _DEBUG
	#define LOG_DEBUG(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "DBG:  ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::debug, cgb::log_importance::normal })
	#define LOG_DEBUG_EM(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "DBG:  ", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::debug, cgb::log_importance::important })
	#define LOG_DEBUG__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "DBG:  ", msg), cgb::log_type::debug, cgb::log_importance::normal })
	#define LOG_DEBUG_EM__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "DBG:  ", msg), cgb::log_type::debug, cgb::log_importance::important })
	#else
	#define LOG_DEBUG(msg)
	#define LOG_DEBUG_EM(msg)
	#define LOG_DEBUG__(msg)
	#define LOG_DEBUG_EM__(msg)	
	#endif

	#if defined(_DEBUG) && LOG_LEVEL > 3
	#define LOG_DEBUG_VERBOSE(msg)		cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "DBG-V:", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::debug_verbose, cgb::log_importance::normal })
	#define LOG_DEBUG_VERBOSE_EM(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}{}\n", "DBG-V:", msg, fmt::format(" | file[{}] line[{}]", cgb::extract_file_name(std::string(__FILE__)), __LINE__)), cgb::log_type::debug_verbose, cgb::log_importance::important })
	#define LOG_DEBUG_VERBOSE__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "DBG-V:", msg), cgb::log_type::debug_verbose, cgb::log_importance::normal })
	#define LOG_DEBUG_VERBOSE_EM__(msg)	cgb::dispatch_log(cgb::log_pack{ fmt::format("{}{}\n", "DBG-V:", msg), cgb::log_type::debug_verbose, cgb::log_importance::important })
	#else
	#define LOG_DEBUG_VERBOSE(msg)
	#define LOG_DEBUG_VERBOSE_EM(msg)   
	#define LOG_DEBUG_VERBOSE__(msg)
	#define LOG_DEBUG_VERBOSE_EM__(msg)
	#endif

	std::string to_string(const glm::mat4&);
	std::string to_string(const glm::mat3&);
	std::string to_string_compact(const glm::mat4&);
	std::string to_string_compact(const glm::mat3&);

	std::string to_string(const glm::vec2&);
	std::string to_string(const glm::vec3&);
	std::string to_string(const glm::vec4&);

	std::string fourcc_to_string(unsigned int fourcc);

	std::string get_current_callstack();
}

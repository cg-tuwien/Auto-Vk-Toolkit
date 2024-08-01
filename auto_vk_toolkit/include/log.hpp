#pragma once

namespace avk
{
	// Define LOGGING_ON_SEPARATE_THREAD to have all the logging being transmitted and performed by a separate thread
	#if !defined(NO_SEPARATE_LOGGING_THREAD)
	//#define LOGGING_ON_SEPARATE_THREAD
	#endif

	// Define PRINT_STACKTRACE to have the stack trace printed for errors
	#if !defined(DONT_PRINT_STACKTRACE)
	#define PRINT_STACKTRACE
	#endif

	// LOG-LEVELS:
	// 0 ... nothing (except debug messages in DEBUG-mode)
	// 1 ... error messages only
	// 2 ... errors and warnings
	// 3 ... errors, warnings and infos
	// 4 ... errors, warnings, infos, and verbose
	// 5 ... errors, warnings, infos, verbose, and mega-verbose
	#if !defined(LOG_LEVEL)
	#define LOG_LEVEL 3
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
	
	extern void set_console_output_color(avk::log_type level, avk::log_importance importance);
	extern void set_console_output_color_for_stacktrace(avk::log_type level, avk::log_importance importance);
	extern void reset_console_output_color();
	extern void dispatch_log(avk::log_pack pToBeLogged);
	
	#if LOG_LEVEL > 0
	#define LOG_ERROR(msg)			avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "ERR:  ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::error, avk::log_importance::normal })
	#define LOG_ERROR_EM(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "ERR:  ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::error, avk::log_importance::important})
	#define LOG_ERROR___(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "ERR:  ", msg), avk::log_type::error, avk::log_importance::normal })
	#define LOG_ERROR_EM___(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "ERR:  ", msg), avk::log_type::error, avk::log_importance::important })
	#else
	#define LOG_ERROR(msg)
	#define LOG_ERROR_EM(msg)
	#define LOG_ERROR___(msg)
	#define LOG_ERROR_EM___(msg)
	#endif

	#if LOG_LEVEL > 1
	#define LOG_WARNING(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "WARN: ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::warning, avk::log_importance::normal })
	#define LOG_WARNING_EM(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "WARN: ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::warning, avk::log_importance::important })
	#define LOG_WARNING___(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "WARN: ", msg), avk::log_type::warning, avk::log_importance::normal })
	#define LOG_WARNING_EM___(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "WARN: ", msg), avk::log_type::warning, avk::log_importance::important })
	#else 
	#define LOG_WARNING(msg)
	#define LOG_WARNING_EM(msg)
	#define LOG_WARNING___(msg)
	#define LOG_WARNING_EM___(msg)
	#endif

	#if LOG_LEVEL > 2
	#define LOG_INFO(msg)			avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "INFO: ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::info, avk::log_importance::normal })
	#define LOG_INFO_EM(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "INFO: ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::info, avk::log_importance::important })
	#define LOG_INFO___(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "INFO: ", msg), avk::log_type::info, avk::log_importance::normal })
	#define LOG_INFO_EM___(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "INFO: ", msg), avk::log_type::info, avk::log_importance::important })
	#else
	#define LOG_INFO(msg)
	#define LOG_INFO_EM(msg)
	#define LOG_INFO___(msg)
	#define LOG_INFO_EM___(msg)
	#endif

	#if LOG_LEVEL > 3
	#define LOG_VERBOSE(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "VRBS: ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::verbose, avk::log_importance::normal })
	#define LOG_VERBOSE_EM(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "VRBS: ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::verbose, avk::log_importance::important })
	#define LOG_VERBOSE___(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "VRBS: ", msg), avk::log_type::verbose, avk::log_importance::normal })
	#define LOG_VERBOSE_EM___(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "VRBS: ", msg), avk::log_type::verbose, avk::log_importance::important })
	#else 
	#define LOG_VERBOSE(msg)
	#define LOG_VERBOSE_EM(msg)
	#define LOG_VERBOSE___(msg)
	#define LOG_VERBOSE_EM___(msg)
	#endif

	#if LOG_LEVEL > 4
	#define LOG_MEGA_VERBOSE(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "MVRBS:", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::verbose, avk::log_importance::normal })
	#define LOG_MEGA_VERBOSE_EM(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "MVRBS:", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::verbose, avk::log_importance::important })
	#define LOG_MEGA_VERBOSE___(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "MVRBS:", msg), avk::log_type::verbose, avk::log_importance::normal })
	#define LOG_MEGA_VERBOSE_EM___(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "MVRBS:", msg), avk::log_type::verbose, avk::log_importance::important })
	#else 
	#define LOG_MEGA_VERBOSE(msg)
	#define LOG_MEGA_VERBOSE_EM(msg)
	#define LOG_MEGA_VERBOSE___(msg)
	#define LOG_MEGA_VERBOSE_EM___(msg)
	#endif

	#if defined(_DEBUG)
	#define LOG_DEBUG(msg)			avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "DBG:  ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::debug, avk::log_importance::normal })
	#define LOG_DEBUG_EM(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "DBG:  ", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::debug, avk::log_importance::important })
	#define LOG_DEBUG___(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "DBG:  ", msg), avk::log_type::debug, avk::log_importance::normal })
	#define LOG_DEBUG_EM___(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "DBG:  ", msg), avk::log_type::debug, avk::log_importance::important })
	#else
	#define LOG_DEBUG(msg)
	#define LOG_DEBUG_EM(msg)
	#define LOG_DEBUG___(msg)
	#define LOG_DEBUG_EM___(msg)	
	#endif

	#if defined(_DEBUG) && LOG_LEVEL > 3
	#define LOG_DEBUG_VERBOSE(msg)			avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "DBG-V:", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::debug_verbose, avk::log_importance::normal })
	#define LOG_DEBUG_VERBOSE_EM(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "DBG-V:", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::debug_verbose, avk::log_importance::important })
	#define LOG_DEBUG_VERBOSE___(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "DBG-V:", msg), avk::log_type::debug_verbose, avk::log_importance::normal })
	#define LOG_DEBUG_VERBOSE_EM___(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "DBG-V:", msg), avk::log_type::debug_verbose, avk::log_importance::important })
	#else
	#define LOG_DEBUG_VERBOSE(msg)
	#define LOG_DEBUG_VERBOSE_EM(msg)   
	#define LOG_DEBUG_VERBOSE___(msg)
	#define LOG_DEBUG_VERBOSE_EM___(msg)
	#endif

	#if defined(_DEBUG) && LOG_LEVEL > 4
	#define LOG_DEBUG_MEGA_VERBOSE(msg)			avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "DBG-MV:", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::debug_verbose, avk::log_importance::normal })
	#define LOG_DEBUG_MEGA_VERBOSE_EM(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}{}\n", "DBG-MV:", msg, std::format(" | file[{}] line[{}]", avk::extract_file_name(std::string(__FILE__)), __LINE__)), avk::log_type::debug_verbose, avk::log_importance::important })
	#define LOG_DEBUG_MEGA_VERBOSE___(msg)		avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "DBG-MV:", msg), avk::log_type::debug_verbose, avk::log_importance::normal })
	#define LOG_DEBUG_MEGA_VERBOSE_EM___(msg)	avk::dispatch_log(avk::log_pack{ std::format("{}{}\n", "DBG-MV:", msg), avk::log_type::debug_verbose, avk::log_importance::important })
	#else
	#define LOG_DEBUG_MEGA_VERBOSE(msg)
	#define LOG_DEBUG_MEGA_VERBOSE_EM(msg)   
	#define LOG_DEBUG_MEGA_VERBOSE___(msg)
	#define LOG_DEBUG_MEGA_VERBOSE_EM___(msg)
	#endif

	std::string to_string(const glm::mat4&);
	std::string to_string(const glm::mat3&);
	std::string to_string_compact(const glm::mat4&);
	std::string to_string_compact(const glm::mat3&);

	std::string to_string(const glm::vec2&);
	std::string to_string(const glm::vec3&);
	std::string to_string(const glm::vec4&);

	std::string get_current_callstack();
}

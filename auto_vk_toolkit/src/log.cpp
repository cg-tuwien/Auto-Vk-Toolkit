#if defined(_WIN32) && defined (_DEBUG) && defined (PRINT_STACKTRACE)
#include <Windows.h>
#include <DbgHelp.h>
#include <sstream>
#endif

namespace avk
{
	void set_console_output_color(avk::log_type level, avk::log_importance importance)
	{
#ifdef _WIN32
		static auto std_output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		switch (level) {
		case avk::log_type::error:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xCF); // white on red
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0xC); // red on black
				break;
			}
			break;
		case avk::log_type::warning:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xE0); // black on yellow
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0xE); // yellow on black
				break;
			}
			break;
		case avk::log_type::verbose:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0x80); // black on gray
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0x8); // gray on black
				break;
			}
			break;
		case avk::log_type::debug:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xA0); // black on green
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0xA); // green on black
				break;
			}
			break;
		case avk::log_type::debug_verbose:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0x20); // black on dark green
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0x2); // dark green on black
				break;
			}
			break;
		case avk::log_type::system:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xDF); // white on magenta
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0xD); // magenta on black
				break;
			}
			break;
		default:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xF0); // black on white
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0xF); // white on black
				break;
			}
			break;
		}
#endif // WIN32
	}

	void set_console_output_color_for_stacktrace(avk::log_type level, avk::log_importance importance)
	{
#ifdef _WIN32
		static auto std_output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		switch (level) {
		case avk::log_type::error:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xC7); 
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0x4);
				break;
			}
			break;
		case avk::log_type::warning:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xE7); 
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0x6); 
				break;
			}
			break;
		case avk::log_type::verbose:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0x87); 
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0x7); 
				break;
			}
			break;
		case avk::log_type::debug:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xA2);
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0x2); 
				break;
			}
			break;
		case avk::log_type::debug_verbose:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0x28); 
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0x8);
				break;
			}
			break;
		case avk::log_type::system:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xD7); 
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0x5); 
				break;
			}
			break;
		default:
			switch (importance) {
			case avk::log_importance::important:
				SetConsoleTextAttribute(std_output_handle, 0xF8); 
				break;
			default:
				SetConsoleTextAttribute(std_output_handle, 0xF7); 
				break;
			}
			break;
		}
#endif // WIN32
	}

	void reset_console_output_color()
	{
#ifdef _WIN32
		static auto std_output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(std_output_handle, 0xF); // white on black
#endif // WIN32
	}

#ifdef LOGGING_ON_SEPARATE_THREAD

	void dispatch_log(log_pack pToBeLogged)
	{
		static std::mutex sLogMutex;
		static std::condition_variable sCondVar;
		static std::queue<log_pack> sLogQueue;
		static bool sContinueLogging = true;
		static std::thread sLogThread = std::thread([]() {
			
			avk::set_console_output_color(avk::log_type::system, avk::log_importance::important);
			std::cout << "Logger thread started...";
			avk::reset_console_output_color();
			std::cout << std::endl;

			while (sContinueLogging) {
				// Process all messages
				{
					log_pack front;
					bool empty;
					// 1st lock => See if there is something
					{
						std::scoped_lock<std::mutex> guard(sLogMutex);
						empty = sLogQueue.empty();
						if (!empty)
						{
							front = sLogQueue.front();
							sLogQueue.pop();
						}
					}

					if (!sContinueLogging) continue;

					// If we were able to successfully pop an element => handle it, i.e. print it
					if (!empty)
					{
						// ACTUAL LOGGING:
						avk::set_console_output_color(front.mLogType, front.mLogImportance);
						std::cout << front.mMessage;
						if (!front.mStacktrace.empty()) {
							avk::set_console_output_color_for_stacktrace(front.mLogType, front.mLogImportance);
							std::cout << front.mStacktrace;
						}
						avk::reset_console_output_color();

						// 2nd lock => get correct empty-value
						{
							std::scoped_lock<std::mutex> guard(sLogMutex);
							empty = sLogQueue.empty();
							if (!empty) continue;
						}
					}
				}

				// No more messages => wait
				{
					std::unique_lock<std::mutex> lock(sLogMutex);
					if (!sContinueLogging) {
						break;
					}
					
					sCondVar.wait(lock);
				}
			}

			avk::set_console_output_color(avk::log_type::system, avk::log_importance::important);
			std::cout << "Logger thread terminating.";
			avk::reset_console_output_color();
			std::cout << std::endl;
		});
		
		static struct thread_stopper {
			~thread_stopper() {
				{
					std::unique_lock<std::mutex> lock(sLogMutex);
					sContinueLogging = false;
				}
				sCondVar.notify_one();
				sLogThread.join();
			}
		} sThreadStopper;

		// Enqueue the message and wake the logger thread
		{
			std::scoped_lock<std::mutex> guard(sLogMutex);
#if defined(_WIN32) && defined (_DEBUG) && defined (PRINT_STACKTRACE)
			if (pToBeLogged.mLogType == log_type::error || pToBeLogged.mLogType == log_type::warning) {
				pToBeLogged.mStacktrace = get_current_callstack();
			}
#endif
			sLogQueue.push(pToBeLogged);
		}
		sCondVar.notify_one();
	}
#else
	void dispatch_log(log_pack pToBeLogged)
	{
		avk::set_console_output_color(pToBeLogged.mLogType, pToBeLogged.mLogImportance);
		std::cout << pToBeLogged.mMessage;
#if defined(_WIN32) && defined (_DEBUG) && defined (PRINT_STACKTRACE)
		if (pToBeLogged.mLogType == log_type::error /*|| pToBeLogged.mLogType == log_type::warning*/) {
			set_console_output_color_for_stacktrace(pToBeLogged.mLogType, pToBeLogged.mLogImportance);
			std::cout << get_current_callstack();
		}
#endif
		avk::reset_console_output_color();
	}
#endif

	std::string to_string(const glm::mat4& pMatrix)
	{
		char buf[256];
    std::snprintf(
      buf, 256,
			"\t%.3f\t%.3f\t%.3f\t%.3f\n\t%.3f\t%.3f\t%.3f\t%.3f\n\t%.3f\t%.3f\t%.3f\t%.3f\n\t%.3f\t%.3f\t%.3f\t%.3f\n",
			pMatrix[0][0], pMatrix[0][1], pMatrix[0][2], pMatrix[0][3],
			pMatrix[1][0], pMatrix[1][1], pMatrix[1][2], pMatrix[1][3],
			pMatrix[2][0], pMatrix[2][1], pMatrix[2][2], pMatrix[2][3],
			pMatrix[3][0], pMatrix[3][1], pMatrix[3][2], pMatrix[3][3]);
		return buf;
	}

	std::string to_string(const glm::mat3& pMatrix)
	{
		char buf[256];
		std::snprintf(
      buf, 256,
			"\t%.3f\t%.3f\t%.3f\n\t%.3f\t%.3f\t%.3f\n\t%.3f\t%.3f\t%.3f\n",
			pMatrix[0][0], pMatrix[0][1], pMatrix[0][2],
			pMatrix[1][0], pMatrix[1][1], pMatrix[1][2],
			pMatrix[2][0], pMatrix[2][1], pMatrix[2][2]);
		return buf;
	}

	std::string to_string_compact(const glm::mat4& pMatrix)
	{
		char buf[256];
		std::snprintf(
      buf, 256,
			"{{%.2f, %.2f, %.2f, %.2f}, {%.2f, %.2f, %.2f, %.2f}, {%.2f, %.2f, %.2f, %.2f}, {%.2f, %.2f, %.2f, %.2f}}\n",
			pMatrix[0][0], pMatrix[0][1], pMatrix[0][2], pMatrix[0][3],
			pMatrix[1][0], pMatrix[1][1], pMatrix[1][2], pMatrix[1][3],
			pMatrix[2][0], pMatrix[2][1], pMatrix[2][2], pMatrix[2][3],
			pMatrix[3][0], pMatrix[3][1], pMatrix[3][2], pMatrix[3][3]);
		return buf;
	}

	std::string to_string_compact(const glm::mat3& pMatrix)
	{
		char buf[256];
		std::snprintf(
      buf, 256,
			"{{%.2f, %.2f, %.2f}, {%.2f, %.2f, %.2f}, {%.2f, %.2f, %.2f}}\n",
			pMatrix[0][0], pMatrix[0][1], pMatrix[0][2],
			pMatrix[1][0], pMatrix[1][1], pMatrix[1][2],
			pMatrix[2][0], pMatrix[2][1], pMatrix[2][2]);
		return buf;
	}


	std::string to_string(const glm::vec2& pVector)
	{
		char buf[64];
		std::snprintf(
      buf, 64, "(%.2f, %.2f)", pVector.x, pVector.y);
		return buf;
	}

	std::string to_string(const glm::vec3& pVector)
	{
		char buf[64];
		std::snprintf(
      buf, 64, "(%.2f, %.2f, %.2f)", pVector.x, pVector.y, pVector.z);
		return buf;
	}

	std::string to_string(const glm::vec4& pVector)
	{
		char buf[64];
		std::snprintf(
      buf, 64, "(%.2f, %.2f, %.2f, %.2f)", pVector.x, pVector.y, pVector.z, pVector.w);
		return buf;
	}

	std::string get_current_callstack()
	{
#if defined(_WIN32) && defined (_DEBUG) && defined (PRINT_STACKTRACE)
		void         * stack[100];

		HANDLE process = GetCurrentProcess();

		SymInitialize(process, nullptr, TRUE);

		unsigned short frames = CaptureStackBackTrace(0, 100, stack, nullptr);
		SYMBOL_INFO * symbol = static_cast<SYMBOL_INFO *>(calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1));
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		auto callstack = std::string();
		for (unsigned int i = 0; i < frames; i++)
		{
			SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), nullptr, symbol);

			std::stringstream ss;
			ss << frames - i - 1 << ": " << symbol->Name << " (0x" << std::hex << symbol->Address << ")" << std::endl;
			callstack += ss.str();

			if (strcmp(symbol->Name, "main") == 0)
			{
				break;
			}
		}

		free(symbol);
		return callstack;
#else
		return std::string();
#endif
	}
}

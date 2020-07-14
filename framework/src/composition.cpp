#include <exekutor.hpp>

namespace xk
{
	std::mutex composition::sCompMutex{};
	std::condition_variable composition::sInputBufferCondVar{};
}

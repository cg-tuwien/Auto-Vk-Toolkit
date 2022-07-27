#include <auto_vk_toolkit.hpp>

namespace avk
{
	std::mutex composition::sCompMutex{};
	std::condition_variable composition::sInputBufferCondVar{};
}

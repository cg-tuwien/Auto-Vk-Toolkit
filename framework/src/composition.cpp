#include <gvk.hpp>

namespace gvk
{
	std::mutex composition::sCompMutex{};
	std::condition_variable composition::sInputBufferCondVar{};
}

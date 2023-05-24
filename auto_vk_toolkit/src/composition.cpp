#include "composition.hpp"

namespace avk
{
	std::mutex composition::sCompMutex{};
	std::condition_variable composition::sInputBufferCondVar{};
}

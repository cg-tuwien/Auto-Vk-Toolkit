#pragma once
#include <exekutor.hpp>

namespace xk
{
	inline const std::array<float, 16>& to_array(const glm::mat4& aMatrix)
	{
		return *reinterpret_cast<const std::array<float, 16>*>( glm::value_ptr(aMatrix) );
	}
}

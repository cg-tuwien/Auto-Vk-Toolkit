#include "bezier_curve.hpp"
#include "math_utils.hpp"

namespace avk
{
	bezier_curve::bezier_curve(std::vector<glm::vec3> pControlPoints)
		: cp_interpolation{ std::move(pControlPoints) }
	{

	}

	glm::vec3 bezier_curve::value_at(float t)
	{
		uint32_t n = static_cast<uint32_t>(num_control_points()) - 1u;
		glm::vec3 sum(0.0f, 0.0f, 0.0f);
		for (uint32_t i = 0; i <= n; ++i)
		{
			sum += bernstein_polynomial(i, n, t) * control_point_at(i);
		}
		return sum;
	}

	glm::vec3 bezier_curve::slope_at(float t)
	{
		uint32_t n = static_cast<uint32_t>(num_control_points()) - 1u;
		uint32_t nMinusOne = n - 1;
		glm::vec3 sum(0.0f, 0.0f, 0.0f);
		for (uint32_t i = 0; i <= nMinusOne; ++i)
		{
			sum += (control_point_at(i + 1) - control_point_at(i)) * bernstein_polynomial(i, nMinusOne, t);
		}
		return static_cast<float>(n) * sum;
	}
}

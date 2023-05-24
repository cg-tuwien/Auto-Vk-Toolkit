#pragma once

#include "cp_interpolation.hpp"

namespace avk
{
	class quadratic_uniform_b_spline : public cp_interpolation
	{
	public:
		quadratic_uniform_b_spline() = default;
		// Initializes a instance with a set of control points
		quadratic_uniform_b_spline(std::vector<glm::vec3> pControlPoints);
		quadratic_uniform_b_spline(quadratic_uniform_b_spline&&) = default;
		quadratic_uniform_b_spline(const quadratic_uniform_b_spline&) = default;
		quadratic_uniform_b_spline& operator=(quadratic_uniform_b_spline&&) = default;
		quadratic_uniform_b_spline& operator=(const quadratic_uniform_b_spline&) = default;
		~quadratic_uniform_b_spline() = default;

		glm::vec3 value_at(float t) override;

		glm::vec3 slope_at(float t) override;

	private:
		std::array<glm::vec3, 3> points_for_interpolation(int32_t i);
		glm::vec3 calculate_value(int32_t i, float u);
		glm::vec3 calculate_slope(int32_t i, float u);

		static std::array<float, 3> Mr0;
		static std::array<float, 3> Mr1;
		static std::array<float, 3> Mr2;
	};
}

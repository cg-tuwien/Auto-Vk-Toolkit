#pragma once

#include "cp_interpolation.hpp"

namespace avk
{
	class cubic_uniform_b_spline : public cp_interpolation
	{
	public:
		cubic_uniform_b_spline() = default;
		// Initializes a instance with a set of control points
		cubic_uniform_b_spline(std::vector<glm::vec3> pControlPoints);
		cubic_uniform_b_spline(cubic_uniform_b_spline&&) = default;
		cubic_uniform_b_spline(const cubic_uniform_b_spline&) = default;
		cubic_uniform_b_spline& operator=(cubic_uniform_b_spline&&) = default;
		cubic_uniform_b_spline& operator=(const cubic_uniform_b_spline&) = default;
		~cubic_uniform_b_spline() = default;

		glm::vec3 value_at(float t) override;

		glm::vec3 slope_at(float t) override;

	private:
		std::array<glm::vec3, 4> points_for_interpolation(int32_t i);
		glm::vec3 calculate_value(int32_t i, float u);
		glm::vec3 calculate_slope(int32_t i, float u);

		static std::array<float, 4> Mr0;
		static std::array<float, 4> Mr1;
		static std::array<float, 4> Mr2;
		static std::array<float, 4> Mr3;
	};
}

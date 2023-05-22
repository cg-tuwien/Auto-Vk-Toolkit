#include "quadratic_uniform_b_spline.hpp"

namespace avk
{
	std::array<float, 3> quadratic_uniform_b_spline::Mr0{ {   .5f, -1.f,   .5f } };
	std::array<float, 3> quadratic_uniform_b_spline::Mr1{ { -1.f,   1.f,  0.f } };
	std::array<float, 3> quadratic_uniform_b_spline::Mr2{ {   .5f,   .5f, 0.f } };

	quadratic_uniform_b_spline::quadratic_uniform_b_spline(std::vector<glm::vec3> pControlPoints)
		: cp_interpolation{ std::move(pControlPoints) }
	{

	}

	glm::vec3 quadratic_uniform_b_spline::value_at(float t)
	{
		auto u = t * static_cast<float>(num_control_points());
		int32_t i = static_cast<int32_t>(u);
		u = u - i;
		return calculate_value(i, u);
	}

	glm::vec3 quadratic_uniform_b_spline::slope_at(float t)
	{
		auto u = t * static_cast<float>(num_control_points());
		int32_t i = static_cast<int32_t>(u);
		u = u - i;
		return calculate_slope(i, u);
	}

	std::array<glm::vec3, 3> quadratic_uniform_b_spline::points_for_interpolation(int32_t i)
	{
		std::array<glm::vec3, 3> points{};
		int p=0;
		for (int32_t x = i - 1; x <= i + 1; ++x)
		{
			if (x <= 0) {
				points[p++] = control_point_at(0);
			}
			else if (x >= num_control_points()) {
				points[p++] = control_point_at(num_control_points() - 1);
			}
			else {
				points[p++] = control_point_at(x);
			}
		}
		return points;
	}

	glm::vec3 quadratic_uniform_b_spline::calculate_value(int32_t i, float u)
	{
		auto points = points_for_interpolation(i);
		auto q_i = 
			(u*u * (Mr0[0] * points[0] + Mr0[1] * points[1]   + Mr0[2] * points[2]))
			+ (u * (Mr1[0] * points[0] + Mr1[1] * points[1] /*+ Mr1[2] * points[2]*/))   // Run-time optimization for Matrix entries which are 0
			+ (    (Mr2[0] * points[0] + Mr2[1] * points[1] /*+ Mr2[2] * points[2]*/));
		return q_i;
	}

	glm::vec3 quadratic_uniform_b_spline::calculate_slope(int32_t i, float u)
	{
		auto points = points_for_interpolation(i);
		auto s_i = 
			(2*u * (Mr0[0] * points[0] + Mr0[1] * points[1]   + Mr0[2] * points[2]))
			+ (    (Mr1[0] * points[0] + Mr1[1] * points[1] /*+ Mr1[2] * points[2]*/));
		return s_i;
	}
}

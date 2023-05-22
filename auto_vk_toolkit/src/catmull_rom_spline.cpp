#include "catmull_rom_spline.hpp"

namespace avk
{
	catmull_rom_spline::catmull_rom_spline(std::vector<glm::vec3> pControlPoints)
		: cp_interpolation()
	{
		pControlPoints.insert(std::begin(pControlPoints), pControlPoints.front());
		pControlPoints.push_back(pControlPoints.back());
		set_control_points(std::move(pControlPoints));
	}

	glm::vec3 catmull_rom_spline::value_at(float t)
	{
		uint32_t numSections = static_cast<uint32_t>(num_control_points() - 3);
		float tns = t * numSections;
		uint32_t section = glm::min(static_cast<uint32_t>(tns), numSections - 1);
		float u = tns - static_cast<float>(section);

		glm::vec3 a = control_point_at(section);
		glm::vec3 b = control_point_at(section + 1);
		glm::vec3 c = control_point_at(section + 2);
		glm::vec3 d = control_point_at(section + 3);

		return .5f * (
						(-a + 3.f * b - 3.f * c + d)		* (u * u * u)
						+ (2.f * a - 5.f * b + 4.f * c - d) * (u * u)
						+ (-a + c)							* u
						+ 2.f * b
					);
	}

	glm::vec3 catmull_rom_spline::slope_at(float t)
	{
		uint32_t numSections = static_cast<uint32_t>(num_control_points() - 3);
		float tns = t * numSections;
		uint32_t section = glm::min(static_cast<uint32_t>(tns), numSections - 1);
		float u = tns - static_cast<float>(section);

		glm::vec3 a = control_point_at(section);
		glm::vec3 b = control_point_at(section + 1);
		glm::vec3 c = control_point_at(section + 2);
		glm::vec3 d = control_point_at(section + 3);

		return 1.5f * (-a + 3.f * b - 3.f * c + d)     * (u * u)
					+ (2.f * a -5.f * b + 4.f * c - d)  * u
					+ .5f * c - .5f * a;
	}

	float catmull_rom_spline::arc_length()
	{
		return arc_length_between_control_points(1, num_control_points() - 2);
	}
}

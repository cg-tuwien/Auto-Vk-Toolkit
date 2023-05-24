#pragma once

#include "cp_interpolation.hpp"

namespace avk
{
	class catmull_rom_spline : public cp_interpolation
	{
	public:
		catmull_rom_spline() = default;
		// Initializes a instance with a set of control points
		catmull_rom_spline(std::vector<glm::vec3> pControlPoints);
		catmull_rom_spline(catmull_rom_spline&&) = default;
		catmull_rom_spline(const catmull_rom_spline&) = default;
		catmull_rom_spline& operator=(catmull_rom_spline&&) = default;
		catmull_rom_spline& operator=(const catmull_rom_spline&) = default;
		~catmull_rom_spline() = default;

		glm::vec3 value_at(float t) override;

		glm::vec3 slope_at(float t) override;

		float arc_length() override;
	};
}


#include "quake_camera.hpp"
#include "composition_interface.hpp"
#include "timer_interface.hpp"

namespace avk
{
	quake_camera::quake_camera(std::string aName, bool aIsEnabled)
		: invokee(std::move(aName), aIsEnabled)
		, mRotationSpeed(0.001f)
		, mMoveSpeed(4.5f) // 4.5 m/s
		, mFastMultiplier(6.0f) // 27 m/s
		, mSlowMultiplier(0.2f) // 0.9 m/s
	{
	}
	
	void quake_camera::on_enable()
	{
		composition_interface::current()->input().set_cursor_mode(cursor::cursor_disabled_raw_input);
	}

	void quake_camera::on_disable()
	{
		composition_interface::current()->input().set_cursor_mode(cursor::arrow_cursor);
	}

	void quake_camera::update()
	{
		static const auto input = []()->input_buffer& { return composition_interface::current()->input(); };
		// display info about myself
		if (input().key_pressed(key_code::i)
			&& (input().key_down(key_code::left_control) || input().key_down(key_code::right_control))) {
			LOG_INFO(std::format("quake_camera's position: {}", to_string(translation())));
			LOG_INFO(std::format("quake_camera's view-dir: {}", to_string(front(*this))));
			LOG_INFO(std::format("quake_camera's up-vec:   {}", to_string(up(*this))));
			LOG_INFO(std::format("quake_camera's position and orientation:\n{}", to_string(mMatrix)));
			LOG_INFO(std::format("quake_camera's view-mat:\n{}", to_string(view_matrix())));
		}

		auto deltaCursor = input().delta_cursor_position();
		auto deltaTime = time().delta_time();

		// query the position of the mouse cursor
		auto mousePos = input().cursor_position();
		//LOG_INFO(std::format("mousePos[{},{}]", mousePos.x, mousePos.y));

		// calculate how much the cursor has moved from the center of the screen
		auto mouseMoved = deltaCursor;
		//LOG_INFO_EM(std::format("mouseMoved[{},{}]", mouseMoved.x, mouseMoved.y));

		// accumulate values and create rotation-matrix
		glm::quat rotHoriz = glm::angleAxis(mRotationSpeed * static_cast<float>(mouseMoved.x), glm::vec3(0.f, 1.f, 0.f));
		glm::quat rotVert =  glm::angleAxis(mRotationSpeed * static_cast<float>(mouseMoved.y), glm::vec3(1.f, 0.f, 0.f));
		set_rotation(rotHoriz * rotation() * rotVert);

		// move camera to new position
		if (input().key_down(key_code::w)) {
			translate_myself(front(*this), deltaTime);
		}
		if (input().key_down(key_code::s)) {
			translate_myself(back(*this), deltaTime);
		}
		if (input().key_down(key_code::d)) {
			translate_myself(right(*this), deltaTime);
		}
		if (input().key_down(key_code::a)) {
			translate_myself(left(*this), deltaTime);
		}
		if (input().key_down(key_code::e)) {
			translate_myself(up(*this), deltaTime);
		}
		if (input().key_down(key_code::q)) {
			translate_myself(down(*this), deltaTime);
		}
	}
	
	void quake_camera::translate_myself(const glm::vec3& translation, double deltaTime)
	{
		float speedMultiplier = 1.0f;
		if (composition_interface::current()->input().key_down(key_code::left_shift)) {
			speedMultiplier = mFastMultiplier;
		}
		if (composition_interface::current()->input().key_down(key_code::left_control)) {
			speedMultiplier = mSlowMultiplier;
		}
		translate(*this, mMoveSpeed * speedMultiplier * static_cast<float>(deltaTime) * translation);
	}
}

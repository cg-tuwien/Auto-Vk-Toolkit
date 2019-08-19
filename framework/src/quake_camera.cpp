#include <cg_base.hpp>

namespace cgb
{
	quake_camera::quake_camera()
		: mRotationSpeed(0.001f)
		, mMoveSpeed(4.5f) // 4.5 m/s
		, mFastMultiplier(6.0f) // 27 m/s
		, mSlowMultiplier(0.2f) // 0.9 m/s
	{
	}

	quake_camera::~quake_camera()
	{
	}

	void quake_camera::on_enable()
	{
		input().set_cursor_disabled(true);
	}

	void quake_camera::on_disable()
	{
		input().set_cursor_disabled(false);
	}

	void quake_camera::update()
	{
		// display info about myself
		if (input().key_pressed(key_code::i)
			&& (input().key_down(key_code::left_control) || input().key_down(key_code::right_control))) {
			LOG_INFO(fmt::format("quake_camera's position: {}", to_string(translation())));
			LOG_INFO(fmt::format("quake_camera's view-dir: {}", to_string(front(*this))));
			LOG_INFO(fmt::format("quake_camera's up-vec:   {}", to_string(up(*this))));
			LOG_INFO(fmt::format("quake_camera's position and orientation:\n{}", to_string(mMatrix)));
			LOG_INFO(fmt::format("quake_camera's view-mat:\n{}", to_string(view_matrix())));
		}

		auto deltaCursor = input().delta_cursor_position();
		auto deltaTime = time().delta_time();

		// query the position of the mouse cursor
		auto mousePos = input().cursor_position();
		//LOG_INFO(fmt::format("mousePos[{},{}]", mousePos.x, mousePos.y));

		// calculate how much the cursor has moved from the center of the screen
		auto mouseMoved = deltaCursor;
		//LOG_INFO_EM(fmt::format("mouseMoved[{},{}]", mouseMoved.x, mouseMoved.y));

		// accumulate values and create rotation-matrix
		glm::quat rotHoriz = glm::quat_cast(glm::rotate(mRotationSpeed * static_cast<float>(mouseMoved.x), glm::vec3(0.f, 1.f, 0.f)));
		glm::quat rotVert =  glm::quat_cast(glm::rotate(mRotationSpeed * static_cast<float>(mouseMoved.y), glm::vec3(1.f, 0.f, 0.f)));
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
		if (input().key_down(key_code::left_shift)) {
			speedMultiplier = mFastMultiplier;
		}
		if (input().key_down(key_code::left_control)) {
			speedMultiplier = mSlowMultiplier;
		}
		translate(*this, mMoveSpeed * speedMultiplier * static_cast<float>(deltaTime) * translation);
	}
}

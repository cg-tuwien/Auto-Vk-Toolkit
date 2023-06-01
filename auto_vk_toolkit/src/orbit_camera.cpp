#include "orbit_camera.hpp"
#include "composition_interface.hpp"
#include "timer_interface.hpp"
#include <glm/gtx/quaternion.hpp>

namespace avk
{
	orbit_camera::orbit_camera(std::string aName, bool aIsEnabled)
		: invokee(std::move(aName), aIsEnabled)
		, mRotationSpeed(0.01f)
		, mPivotDistance{ 10.f }
		, mPivotDistanceSpeed{ .5f }
		, mMinPivotDistance{ 1.f }
		, mMaxPivotDistance{ 30.f }
		, mPivotDistanceSlowDownRange{ 10.f }
		, mLateralSpeed{ 1.f }
		, mFastMultiplier(6.0f)
		, mSlowMultiplier(0.2f)
	{
	}

	void orbit_camera::on_enable()
	{
		composition_interface::current()->input().set_cursor_mode(cursor::hand_cursor);
		calculate_lateral_speed();
	}

	void orbit_camera::on_disable()
	{
		composition_interface::current()->input().set_cursor_mode(cursor::arrow_cursor);
	}

	void orbit_camera::update()
	{
		static const auto input = []() -> input_buffer& { return composition_interface::current()->input(); };

		auto deltaCursor = input().delta_cursor_position();
		auto deltaTime = time().delta_time();

		// query the position of the mouse cursor
		auto mousePos = input().cursor_position();
		//LOG_INFO(fmt::format("mousePos[{},{}]", mousePos.x, mousePos.y));

		// calculate how much the cursor has moved from the center of the screen
		auto mouseMoved = deltaCursor;
		//LOG_INFO_EM(fmt::format("mouseMoved[{},{}]", mouseMoved.x, mouseMoved.y));

		constexpr uint8_t LMB = 0;
		constexpr uint8_t RMB = 1;
		if (input().mouse_button_down(LMB)) {
			const auto rotSpeed = mRotationSpeed
		        * ((input().key_down(key_code::left_shift)   || input().key_down(key_code::right_shift))   ? mFastMultiplier : 1.f)
			    * ((input().key_down(key_code::left_control) || input().key_down(key_code::right_control)) ? mSlowMultiplier : 1.f);
			glm::quat rotHoriz = glm::angleAxis( rotSpeed * static_cast<float>(mouseMoved.x), glm::vec3(0.f, 1.f, 0.f));
			glm::quat rotVert =  glm::angleAxis(-rotSpeed * static_cast<float>(mouseMoved.y), glm::vec3(1.f, 0.f, 0.f));
			const auto oldPos = back(*this) * mPivotDistance;
			const auto newPos = rotHoriz * oldPos * rotVert;
			translate(*this, newPos - oldPos);
			look_along(-newPos);
		}
		if (input().mouse_button_down(RMB)) {
			const auto latSpeed = mLateralSpeed * 0.1f
			    * ((input().key_down(key_code::left_shift)   || input().key_down(key_code::right_shift))   ? mFastMultiplier : 1.f)
			    * ((input().key_down(key_code::left_control) || input().key_down(key_code::right_control)) ? mSlowMultiplier : 1.f);

			const auto t
		        = down(*this)  * static_cast<float>(deltaCursor.y) * latSpeed
		        + right(*this) * static_cast<float>(deltaCursor.x) * latSpeed;
			translate(*this, t);
		}

		const auto scrollDist = static_cast<float>(input().scroll_delta().y);
		const auto pivDistSpeed = mPivotDistanceSpeed
			* ((input().key_down(key_code::left_shift)   || input().key_down(key_code::right_shift))   ? mFastMultiplier : 1.f)
			* ((input().key_down(key_code::left_control) || input().key_down(key_code::right_control)) ? mSlowMultiplier : 1.f);

		if (input().mouse_button_down(RMB) || input().key_down(key_code::left_alt) || input().key_down(key_code::right_alt)) {
			// Move pivot along with the camera
			translate(*this, front(*this) * scrollDist * pivDistSpeed);
			// ...and leave mPivotDistance unchanged.
		}
		else {
			// Move camera towards/away from camera
		    const auto moveCloser = scrollDist > 0.f;
		    const auto moveAway   = scrollDist < 0.f;

		    if (moveCloser) {
			    auto len = glm::smoothstep(mMinPivotDistance, mMinPivotDistance + mPivotDistanceSlowDownRange, mPivotDistance);
			    auto move = front(*this) * len * pivDistSpeed;
			    translate(*this, move);
			    mPivotDistance -= len * pivDistSpeed;
				calculate_lateral_speed();
			}
		    if (moveAway) {
			    auto len = glm::smoothstep(mMaxPivotDistance, mMaxPivotDistance - mPivotDistanceSlowDownRange, mPivotDistance);
			    auto move = back(*this) * len * pivDistSpeed;
			    translate(*this, move);
			    mPivotDistance += len * pivDistSpeed;
				calculate_lateral_speed();
			}
		}
	}

	void orbit_camera::calculate_lateral_speed()
	{
		const auto* wnd = context().main_window();
		auto resy = nullptr != wnd ? static_cast<float>(wnd->resolution().y) : 1000.f;
		resy *= 0.5f;
		mLateralSpeed = glm::abs(mPivotDistance / projection_matrix()[1][1]) / resy;
		LOG_INFO(fmt::format("mPivotDistance[{}], mLateralSpeed[{}]", mPivotDistance - mNear, mLateralSpeed));
	}
}

#include "orbit_camera.hpp"
#include "composition_interface.hpp"
#include "timer_interface.hpp"
#include <glm/gtx/quaternion.hpp>

namespace avk
{
	orbit_camera::orbit_camera(std::string aName, bool aIsEnabled)
		: invokee(std::move(aName), aIsEnabled)
		, mRotationSpeed(0.002f)
		, mPivotDistance{ 10.f }
		, mPivotDistanceSpeed{ .5f }
		, mZoomSpeed{ 0.07f }
		, mLateralSpeed{ 1.f }
		, mFastMultiplier(5.0f)
		, mSlowMultiplier(0.1f)
	{
	}

	void orbit_camera::on_enable()
	{
		calculate_lateral_speed();
	}

	void orbit_camera::on_disable()
	{
	}

	void orbit_camera::update()
	{
		static const auto input = []() -> input_buffer& { return composition_interface::current()->input(); };

		auto deltaCursor = input().delta_cursor_position();
		auto deltaTime = time().delta_time();

		// query the position of the mouse cursor
		auto mousePos = input().cursor_position();

		// calculate how much the cursor has moved from the center of the screen
		auto mouseMoved = deltaCursor;

		constexpr uint8_t LMB = 0;
		constexpr uint8_t RMB = 1;
		if (input().mouse_button_down(LMB)) {
			auto rotSpeed = mRotationSpeed
		        * ((input().key_down(key_code::left_shift)   || input().key_down(key_code::right_shift))   ? mFastMultiplier : 1.f)
			    * ((input().key_down(key_code::left_control) || input().key_down(key_code::right_control)) ? mSlowMultiplier : 1.f);

			glm::quat rotHoriz = glm::angleAxis(rotSpeed * static_cast<float>(mouseMoved.x), up());

			auto mouseMoveRot  = rotSpeed * static_cast<float>(mouseMoved.y);
			auto maxAllowdRot  = 1.0f - glm::dot(up(), back(*this));
			glm::quat rotVert  =  glm::angleAxis(mouseMoveRot, right(*this));
		    const auto oldPos  = back(*this) * mPivotDistance;
			const auto rotQuat = (rotHoriz * rotVert);
			const auto newPos  = rotQuat * oldPos;
		    // With everything at hand, translate and rotate:	
		    translate(*this, newPos - oldPos);
		    set_rotation(rotQuat * rotation()); // <-- Rotate existing rotation
		}
		if (input().mouse_button_down(RMB)) {
			const auto latSpeed = mLateralSpeed
			    * ((input().key_down(key_code::left_shift)   || input().key_down(key_code::right_shift))   ? mFastMultiplier : 1.f)
			    * ((input().key_down(key_code::left_control) || input().key_down(key_code::right_control)) ? mSlowMultiplier : 1.f);

			const auto t
		        = down(*this)  * static_cast<float>(deltaCursor.y) * latSpeed.y
		        + right(*this) * static_cast<float>(deltaCursor.x) * latSpeed.x;
			translate(*this, t);
		}

		const auto scrollDist = static_cast<float>(input().scroll_delta().y);
		const auto pivDistSpeed = mPivotDistanceSpeed
			* ((input().key_down(key_code::left_shift)   || input().key_down(key_code::right_shift))   ? mFastMultiplier : 1.f)
			* ((input().key_down(key_code::left_control) || input().key_down(key_code::right_control)) ? mSlowMultiplier : 1.f);

		if (input().key_down(key_code::left_alt) || input().key_down(key_code::right_alt)) {
			// Move pivot along with the camera
			translate(*this, front(*this) * scrollDist * pivDistSpeed);
			// ...and leave mPivotDistance unchanged.
		}
		else {
			// Move camera towards/away from camera
		    const auto moveCloser = scrollDist > 0.f;
		    const auto moveAway   = scrollDist < 0.f;

			if (scrollDist != 0.0f) {
				auto moveAwayFactor = 1.f + mZoomSpeed;
				auto moveCloserFactor = moveCloser ? 1.f + mZoomSpeed * glm::smoothstep(0.0f, 1.0f, mPivotDistance) : 1.0f;
				auto newPos = scrollDist < 0.0
					? mPivotDistance * moveAwayFactor
					: mPivotDistance / moveCloserFactor;
				translate(*this, front(*this) * (mPivotDistance - newPos));
				set_pivot_distance(newPos);
			}
		}
	}

	void orbit_camera::set_pivot_distance(float aDistanceFromCamera) {
		mPivotDistance = aDistanceFromCamera;
		calculate_lateral_speed();
		mPivotDistanceSpeed = mPivotDistance / 20.f;
	}

	float orbit_camera::pivot_distance() const {
		return mPivotDistance;
	}

	void orbit_camera::calculate_lateral_speed()
	{
		const auto* wnd = context().main_window();
		auto resi = nullptr != wnd ? wnd->resolution() : glm::uvec2{1920, 1080};
		mLateralSpeed = glm::vec2{                                              // This  v  accounts for different FOVs
			mPivotDistance / static_cast<float>(resi.x) / (near_plane_distance() * -projection_matrix()[1][1]),
			mPivotDistance / static_cast<float>(resi.x) / (near_plane_distance() * -projection_matrix()[1][1])
		};                     // No idea why there  ^  must .x for both axes. *shrug*
	}
}

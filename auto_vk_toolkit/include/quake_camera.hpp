#pragma once

#include "camera.hpp"
#include "sequential_invoker.hpp"

namespace avk
{
	// A flying camera, controlled in a somewhat quake-style.
	// Move the camera with WASD + Mouse!
	// Additional keys: 
	//  - E     = move up
	//  - Q     = move down
	//  - Shift = move faster
	//  - Ctrl  = move slower
	class quake_camera : public camera, public invokee
	{
	public:
		quake_camera(std::string aName = "quake_camera", bool aIsEnabled = true);
		quake_camera(quake_camera&&) noexcept = default;
		quake_camera(const quake_camera&) noexcept = default;
		quake_camera& operator=(quake_camera&&) noexcept = default;
		quake_camera& operator=(const quake_camera&) noexcept = default;
		~quake_camera() override = default;

		// Invoked when this camera becomes active
		void on_enable() override;
		// Invoked when this camera becomes inactive
		void on_disable() override;
		// Invoked every frame to handle input and update the camera's position
		void update() override;

		// Returns the currently set rotation speed (Mouse movement)
		float rotation_speed() const { return mRotationSpeed; }
		// Returns the currently set movement speed (WASD,EQ keys)
		float move_speed() const { return mMoveSpeed; }
		// Returns the currently set multiplyer for fast movement (Shift key)
		float fast_multiplier() const { return mFastMultiplier; }
		// Returns the currently set multiplier for slow movement (Ctrl key)
		float slow_multiplier() const { return mSlowMultiplier; }

		// Sets a new rotation speed (Mouse movement)
		void set_rotation_speed(float value) { mRotationSpeed = value; }
		// Sets a new movement speed (WASD,EQ keys)
		void set_move_speed(float value) { mMoveSpeed = value; }
		// Sets a new multiplier for fast movement (Shift key)
		void set_fast_multiplier(float value) { mFastMultiplier = value; }
		// Sets a new multiplier for slow movement (Ctrl key)
		void set_slow_multiplier(float value) { mSlowMultiplier = value; }

	protected:
		// Translates this transform (base class of this class' base camera) in a given direction
		void translate_myself(const glm::vec3& translation, double deltaTime);

		float mRotationSpeed;
		float mMoveSpeed;
		float mFastMultiplier;
		float mSlowMultiplier;
	};
}

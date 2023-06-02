#pragma once

#include "camera.hpp"
#include "sequential_invoker.hpp"

namespace avk
{
	// An orbit camera or arcball camera, mostly by clicking and dragging the mouse.
	// Controls
	//  - Left click down ............... Camera rotates around rotation center if the mouse is moved
	//  - Right click down .............. Camera and rotation center is moved with the mouse movements.
	//  - Scroll ........................ Move towards or away from rotation center.
	//  - Alt || right click + scroll ... Move camera and rotation center in forward/back direction.
	//  - Ctrl .......................... Slower movements
	//  - Shift ......................... Faster movements
	class orbit_camera : public camera, public invokee
	{
	public:
		orbit_camera(std::string aName = "orbit_camera", bool aIsEnabled = true);
		orbit_camera(orbit_camera&&) noexcept = default;
		orbit_camera(const orbit_camera&) noexcept = default;
		orbit_camera& operator=(orbit_camera&&) noexcept = default;
		orbit_camera& operator=(const orbit_camera&) noexcept = default;
		~orbit_camera() override = default;

		// Invoked when this camera becomes active
		void on_enable() override;
		// Invoked when this camera becomes inactive
		void on_disable() override;
		// Invoked every frame to handle input and update the camera's position
		void update() override;

		/**	Sets the pivot distance, which is the distance along the front vector
		 *	of the camera which this camera orbits around.
		 *	Note: The passed value might get clamped to min/max bounds.
		 *	@param aDistanceFromCamera	The desired distance along the front vector.
		 */
		void set_pivot_distance(float aDistanceFromCamera);

        /** Gets the current pivot distance
         *  @return The currently used pivot distance.
         */
        float pivot_distance() const;

	private:
		void calculate_lateral_speed();

	protected:
		float mRotationSpeed;
		float mPivotDistance;
		float mPivotDistanceSpeed;
		float mMinPivotDistance;
		float mMaxPivotDistance;
		float mPivotDistanceSlowDownRange;
		glm::vec2 mLateralSpeed;
		float mFastMultiplier;
		float mSlowMultiplier;
	};
}

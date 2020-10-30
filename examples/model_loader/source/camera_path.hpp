#pragma once
#include <gvk.hpp>

class camera_path : public gvk::invokee
{
public:
	camera_path(gvk::quake_camera& cam)
		: mCam{ &cam } // Target camera
		, mSpeed{ 0.1f } // How fast does it move
		, mStartTime{} // Set in initialize()
	{ 
		// The path to follow
		mPath = std::make_unique<gvk::bezier_curve>(std::vector<glm::vec3>{ 
			glm::vec3( 10.0f, 15.0f,  -10.0f),
			glm::vec3(-10.0f,  5.0f,  - 5.0f),
			glm::vec3(  0.0f,  0.0f,    0.0f),
			glm::vec3( 10.0f, 15.0f,   10.0f),
			glm::vec3( 10.0f, 10.0f,    0.0f)
		});
	}

	int execution_order() const override
	{
		return 5; // run after quake_camera, s.t. we do not get any jittering from mouse movements
	}

	void initialize() override 
	{
		mStartTime = gvk::time().time_since_start();
	}

	void update() override
	{
		auto t = (gvk::time().time_since_start() - mStartTime) * mSpeed;
		if (t >= 0.0f && t <= 1.0f) {
			mCam->set_translation(mPath->value_at(t));
			mCam->look_along(mPath->slope_at(t));
		}
	}

private:
	gvk::quake_camera* mCam;
	float mSpeed;
	float mStartTime;
	std::unique_ptr<gvk::cp_interpolation> mPath;
};

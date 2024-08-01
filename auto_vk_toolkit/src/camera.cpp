
#include "camera.hpp"

namespace avk
{
	camera::camera()
		: mProjectionMatrix{ 1.0f }
		, mProjectionType{ projection_type::unknown }
		, mNear{ 0 }
		, mFar{ 0 }
		, mFov{ 0.0f }
		, mAspect{ 0.0 }
		, mLeft{ 0 }
		, mRight{ 0 }
		, mTop{ 0 }
		, mBottom{ 0 }
	{}
	
    camera& camera::set_projection_matrix(const glm::mat4& aMatrix, avk::projection_type aProjectionType)
	{
		mProjectionType = aProjectionType;
		mProjectionMatrix = aMatrix;
		return *this;
	}

	camera& camera::set_perspective_projection(float aFov, float aAspect, float aNear, float aFar)
	{
		mProjectionType = projection_type::perspective;
		mFov = aFov;
		mAspect = aAspect;
		mNear = aNear;
		mFar = aFar;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_orthographic_projection(float aLeft, float aRight, float aBottom, float aTop, float aNear, float aFar)
	{
		mProjectionType = projection_type::orthographic;
		mLeft = aLeft;
		mRight = aRight;
		mBottom = aBottom;
		mTop = aTop;
		mNear = aNear;
		mFar = aFar;
		update_projection_matrix();
		return *this;
	}


	camera& camera::set_near_plane_distance(float aValue)
	{
		mNear = aValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_far_plane_distance(float aValue)
	{
		mFar = aValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_field_of_view(float aValue)
	{
		mFov = aValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_aspect_ratio(float aValue)
	{
		mAspect = aValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_left_border(float aValue)
	{
		mLeft = aValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_right_border(float aValue)
	{
		mRight = aValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_top_border(float aValue)
	{
		mTop = aValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_bottom_border(float aValue)
	{
		mBottom = aValue;
		update_projection_matrix();
		return *this;
	}


	camera& camera::copy_parameters_from(const camera& aOtherCamera)
	{
		mProjectionMatrix = aOtherCamera.mProjectionMatrix;
		mProjectionType = aOtherCamera.mProjectionType;
		mNear = aOtherCamera.mNear;
		mFar = aOtherCamera.mFar;
		mFov = aOtherCamera.mFov;
		mAspect = aOtherCamera.mAspect;
		mLeft = aOtherCamera.mLeft;
		mRight = aOtherCamera.mRight;
		mTop = aOtherCamera.mTop;
		mBottom = aOtherCamera.mBottom;
		update_projection_matrix();
		return *this;
	}


	float camera::get_z_buffer_depth(const glm::vec3& aWorldSpacePosition)
	{
		auto posSS = projection_and_view_matrix() * glm::vec4(aWorldSpacePosition, 1.0f);
		float depth = posSS.z / posSS.w;
		return depth;
	}

	float camera::get_z_buffer_depth(const transform& aTransform)
	{
		return get_z_buffer_depth(aTransform.translation());
	}


	glm::mat4 camera::view_matrix() const
	{
		// We are staying in a right-handed coordinate system throughout the entire pipeline.
		return inverse_local_transformation_matrix(); // TODO: Actually, we should use the inverse of the GLOBAL transformation matrix here, right?! => Investigate further and fix!
	}

	glm::mat4 camera::projection_and_view_matrix() const
	{
		return projection_matrix() * view_matrix();
	}

	void camera::update_projection_matrix()
	{
		static const glm::mat4 sInverseRotateAroundXFrom_RH_Yup_to_RH_Ydown = []() {
			
			// We assume all the spaces up to and including view space to feature a Y-axis pointing upwards.
			// Screen space in Vulkan, however, has the +Y-axis pointing downwards, +X to the left, and +Z into the screen.
			// We are staying in right-handed (RH) coordinate systems throughout ALL the spaces.
			// 
			// Therefore, we are representing the coordinates from here on -- actually exactly BETWEEN View Space and
			// Clip Space -- in a coordinate system which is rotated 180° around X, having Y point down, still RH.
			const glm::mat4 rotateAroundXFrom_RH_Yup_to_RH_Ydown = glm::mat4{
				 glm::vec4{ 1.f,  0.f,  0.f,  0.f},
				-glm::vec4{ 0.f,  1.f,  0.f,  0.f},
				-glm::vec4{ 0.f,  0.f,  1.f,  0.f},
				 glm::vec4{ 0.f,  0.f,  0.f,  1.f},
			};

			// ...in order to represent coordinates in that aforementioned space, we need the inverse, u know:
			return glm::inverse(rotateAroundXFrom_RH_Yup_to_RH_Ydown);
			
		}();
		
		switch (mProjectionType) {
			case projection_type::unknown:
				break;
			case projection_type::perspective:
			{
				// Scaling factor for the x and y coordinates which depends on the 
				// field of view (and the aspect ratio... see matrix construction)
				auto xyScale = 1.0f / glm::tan(mFov / 2.f);
				auto F_N = mFar - mNear;
				auto zScale = mFar / F_N;

				glm::mat4 m(0.0f);
				m[0][0] = xyScale / mAspect;
				m[1][1] = xyScale;
				m[2][2] = zScale;
				m[2][3] = 1.f; // Offset z...
				m[3][2] = -mNear * zScale; // ... by this amount

				mProjectionMatrix = m * sInverseRotateAroundXFrom_RH_Yup_to_RH_Ydown;

				break;
			}
			case projection_type::orthographic:
			{
				glm::mat4 m(1.0f);
				auto R_L = mRight - mLeft;
				auto T_B = mTop - mBottom;
				auto F_N = mFar - mNear;

				m[0][0] = 2.f / R_L;
				m[1][1] = 2.f / T_B;
				m[2][2] = 1.f / F_N;
				m[3][0] = -(mRight + mLeft) / R_L;
				m[3][1] = -(mTop + mBottom) / T_B;
				m[3][2] = -(mNear) / F_N;

				mProjectionMatrix = m * sInverseRotateAroundXFrom_RH_Yup_to_RH_Ydown;

				break;
			}
		}
	}
}

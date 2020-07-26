#include <gvk.hpp>

namespace gvk
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

	camera::~camera()
	{}


	camera& camera::set_projection_matrix(const glm::mat4& pMatrix)
	{
		mProjectionType = projection_type::unknown;
		mProjectionMatrix = pMatrix;
		return *this;
	}

	camera& camera::set_perspective_projection(float pFov, float pAspect, float pNear, float pFar)
	{
		mProjectionType = projection_type::perspective;
		mFov = pFov;
		mAspect = pAspect;
		mNear = pNear;
		mFar = pFar;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_orthographic_projection(float pLeft, float pRight, float pBottom, float pTop, float pNear, float pFar)
	{
		mProjectionType = projection_type::orthographic;
		mLeft = pLeft;
		mRight = pRight;
		mBottom = pBottom;
		mTop = pTop;
		mNear = pNear;
		mFar = pFar;
		update_projection_matrix();
		return *this;
	}


	camera& camera::set_near_plane_distance(float pValue)
	{
		mNear = pValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_far_plane_distance(float pValue)
	{
		mFar = pValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_field_of_view(float pValue)
	{
		mFov = pValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_aspect_ratio(float pValue)
	{
		mAspect = pValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_left_border(float pValue)
	{
		mLeft = pValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_right_border(float pValue)
	{
		mRight = pValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_top_border(float pValue)
	{
		mTop = pValue;
		update_projection_matrix();
		return *this;
	}

	camera& camera::set_bottom_border(float pValue)
	{
		mBottom = pValue;
		update_projection_matrix();
		return *this;
	}


	camera& camera::copy_parameters_from(const camera& pOtherCamera)
	{
		mProjectionMatrix = pOtherCamera.mProjectionMatrix;
		mProjectionType = pOtherCamera.mProjectionType;
		mNear = pOtherCamera.mNear;
		mFar = pOtherCamera.mFar;
		mFov = pOtherCamera.mFov;
		mAspect = pOtherCamera.mAspect;
		mLeft = pOtherCamera.mLeft;
		mRight = pOtherCamera.mRight;
		mTop = pOtherCamera.mTop;
		mBottom = pOtherCamera.mBottom;
		update_projection_matrix();
		return *this;
	}


	float camera::get_z_buffer_depth(const glm::vec3& pWorldSpacePosition)
	{
		auto posSS = projection_and_view_matrix() * glm::vec4(pWorldSpacePosition, 1.0f);
		float depth = posSS.z / posSS.w;
		// TODO: For OpenGL, this has to be transformed into a different range most likely
		return depth;
	}

	float camera::get_z_buffer_depth(transform& transform)
	{
		// TODO: pass transform's world space position:
		return get_z_buffer_depth(glm::vec3{ 0.f, 0.f, 0.f });
	}


	glm::mat4 camera::view_matrix() const
	{
		// TODO: For OpenGL, this will have to be a different matrix, namely one that transform into a left-handed coordinate system
		glm::mat4 vM = glm::mat4(
			 mMatrix[0],
			-mMatrix[1],
			-mMatrix[2],
			 mMatrix[3]
		);
		return glm::inverse(vM);
	}

	glm::mat4 camera::projection_and_view_matrix() const
	{
		return projection_matrix() * view_matrix();
	}

	void camera::update_projection_matrix()
	{
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

				mProjectionMatrix = m;

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

				mProjectionMatrix = m;

				break;
			}
		}
	}
}

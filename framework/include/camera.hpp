#pragma once

namespace cgb
{
	enum struct projection_type
	{
		unknown,
		perspective,
		orthographic,
	};

	class camera : public transform
	{
	public:
		camera();
		camera(camera&&) noexcept = default;
		camera(const camera&) noexcept = default;
		camera& operator=(camera&&) noexcept = default;
		camera& operator=(const camera&) noexcept = default;
		virtual ~camera();

		// Returns the type of projection matrix used
		projection_type projection_type() const { return mProjectionType; }

		// returns the projection matrix
		const glm::mat4& projection_matrix() const { return mProjectionMatrix; }
		// returns the distance of the near plane (ortho and perspective)
		float near_plane_distance() const { return mNear; }
		// returns the distance of the far plane (ortho and perspective)
		float far_plane_distance() const { return mFar; }
		// returns the field of view angle in radians (perspective only)
		float field_of_view() const { return mFov; }
		// returns the aspect ratio (width/height)
		float aspect_ratio() const { 
			return projection_type::orthographic == mProjectionType 
					? (right_border() - left_border()) / (top_border() - bottom_border()) 
					: mAspect; 
		}
		// returns the distance of the view cube's left border (ortho only)
		float left_border() const { return mLeft; }
		// returns the distance of the view cube's right border (ortho only)
		float right_border() const { return mRight; }
		// returns the distance of the view cube's top border (ortho only)
		float top_border() const { return mTop; }
		// returns the distance of the view cube's bottom border (ortho only)
		float bottom_border() const { return mBottom; }

		// sets the projection matrix
		camera& set_projection_matrix(const glm::mat4& pMatrix);
		// Constructs and sets a perspective projection matrix with the given parameters
		camera& set_perspective_projection(float pFov, float pAspect, float pNear, float pFar);
		// Constructs and sets an orthographic projection matrix with the given parameters
		camera& set_orthographic_projection(float pLeft, float pRight, float pBottom, float pTop, float pNear, float pFar);

		// sets the distance of the near plane (ortho and perspective) and updates the matrix
		camera& set_near_plane_distance(float pValue);
		// sets the distance of the far plane (ortho and perspective) and updates the matrix
		camera& set_far_plane_distance(float pValue);
		// sets the field of view angle in radians (perspective only) and updates the matrix
		camera& set_field_of_view(float pValue);
		// sets the aspect ratio (width/height) and updates the matrix
		camera& set_aspect_ratio(float pValue);
		// sets the distance of the view cube's left border (ortho only) and updates the matrix
		camera& set_left_border(float pValue);
		// sets the distance of the view cube's right border (ortho only) and updates the matrix
		camera& set_right_border(float pValue);
		// sets the distance of the view cube's top border (ortho only) and updates the matrix
		camera& set_top_border(float pValue);
		// sets the distance of the view cube's bottom border (ortho only) and updates the matrix
		camera& set_bottom_border(float pValue);

		// Copies all the camera parameters from the other camera, but leaves the transform unchanged
		// The parameters copied are all the direct members of this class.
		camera& copy_parameters_from(const camera& pOtherCamera);

		// calculates the z-buffer depth of the specified position in world space
		float get_z_buffer_depth(const glm::vec3& pWorldSpacePosition);
		// Calculates the z-buffer depth of a given transform's position
		float get_z_buffer_depth(transform& transform);

		// Calculates and returns the view matrix of this camera.
		// The view matrix is calculated based on the following assumptions:
		//  - forward a.k.a. the camera's look direction is pointing in -z direction
		//  - TBD: differences OpenGL vs Vulkan
		glm::mat4 view_matrix() const;

		// This is a shortcut for projection_matrix() * view_matrix(), i.e. it returns 
		// both matrices combined together.
		glm::mat4 projection_and_view_matrix() const;

	protected:
		void update_projection_matrix();

		glm::mat4 mProjectionMatrix;

		cgb::projection_type mProjectionType;
		float mNear;
		float mFar;
		float mFov;		// perspective only
		float mAspect;	// perspective only
		float mLeft;	// ortho only
		float mRight;	// ortho only
		float mTop;		// ortho only
		float mBottom;	// ortho only

	};
}

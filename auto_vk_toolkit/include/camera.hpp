#pragma once

#include "transform.hpp"

namespace avk
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
		virtual ~camera() = default;

		// Returns the type of projection matrix used
		enum projection_type projection_type() const { return mProjectionType; }

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
		camera& set_projection_matrix(const glm::mat4& aMatrix, avk::projection_type aProjectionType = avk::projection_type::unknown);
		// Constructs and sets a perspective projection matrix with the given parameters
		camera& set_perspective_projection(float aFov, float aAspect, float aNear, float aFar);
		// Constructs and sets an orthographic projection matrix with the given parameters
		camera& set_orthographic_projection(float aLeft, float aRight, float aBottom, float aTop, float aNear, float aFar);

		// sets the distance of the near plane (ortho and perspective) and updates the matrix
		camera& set_near_plane_distance(float aValue);
		// sets the distance of the far plane (ortho and perspective) and updates the matrix
		camera& set_far_plane_distance(float aValue);
		// sets the field of view angle in radians (perspective only) and updates the matrix
		camera& set_field_of_view(float aValue);
		// sets the aspect ratio (width/height) and updates the matrix
		camera& set_aspect_ratio(float aValue);
		// sets the distance of the view cube's left border (ortho only) and updates the matrix
		camera& set_left_border(float aValue);
		// sets the distance of the view cube's right border (ortho only) and updates the matrix
		camera& set_right_border(float aValue);
		// sets the distance of the view cube's top border (ortho only) and updates the matrix
		camera& set_top_border(float aValue);
		// sets the distance of the view cube's bottom border (ortho only) and updates the matrix
		camera& set_bottom_border(float aValue);

		// Copies all the camera parameters from the other camera, but leaves the transform unchanged
		// The parameters copied are all the direct members of this class.
		camera& copy_parameters_from(const camera& aOtherCamera);

		// calculates the z-buffer depth of the specified position in world space
		float get_z_buffer_depth(const glm::vec3& aWorldSpacePosition);
		// Calculates the z-buffer depth of a given transform's position
		float get_z_buffer_depth(const transform& aTransform);

		// Calculates and returns the view matrix of this camera.
		glm::mat4 view_matrix() const;

		// This is a shortcut for projection_matrix() * view_matrix(), i.e. it returns 
		// both matrices combined together.
		glm::mat4 projection_and_view_matrix() const;

	protected:
		void update_projection_matrix();

		glm::mat4 mProjectionMatrix;

		avk::projection_type mProjectionType;
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

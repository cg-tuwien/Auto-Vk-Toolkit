module;
#include <auto_vk_toolkit.hpp>

export module transform;

export
namespace avk
{
	class transform : std::enable_shared_from_this<transform>
	{
	public:
		using ptr = std::shared_ptr<transform>;

		friend void attach_transform(transform::ptr pParent, transform::ptr pChild);
		friend void detach_transform(transform::ptr pParent, transform::ptr pChild);

		/** Constructs a transform with separate values for translation, rotation, and scale */
		transform(glm::vec3 pTranslation = { 0.f, 0.f, 0.f }, glm::quat pRotation = { 1.f, 0.f, 0.f, 0.f }, glm::vec3 pScale = { 1.f, 1.f, 1.f }) noexcept;
		/** Constructs a transform with coordinate transform basis vectors, and a translation */
		transform(glm::vec3 pBasisX, glm::vec3 pBasisY, glm::vec3 pBasisZ, glm::vec3 pTranslation = { 0.f, 0.f, 0.f }) noexcept;
		/** Steal the guts of another transform */
		transform(transform&& other) noexcept;
		/** Copy of another transform */
		transform(const transform& other) noexcept;
		/** Steal the guts of another transform */
		transform& operator=(transform&& other) noexcept;
		/** Copy of another transform */
		transform& operator=(const transform& other) noexcept;
		virtual ~transform();

		/** sets a new position, current position is overwritten */
		void set_translation(const glm::vec3& aValue);
		/** sets a new rotation, current rotation is overwritten */
		void set_rotation(const glm::quat& aValue);
		/** sets a new scale, current scale is overwritten */
		void set_scale(const glm::vec3& aValue);

		/** sets an entirely new matrix */
		void set_matrix(const glm::mat4& aValue);

		/** returns the local transformation matrix, disregarding parent transforms */
		glm::mat4 local_transformation_matrix() const;
		/** Returns the inverse of the local transformation matrix. This can be used to change basis into this transform's coordinate system. */
		glm::mat4 inverse_local_transformation_matrix() const;
		/** returns the global transformation matrix, taking parent transforms into account */
		glm::mat4 global_transformation_matrix() const;
		/** returns the inverse of the global transformation matrix */
		glm::mat4 inverse_global_transformation_matrix() const;

		/** Gets the matrix' x-axis, which can be seen as the local right vector */
		glm::vec3 x_axis() const { return mMatrix[0]; }
		/** Gets the matrix' y-axis, which can be seen as the local up vector */
		glm::vec3 y_axis() const { return mMatrix[1]; }
		/** Gets the matrix' z-axis, which can be seen as the local back vector */
		glm::vec3 z_axis() const { return mMatrix[2]; }

		/** Gets the local transformation matrix */
		glm::mat4 matrix() const { return mMatrix; }

		/** Gets the local translation */
		glm::vec3 translation() const { return mTranslation; }

		/** Gets the local rotation */
		glm::quat rotation() const { return mRotation; }

		/** Local scale vector */
		glm::vec3 scale() const { return mScale; }

		/* Make the front direction (a.k.a. -z) be oriented towards the given position. */
		void look_at(const glm::vec3& aPosition);

		/* Look along the given direction, or put differently: orient -z towards aDirection. */
		void look_along(const glm::vec3& aDirection);

		/** Returns true if this transform is a child has a parent transform. */
		bool has_parent();
		/** Returns true if this transform has child transforms. */
		bool has_childs();
		/** Returns the parent of this transform or nullptr */
		transform::ptr parent();

	private:
		/** Updates the internal matrix based on translation, rotation scale */
		void update_matrix_from_transforms();
		/** Extracts translation, rotation and scale from the matrix and sets the internal fields' values */
		void update_transforms_from_matrix();

	protected:
		/** Orthogonal basis + translation in a 4x4 matrix */
		glm::mat4 mMatrix;
		/** The inverse of mMatrix */
		glm::mat4 mInverseMatrix;
		/** Offset from the coordinate origin */
		glm::vec3 mTranslation;
		/** Rotation quaternion */
		glm::quat mRotation;
		/** Local scale vector */
		glm::vec3 mScale;

		/** Parent transform or nullptr */
		transform::ptr mParent;
		/** List of child transforms */
		std::vector<transform::ptr> mChilds;
	};

	glm::mat4 matrix_from_transforms(glm::vec3 aTranslation, glm::quat aRotation, glm::vec3 aScale)
	{
		auto x = aRotation * glm::vec3{ 1.0f, 0.0f, 0.0f };
		auto y = aRotation * glm::vec3{ 0.0f, 1.0f, 0.0f };
		auto z = glm::cross(x, y);
		y = glm::cross(z, x);
		return glm::mat4{
			glm::vec4{x, 0.0f} *aScale.x,
			glm::vec4{y, 0.0f} *aScale.y,
			glm::vec4{z, 0.0f} *aScale.z,
			glm::vec4{aTranslation, 1.0f}
		};
	}

	std::tuple<glm::vec3, glm::quat, glm::vec3> transforms_from_matrix(glm::mat4 aMatrix)
	{
		auto translation = glm::vec3{ aMatrix[3] };
		auto scale = glm::vec3{
			glm::length(glm::vec3{aMatrix[0]}),
			glm::length(glm::vec3{aMatrix[1]}),
			glm::length(glm::vec3{aMatrix[2]})
		};
		auto rotation = glm::quat_cast(glm::mat3{
			aMatrix[0] / scale.x,
			aMatrix[1] / scale.y,
			aMatrix[2] / scale.z
			});
		return std::make_tuple(translation, rotation, scale);
	}

	void attach_transform(transform::ptr pParent, transform::ptr pChild);
	void detach_transform(transform::ptr pParent, transform::ptr pChild);

	glm::vec3 back() { return glm::vec3{ 0.f,  0.f,  1.f }; } // "back" together with "right" and "up" forms a right-handed coordinate system.
	glm::vec3 front() { return glm::vec3{ 0.f,  0.f, -1.f }; } // Auto-Vk-Toolkit convention is: the negative z-direction means "front" 
	glm::vec3 right() { return glm::vec3{ 1.f,  0.f,  0.f }; }
	glm::vec3 left() { return glm::vec3{ -1.f,  0.f,  0.f }; }
	glm::vec3 up() { return glm::vec3{ 0.f,  1.f,  0.f }; }
	glm::vec3 down() { return glm::vec3{ 0.f, -1.f,  0.f }; }
	glm::vec3 back(const transform& pTransform) { return  pTransform.z_axis(); }
	glm::vec3 front(const transform& pTransform) { return -pTransform.z_axis(); }
	glm::vec3 right(const transform& pTransform) { return  pTransform.x_axis(); }
	glm::vec3 left(const transform& pTransform) { return -pTransform.x_axis(); }
	glm::vec3 up(const transform& pTransform) { return  pTransform.y_axis(); }
	glm::vec3 down(const transform& pTransform) { return -pTransform.y_axis(); }
	glm::vec3 front_wrt(const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 back_wrt(const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 right_wrt(const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 left_wrt(const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 up_wrt(const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 down_wrt(const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	void translate(transform& pTransform, const glm::vec3& pTranslation);
	void rotate(transform& pTransform, const glm::quat& pRotation);
	void scale(transform& pTransform, const glm::vec3& pScale);
	void translate_wrt(transform& pTransform, const glm::vec3& pTranslation, glm::mat4 pReference = glm::mat4(1.0f));
	void rotate_wrt(transform& pTransform, const glm::quat& pRotation, glm::mat4 pReference = glm::mat4(1.0f));
	void scale_wrt(transform& pTransform, const glm::vec3& pScale, glm::mat4 pReference = glm::mat4(1.0f));

}
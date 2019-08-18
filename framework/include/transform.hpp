#pragma once

namespace cgb
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
		void set_translation(const glm::vec3& pValue);
		/** sets a new rotation, current rotation is overwritten */
		void set_rotation(const glm::quat& pValue);
		/** sets a new scale, current scale is overwritten */
		void set_scale(const glm::vec3& pValue);

		/** returns the local transformation matrix, disregarding parent transforms */
		const glm::mat4& local_transformation_matrix() const;
		/** returns the global transformation matrix, taking parent transforms into account */
		glm::mat4 global_transformation_matrix() const;

		/** Gets the matrix' x-axis, which can be seen as the local right vector */
		glm::vec3 x_axis() const { return mMatrix[0]; }
		/** Gets the matrix' y-axis, which can be seen as the local up vector */
		glm::vec3 y_axis() const { return mMatrix[1]; }
		/** Gets the matrix' z-axis, which can be seen as the local back vector */
		glm::vec3 z_axis() const { return mMatrix[2]; }

		/** Gets the local transformation matrix */
		const glm::mat4& matrix() const { return mMatrix; }

		/** Gets the local translation */
		const glm::vec3& translation() const { return mTranslation; }
		
		/** Gets the local rotation */
		const glm::quat& rotation() const { return mRotation; }

		/** Local scale vector */
		const glm::vec3& scale() const { return mScale; }

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

	void attach_transform(transform::ptr pParent, transform::ptr pChild);
	void detach_transform(transform::ptr pParent, transform::ptr pChild);

	static glm::vec3 back (const transform& pTransform) { return  pTransform.z_axis(); }
	static glm::vec3 front(const transform& pTransform) { return -pTransform.z_axis(); }
	static glm::vec3 right(const transform& pTransform) { return  pTransform.x_axis(); }
	static glm::vec3 left (const transform& pTransform) { return -pTransform.x_axis(); }
	static glm::vec3 up   (const transform& pTransform) { return  pTransform.y_axis(); }
	static glm::vec3 down (const transform& pTransform) { return -pTransform.y_axis(); }
	glm::vec3 front_wrt(const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 back_wrt (const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 right_wrt(const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 left_wrt (const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 up_wrt   (const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	glm::vec3 down_wrt (const transform& pTransform, glm::mat4 pReference = glm::mat4(1.0f));
	void translate(transform& pTransform, const glm::vec3& pTranslation);
	void rotate(transform& pTransform, const glm::quat& pRotation);
	void scale(transform& pTransform, const glm::vec3& pScale);
	void translate_wrt(transform& pTransform, const glm::vec3& pTranslation, glm::mat4 pReference = glm::mat4(1.0f));
	void rotate_wrt(transform& pTransform, const glm::quat& pRotation, glm::mat4 pReference = glm::mat4(1.0f));
	void scale_wrt(transform& pTransform, const glm::vec3& pScale, glm::mat4 pReference = glm::mat4(1.0f));

	///**  */
	//void LookAt(transform* target);
	///**  */
	//void LookAt(const glm::vec3& target);
	///**  */
	//void LookAlong(const glm::vec3& direction);

	///**  */
	//void AlignUpVectorTowards(transform* target);
	///**  */
	//void AlignUpVectorTowards(const glm::vec3& target);
	///**  */
	//void AlignUpVectorAlong(const glm::vec3& direction);
}

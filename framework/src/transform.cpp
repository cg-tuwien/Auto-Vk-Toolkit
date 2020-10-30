#include <gvk.hpp>

namespace gvk
{
	using namespace glm;

	void transform::update_matrix_from_transforms()
	{
		mMatrix = matrix_from_transforms(mTranslation, mRotation, mScale);
	}
	
	void transform::update_transforms_from_matrix()
	{
		auto [translation, rotation, scale] = transforms_from_matrix(mMatrix);
		mTranslation = translation;
		mRotation = rotation;
		mScale = scale;
	}

	transform::transform(vec3 pTranslation, quat pRotation, vec3 pScale) noexcept
		: mTranslation(pTranslation)
		, mRotation(pRotation)
		, mScale(pScale)
		//, mParent(nullptr)
	{ 
		update_matrix_from_transforms();
	}
	
	transform::transform(glm::vec3 pBasisX, vec3 pBasisY, vec3 pBasisZ, vec3 pTranslation) noexcept
		: mMatrix(mat4(
			vec4(pBasisX, 0.0f)* mScale.x,
			vec4(pBasisY, 0.0f)* mScale.y,
			vec4(pBasisZ, 0.0f)* mScale.z,
			vec4(pTranslation, 1.0f)
		))
		//, mParent(nullptr)
	{
		update_transforms_from_matrix();
	}

	transform::transform(transform&& other) noexcept
		: mMatrix{ std::move(other.mMatrix) }
		, mTranslation{ std::move(other.mTranslation) }
		, mRotation{ std::move(other.mRotation) }
		, mScale{ std::move(other.mScale) }
		, mParent{ std::move(other.mParent) }
		, mChilds{ std::move(other.mChilds) }
	{
		for (auto& child : mChilds) {
			// This will overwrite their previous parent-pointer:
			attach_transform(shared_from_this(), child);
		}
		other.mParent = nullptr;
		other.mChilds.clear();
	}
	
	transform::transform(const transform& other) noexcept
		: mMatrix{ other.mMatrix }
		, mTranslation{ other.mTranslation }
		, mRotation{ other.mRotation }
		, mScale{ other.mScale }
		, mParent{ other.mParent }
	{
		for (auto& child : other.mChilds) {
			// Copy the childs. This can have undesired side effects, actually (e.g. if 
			// childs' classes are derived from transform, what happens then? Don't know.)
			auto clonedChild = std::make_shared<transform>(*child);
			attach_transform(shared_from_this(), child);
		}
	}
	
	transform& transform::operator=(transform&& other) noexcept
	{
		mMatrix = std::move(other.mMatrix);
		mTranslation = std::move(other.mTranslation);
		mRotation = std::move(other.mRotation);
		mScale = std::move(other.mScale);
		mParent = std::move(other.mParent);
		mChilds = std::move(other.mChilds);
		for (auto& child : mChilds) {
			// This will overwrite their previous parent-pointer:
			attach_transform(shared_from_this(), child);
		}
		other.mParent = nullptr;
		other.mChilds.clear();
		return *this;
	}
	
	transform& transform::operator=(const transform& other) noexcept
	{
		mMatrix = other.mMatrix;
		mTranslation = other.mTranslation;
		mRotation = other.mRotation;
		mScale = other.mScale;
		mParent = other.mParent;
		for (auto& child : other.mChilds) {
			// Copy the childs. This can have undesired side effects, actually (e.g. if 
			// childs' classes are derived from transform, what happens then? Don't know.)
			auto clonedChild = std::make_shared<transform>(*child);
			attach_transform(shared_from_this(), child);
		}
		return *this;
	}

	transform::~transform()
	{
		mParent = nullptr;
		mChilds.clear();
	}

	void transform::set_translation(const vec3& pValue)
	{
		mTranslation = pValue;
		update_matrix_from_transforms();
	}

	void transform::set_rotation(const quat& pValue)
	{
		mRotation = pValue;
		update_matrix_from_transforms();
	}

	void transform::set_scale(const vec3& pValue)
	{
		mScale = pValue;
		update_matrix_from_transforms();
	}

	glm::mat4 transform::local_transformation_matrix() const
	{
		return mMatrix;
	}

	glm::mat4 transform::global_transformation_matrix() const
	{
		if (mParent) {
			return mParent->global_transformation_matrix() * mMatrix;
		}
		else {
			return mMatrix;
		}
	}

	bool transform::has_parent()
	{
		return static_cast<bool>(mParent);
	}

	bool transform::has_childs()
	{
		return mChilds.size() > 0;
	}

	transform::ptr transform::parent()
	{
		return mParent;
	}

	void attach_transform(transform::ptr pParent, transform::ptr pChild)
	{
		assert(pParent);
		assert(pChild);
		if (pChild->has_parent()) {
			detach_transform(pChild->parent(), pChild);
		}
		pChild->mParent = pParent;
		pParent->mChilds.push_back(pChild);
	}

	void detach_transform(transform::ptr pParent, transform::ptr pChild)
	{
		assert(pParent);
		assert(pChild);
		if (!pChild->has_parent() || pChild->parent() != pParent) {
			LOG_WARNING(fmt::format("Can not detach child[{}] from parent[{}]", fmt::ptr(pChild.get()), fmt::ptr(pParent.get())));
			return;
		}
		pChild->mParent = nullptr;
		pParent->mChilds.erase(std::remove(
				std::begin(pParent->mChilds), std::end(pParent->mChilds),
				pChild
			), 
			std::end(pParent->mChilds));
	}

	glm::vec3 front_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(gvk::front(pTransform), 1.0f);
	}

	glm::vec3 back_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(gvk::back(pTransform), 1.0f);
	}

	glm::vec3 right_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(gvk::right(pTransform), 1.0f);
	}

	glm::vec3 left_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(gvk::left(pTransform), 1.0f);
	}

	glm::vec3 up_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(gvk::up(pTransform), 1.0f);
	}

	glm::vec3 down_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(gvk::down(pTransform), 1.0f);
	}

	void translate(transform& pTransform, const glm::vec3& pTranslation)
	{
		pTransform.set_translation(pTransform.translation() + pTranslation);
	}

	void rotate(transform& pTransform, const glm::quat& pRotation)
	{
		pTransform.set_rotation(pRotation * pTransform.rotation());
	}

	void scale(transform& pTransform, const glm::vec3& pScale)
	{
		pTransform.set_scale(pTransform.scale() * pScale);
	}

	void translate_wrt(transform& pTransform, const glm::vec3& pTranslation, glm::mat4 pReference)
	{
		// TODO: How to?
	}

	void rotate_wrt(transform& pTransform, const glm::quat& pRotation, glm::mat4 pReference)
	{
		// TODO: How to?
	}

	void scale_wrt(transform& pTransform, const glm::vec3& pScale, glm::mat4 pReference)
	{
		// TODO: How to?
	}

}

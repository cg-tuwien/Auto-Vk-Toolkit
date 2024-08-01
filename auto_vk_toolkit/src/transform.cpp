#include "transform.hpp"

namespace avk
{
	using namespace glm;

	void transform::update_matrix_from_transforms()
	{
		mMatrix = matrix_from_transforms(mTranslation, mRotation, mScale);
		mInverseMatrix = glm::inverse(mMatrix);
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
		, mInverseMatrix{ std::move(other.mInverseMatrix) }
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
		, mInverseMatrix{ other.mInverseMatrix }
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
		mInverseMatrix = std::move(other.mInverseMatrix);
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
		mInverseMatrix = other.mInverseMatrix;
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

	void transform::set_translation(const vec3& aValue)
	{
		mTranslation = aValue;
		update_matrix_from_transforms();
	}

	void transform::set_rotation(const quat& aValue)
	{
		mRotation = aValue;
		update_matrix_from_transforms();
	}

	void transform::set_scale(const vec3& aValue)
	{
		mScale = aValue;
		update_matrix_from_transforms();
	}

	void transform::set_matrix(const glm::mat4& aValue)
	{
		mMatrix = aValue;
		mInverseMatrix = glm::inverse(mMatrix);
		update_transforms_from_matrix();
	}

	glm::mat4 transform::local_transformation_matrix() const
	{
		return mMatrix;
	}

	glm::mat4 transform::inverse_local_transformation_matrix() const
	{
		return mInverseMatrix;
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

	glm::mat4 transform::inverse_global_transformation_matrix() const
	{
		return glm::inverse(global_transformation_matrix());
	}

	void transform::look_at(const glm::vec3& aPosition)
	{
		auto dir = aPosition - translation();
		auto len = dot(dir, dir);
		if (len <= 1.2e-7 /* ~machine epsilon */) {
			return;
		}
		look_along(dir); // will be normalized within look_along
	}

	void transform::look_along(const glm::vec3& aDirection)
	{
		if (glm::dot(aDirection, aDirection) < 1.2e-7 /* ~machine epsilon */) {
			LOG_DEBUG("Direction vector passed to transform::look_along has (almost) zero length.");
			return;
		}
		
		// Note: I think, this only works "by accident" so to say, because GLM assumes that it should create a view matrix with quatLookAt.
		//       Actually, I do not want GLM to create a view matrix here, but only to orient in a specific way.
		//       The direction is being negated within quatLookAt, which is probably the reason, why we don't have to negate it here.
		//       The thing is, the front-vector is pointing into -z direction by our conventions. But we don't have to negate the
		//       incoming aDirection to get the -z direction pointing into aDirection. That's why I think, quatLookAt already creates
		//       the... hmm... inverse? Not sure about that, though. I mean, it's not a matrix, but a quaternion. My reasoning is probably correct. (Don't know for sure.)
		set_rotation(glm::normalize(glm::quatLookAt(glm::normalize(aDirection), glm::vec3{0.f, 1.f, 0.f})));
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
			LOG_WARNING(std::format("Can not detach child[{}] from parent[{}]", reinterpret_cast<intptr_t>(pChild.get()), reinterpret_cast<intptr_t>(pParent.get())));
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
		return invTrSpace * glm::vec4(avk::front(pTransform), 1.0f);
	}

	glm::vec3 back_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(avk::back(pTransform), 1.0f);
	}

	glm::vec3 right_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(avk::right(pTransform), 1.0f);
	}

	glm::vec3 left_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(avk::left(pTransform), 1.0f);
	}

	glm::vec3 up_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(avk::up(pTransform), 1.0f);
	}

	glm::vec3 down_wrt(const transform& pTransform, glm::mat4 pReference)
	{
		glm::mat4 trSpace = pReference * pTransform.global_transformation_matrix();
		glm::mat4 invTrSpace = glm::inverse(trSpace);
		return invTrSpace * glm::vec4(avk::down(pTransform), 1.0f);
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

#include <gvk.hpp>
#include <glm/gtx/quaternion.hpp>

namespace gvk
{
	void animation::animate(const animation_clip_data& aClip, double aTime, void(*aBoneMatrixCalc)(glm::mat4*, const glm::mat4&, const glm::mat4&, const glm::mat4&))
	{
		if (aClip.mTicksPerSecond == 0.0) {
			throw gvk::runtime_error("mTicksPerSecond may not be 0.0 => set a different value!");
		}
		if (aClip.mAnimationIndex != mAnimationIndex) {
			throw gvk::runtime_error("The animation index of the passed animation_clip_data is not the same that was used to create this animation.");
		}

		double timeInTicks = aTime * aClip.mTicksPerSecond;

		for (auto& anode : mAnimationData) {
			// First, calculate the local transform
			glm::mat4 localTransform{1.0f};

			// The localTransform can only be different than the identity if there are animation keys.
			if (anode.mPositionKeys.size() + anode.mRotationKeys.size() + anode.mScalingKeys.size() > 0) {
				// Translation/position:
				auto [tpos1, tpos2] = find_positions_in_keys(anode.mPositionKeys, timeInTicks);
				auto tf = get_interpolation_factor(anode.mPositionKeys[tpos1], anode.mPositionKeys[tpos2], timeInTicks);
				auto translation = glm::lerp(anode.mPositionKeys[tpos1].mValue, anode.mPositionKeys[tpos2].mValue, tf);

				// Rotation:
				size_t rpos1 = tpos1, rpos2 = tpos2;
				if (!anode.mSameRotationAndPositionKeyTimes) {
					std::tie(rpos1, rpos2) = find_positions_in_keys(anode.mRotationKeys, timeInTicks);
				}
				auto rf = get_interpolation_factor(anode.mRotationKeys[rpos1], anode.mRotationKeys[rpos2], timeInTicks);
				auto rotation = glm::slerp(anode.mRotationKeys[rpos1].mValue, anode.mRotationKeys[rpos2].mValue, rf);	// use slerp, not lerp or mix (those lead to jerks)
				rotation = glm::normalize(rotation); // normalize the resulting quaternion, just to be on the safe side

				// Scaling:
				size_t spos1 = tpos1, spos2 = tpos2;
				if (!anode.mSameScalingAndPositionKeyTimes) {
					std::tie(spos1, spos2) = find_positions_in_keys(anode.mScalingKeys, timeInTicks);
				}
				auto sf = get_interpolation_factor(anode.mScalingKeys[spos1], anode.mScalingKeys[spos2], timeInTicks);
				auto scaling = glm::lerp(anode.mScalingKeys[spos1].mValue, anode.mScalingKeys[spos2].mValue, sf);

				localTransform = matrix_from_transforms(translation, rotation, scaling);
			}

			// Calculate the node's global transform, using its local transform and the transforms of its parents:
			if (anode.mAnimatedParentIndex.has_value()) {
				anode.mTransform = mAnimationData[anode.mAnimatedParentIndex.value()].mTransform * anode.mParentTransform * localTransform;
			}
			else {
				anode.mTransform = anode.mParentTransform * localTransform;
			}

			// Calculate the final bone matrices for this node, for each mesh that is affected; and write out the matrix into the target storage:
			const auto n = anode.mBoneMeshTargets.size();
			for (size_t i = 0; i < n; ++i) {
				// Construction of the bone matrix for this node:
				//  1. Bring vertex into bone space
				//  2. Apply transformaton in bone space
				//  3. Convert transformed vertex back to mesh space
				// The callback function will store into target:
				aBoneMatrixCalc(anode.mBoneMeshTargets[i].mBoneMatrixTarget, anode.mBoneMeshTargets[i].mInverseMeshRootMatrix, anode.mTransform, anode.mBoneMeshTargets[i].mInverseBindPoseMatrix);
			}
		}
	}

	void animation::animate(const animation_clip_data& aClip, double aTime)
	{
		animate(aClip, aTime, [](glm::mat4* targetStorage, const glm::mat4& inverseMeshRootMatrix, const glm::mat4& nodeTransformMatrix, const glm::mat4& inverseBindPoseMatrix){
			*targetStorage = inverseMeshRootMatrix * nodeTransformMatrix * inverseBindPoseMatrix;
		});
	}

	void animation::animate(const animation_clip_data& aClip, double aTime, bone_matrices_space aTargetSpace)
	{
		switch (aTargetSpace) {
		case bone_matrices_space::mesh_space:
			animate(aClip, aTime, [](glm::mat4* targetStorage, const glm::mat4& inverseMeshRootMatrix, const glm::mat4& nodeTransformMatrix, const glm::mat4& inverseBindPoseMatrix){
				*targetStorage = inverseMeshRootMatrix * nodeTransformMatrix * inverseBindPoseMatrix;
			});
			break;
		case bone_matrices_space::object_space:
			animate(aClip, aTime, [](glm::mat4* targetStorage, const glm::mat4& inverseMeshRootMatrix, const glm::mat4& nodeTransformMatrix, const glm::mat4& inverseBindPoseMatrix){
				// The nodeTransformMatrix yields the result in OBJECT SPACE again
				*targetStorage = nodeTransformMatrix * inverseBindPoseMatrix;
			});
			break;
		case bone_matrices_space::bone_space:
			animate(aClip, aTime, [](glm::mat4* targetStorage, const glm::mat4& inverseMeshRootMatrix, const glm::mat4& nodeTransformMatrix, const glm::mat4& inverseBindPoseMatrix){
				// The nodeTransformMatrix yields the result in OBJECT SPACE => transform again by inverseBindPoseMatrix to get it in BONE SPACE
				*targetStorage = inverseBindPoseMatrix * nodeTransformMatrix * inverseBindPoseMatrix;
			});
			break;
		default:
			throw gvk::runtime_error("Unhandled target space value.");
		}
	}
}

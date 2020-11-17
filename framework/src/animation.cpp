#include <gvk.hpp>
#include <glm/gtx/quaternion.hpp>

namespace gvk
{
	void animation::animate(const animation_clip_data& aClip, double aTime)
	{
		return animate(aClip, aTime, bone_matrices_space::mesh_space);
	}

	void animation::animate(const animation_clip_data& aClip, double aTime, bone_matrices_space aTargetSpace)
	{
		switch (aTargetSpace) {
		case bone_matrices_space::mesh_space:
			animate(aClip, aTime, [](const animated_node& anode, size_t i){
				*anode.mBoneMeshTargets[i].mBoneMatrixTarget = anode.mBoneMeshTargets[i].mInverseMeshRootMatrix * anode.mTransform * anode.mBoneMeshTargets[i].mInverseBindPoseMatrix;
			});
			break;
		case bone_matrices_space::object_space:
			animate(aClip, aTime, [](const animated_node& anode, size_t i){
				// The nodeTransformMatrix yields the result in OBJECT SPACE again
				*anode.mBoneMeshTargets[i].mBoneMatrixTarget = anode.mTransform * anode.mBoneMeshTargets[i].mInverseBindPoseMatrix;
			});
			break;
		case bone_matrices_space::bone_space:
			animate(aClip, aTime, [](const animated_node& anode, size_t i){
				// The nodeTransformMatrix yields the result in OBJECT SPACE => transform again by inverseBindPoseMatrix to get it in BONE SPACE
				*anode.mBoneMeshTargets[i].mBoneMatrixTarget = anode.mBoneMeshTargets[i].mInverseBindPoseMatrix * anode.mTransform * anode.mBoneMeshTargets[i].mInverseBindPoseMatrix;
			});
			break;
		default:
			throw gvk::runtime_error("Unhandled target space value.");
		}
	}

	std::vector<double> animation::animation_key_times_within_clip(const animation_clip_data& aClip)
	{
		std::set<double> mUniqueKeys;
		for (auto& anode : mAnimationData) {
			for (const auto& pk : anode.mPositionKeys) {
				if (pk.mTime >= aClip.mStartTicks && pk.mTime <= aClip.mEndTicks) {
					mUniqueKeys.insert(pk.mTime);
				}
			}
			for (const auto& rk : anode.mRotationKeys) {
				if (rk.mTime >= aClip.mStartTicks && rk.mTime <= aClip.mEndTicks) {
					mUniqueKeys.insert(rk.mTime);
				}
			}
			for (const auto& sk : anode.mScalingKeys) {
				if (sk.mTime >= aClip.mStartTicks && sk.mTime <= aClip.mEndTicks) {
					mUniqueKeys.insert(sk.mTime);
				}
			}
		}
		std::vector<double> result;
		for (auto entry : mUniqueKeys) {
			result.push_back(entry);
		}
		return result;
	}
}

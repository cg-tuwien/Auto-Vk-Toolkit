#include <gvk.hpp>
#include <glm/gtx/quaternion.hpp>

namespace gvk
{

	void animation::animate_into_strided_target_per_mesh(const animation_clip_data& aClip, double aTime, glm::mat4* aTargetMemory, size_t aMeshStride, std::optional<size_t> aMatricesStride, std::optional<size_t> aMaxMeshes, std::optional<size_t> aMaxBonesPerMesh)
	{
		return animate_into_strided_target_per_mesh(aClip, aTime, bone_matrices_space::mesh_space, aTargetMemory, aMeshStride, aMatricesStride, aMaxMeshes, aMaxBonesPerMesh);
	}

	void animation::animate_into_single_target_buffer(const animation_clip_data& aClip, double aTime, glm::mat4* aTargetMemory)
	{
		return animate_into_single_target_buffer(aClip, aTime, bone_matrices_space::mesh_space, aTargetMemory);
	}

	void animation::animate_into_strided_target_per_mesh(const animation_clip_data& aClip, double aTime, bone_matrices_space aTargetSpace, glm::mat4* aTargetMemory, size_t aMeshStride, std::optional<size_t> aMatricesStride, std::optional<size_t> aMaxMeshes, std::optional<size_t> aMaxBonesPerMesh)
	{
		switch (aTargetSpace) {
		case bone_matrices_space::mesh_space:
			animate(aClip, aTime, [target = reinterpret_cast<uint8_t*>(aTargetMemory), meshStride = aMeshStride, matStride = aMatricesStride.value_or(sizeof(glm::mat4)), maxMeshes = aMaxMeshes.value_or(std::numeric_limits<size_t>::max()), maxBones = aMaxBonesPerMesh.value_or(std::numeric_limits<size_t>::max())]
									(mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
										// Construction of the bone matrix for this node:
										//   1. Bring vertex into bone space
										//   2. Apply transformaton in bone space
										//   3. Convert transformed vertex back to mesh space
										if (aInfo.mMeshAnimationIndex < maxMeshes && aInfo.mMeshLocalBoneIndex < maxBones) {
											*reinterpret_cast<glm::mat4*>(target + aInfo.mMeshAnimationIndex * meshStride + aInfo.mMeshLocalBoneIndex * matStride) = aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
										}
									}
			);
			break;
		case bone_matrices_space::model_space:
			animate(aClip, aTime, [target = reinterpret_cast<uint8_t*>(aTargetMemory), meshStride = aMeshStride, matStride = aMatricesStride.value_or(sizeof(glm::mat4)), maxMeshes = aMaxMeshes.value_or(std::numeric_limits<size_t>::max()), maxBones = aMaxBonesPerMesh.value_or(std::numeric_limits<size_t>::max())]
									(mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
										// Construction of the bone matrix for this node:
										//   1. Bring vertex into bone space
										//   2. Apply transformaton in bone space => MODEL SPACE
										if (aInfo.mMeshAnimationIndex < maxMeshes && aInfo.mMeshLocalBoneIndex < maxBones) {
											*reinterpret_cast<glm::mat4*>(target + aInfo.mMeshAnimationIndex * meshStride + aInfo.mMeshLocalBoneIndex * matStride) = aTransformMatrix * aInverseBindPoseMatrix;
										}
									}
			);
			break;
		default:
			throw gvk::runtime_error("Unknown target space value.");
		}
	}
	
	void animation::animate_into_single_target_buffer(const animation_clip_data& aClip, double aTime, bone_matrices_space aTargetSpace, glm::mat4* aTargetMemory)
	{
		switch (aTargetSpace) {
		case bone_matrices_space::mesh_space:
			animate(aClip, aTime, [aTargetMemory](mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
				// Construction of the bone matrix for this node:
				//   1. Bring vertex into bone space
				//   2. Apply transformaton in bone space
				//   3. Convert transformed vertex back to mesh space
				aTargetMemory[aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex] = aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
			});
			break;
		case bone_matrices_space::model_space:
			animate(aClip, aTime, [aTargetMemory](mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
				// Construction of the bone matrix for this node:
				//   1. Bring vertex into bone space
				//   2. Apply transformaton in bone space => MODEL SPACE
				aTargetMemory[aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex] = aTransformMatrix * aInverseBindPoseMatrix;
			});
			break;
		default:
			throw gvk::runtime_error("Unknown target space value.");
		}
	}

	std::vector<double> animation::animation_key_times_for_clip(const animation_clip_data& aClip) const
	{
		const double cMachineEpsilon = 2.3e-16;
		
		std::set<double> mUniqueKeys;
		for (auto& anode : mAnimationData) {
			// POSITION KEYS:
			for (size_t i = 1; i < anode.mPositionKeys.size(); ++i) {
				if (anode.mPositionKeys[i].mTime >= aClip.mStartTicks - cMachineEpsilon && anode.mPositionKeys[i].mTime <= aClip.mEndTicks + cMachineEpsilon) {
					// Check predecessor:
					if (anode.mPositionKeys[i].mTime - aClip.mStartTicks > cMachineEpsilon) { // => Add PREDECESSOR unless theres is only epsilon-difference to the CURRENT timestep
						mUniqueKeys.insert(anode.mPositionKeys[i - 1].mTime);
					}
					// Definitely add this one:
					mUniqueKeys.insert(anode.mPositionKeys[i].mTime);
					// Check successor:
					if (i + 1 < anode.mPositionKeys.size() && aClip.mEndTicks - anode.mPositionKeys[i].mTime > cMachineEpsilon) { // => Add SUCCESSOR unless theres is only epsilon-difference to the CURRENT timestep
						mUniqueKeys.insert(anode.mPositionKeys[i + 1].mTime);
					}
				}
			}
			// ROTATION KEYS:
			for (size_t i = 1; i < anode.mRotationKeys.size(); ++i) {
				if (anode.mRotationKeys[i].mTime >= aClip.mStartTicks - cMachineEpsilon && anode.mRotationKeys[i].mTime <= aClip.mEndTicks + cMachineEpsilon) {
					// Check predecessor:
					if (anode.mRotationKeys[i].mTime - aClip.mStartTicks > cMachineEpsilon) { // => Add PREDECESSOR unless theres is only epsilon-difference to the CURRENT timestep
						mUniqueKeys.insert(anode.mRotationKeys[i - 1].mTime);
					}
					// Definitely add this one:
					mUniqueKeys.insert(anode.mRotationKeys[i].mTime);
					// Check successor:
					if (i + 1 < anode.mRotationKeys.size() && aClip.mEndTicks - anode.mRotationKeys[i].mTime > cMachineEpsilon) { // => Add SUCCESSOR unless theres is only epsilon-difference to the CURRENT timestep
						mUniqueKeys.insert(anode.mRotationKeys[i + 1].mTime);
					}
				}
			}
			// SCALE KEYS:
			for (size_t i = 1; i < anode.mScalingKeys.size(); ++i) {
				if (anode.mScalingKeys[i].mTime >= aClip.mStartTicks - cMachineEpsilon && anode.mScalingKeys[i].mTime <= aClip.mEndTicks + cMachineEpsilon) {
					// Check predecessor:
					if (anode.mScalingKeys[i].mTime - aClip.mStartTicks > cMachineEpsilon) { // => Add PREDECESSOR unless theres is only epsilon-difference to the CURRENT timestep
						mUniqueKeys.insert(anode.mScalingKeys[i - 1].mTime);
					}
					// Definitely add this one:
					mUniqueKeys.insert(anode.mScalingKeys[i].mTime);
					// Check successor:
					if (i + 1 < anode.mScalingKeys.size() && aClip.mEndTicks - anode.mScalingKeys[i].mTime > cMachineEpsilon) { // => Add SUCCESSOR unless theres is only epsilon-difference to the CURRENT timestep
						mUniqueKeys.insert(anode.mScalingKeys[i + 1].mTime);
					}
				}
			}
		}
		std::vector<double> result;
		for (auto entry : mUniqueKeys) {
			result.push_back(entry);
		}
		return result;
	}

	size_t animation::number_of_animated_nodes() const
	{
		return mAnimationData.size();
	}
	
	std::reference_wrapper<animated_node> animation::get_animated_node_at(size_t aNodeIndex)
	{
		assert(aNodeIndex < mAnimationData.size());
		return std::ref(mAnimationData[aNodeIndex]);
	}

	std::reference_wrapper<const animated_node> animation::get_animated_node_at(size_t aNodeIndex) const
	{
		assert(aNodeIndex < mAnimationData.size());
		return std::cref(mAnimationData[aNodeIndex]);
	}

	std::optional<size_t> animation::get_animated_parent_index_of(size_t aNodeIndex) const
	{
		assert(aNodeIndex < mAnimationData.size());
		return mAnimationData[aNodeIndex].mAnimatedParentIndex;
	}

	std::optional<std::reference_wrapper<animated_node>> animation::get_animated_parent_node_of(size_t aNodeIndex)
	{
		auto parentIndex = get_animated_parent_index_of(aNodeIndex);
		if (parentIndex.has_value()) {
			assert(parentIndex.value() < mAnimationData.size());
			return std::ref(mAnimationData[parentIndex.value()]);
		}
		return {};
	}

	std::optional<std::reference_wrapper<const animated_node>> animation::get_animated_parent_node_of(size_t aNodeIndex) const
	{
		auto parentIndex = get_animated_parent_index_of(aNodeIndex);
		if (parentIndex.has_value()) {
			assert(parentIndex.value() < mAnimationData.size());
			return std::cref(mAnimationData[parentIndex.value()]);
		}
		return {};
	}

	std::vector<size_t> animation::get_child_indices_of(std::optional<size_t> aNodeIndex) const
	{
		std::vector<size_t> result;
		const auto n = mAnimationData.size();
		assert(aNodeIndex < mAnimationData.size());
		for (size_t i = aNodeIndex.value_or(0); i < n; ++i) {
			if (mAnimationData[i].mAnimatedParentIndex == aNodeIndex) {
				result.push_back(i);
			}
		}
		return result;
	}

	std::optional<size_t> animation::get_next_child_index_of(std::optional<size_t> aNodeIndex, size_t aStartSearchOffset) const
	{
		const auto n = mAnimationData.size();
		assert(aNodeIndex < mAnimationData.size());
		for (size_t i = std::max(aNodeIndex.value_or(0), aStartSearchOffset); i < n; ++i) {
			if (mAnimationData[i].mAnimatedParentIndex == aNodeIndex) {
				return i;
			}
		}
		return {};
	}
	
	std::vector<std::reference_wrapper<animated_node>> animation::get_child_nodes_of(std::optional<size_t> aNodeIndex)
	{
		std::vector<std::reference_wrapper<animated_node>> result;
		const auto n = mAnimationData.size();
		assert(aNodeIndex < mAnimationData.size());
		for (size_t i = aNodeIndex.value_or(0); i < n; ++i) {
			if (mAnimationData[i].mAnimatedParentIndex == aNodeIndex) {
				result.push_back(std::ref(mAnimationData[i]));
			}
		}
		return result;
	}

	std::vector<std::reference_wrapper<const animated_node>> animation::get_child_nodes_of(std::optional<size_t> aNodeIndex) const
	{
		std::vector<std::reference_wrapper<const animated_node>> result;
		const auto n = mAnimationData.size();
		assert(aNodeIndex < mAnimationData.size());
		for (size_t i = aNodeIndex.value_or(0); i < n; ++i) {
			if (mAnimationData[i].mAnimatedParentIndex == aNodeIndex) {
				result.push_back(std::cref(mAnimationData[i]));
			}
		}
		return result;
	}
}

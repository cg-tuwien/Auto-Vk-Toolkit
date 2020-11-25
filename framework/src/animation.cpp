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
		case bone_matrices_space::mesh_local_bone_space:
			animate(aClip, aTime, [target = reinterpret_cast<uint8_t*>(aTargetMemory), meshStride = aMeshStride, matStride = aMatricesStride.value_or(sizeof(glm::mat4)), maxMeshes = aMaxMeshes.value_or(std::numeric_limits<size_t>::max()), maxBones = aMaxBonesPerMesh.value_or(std::numeric_limits<size_t>::max())]
									(mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
										// Construction of the bone matrix for this node:
										//   1. Bring vertex into bone space
										//   2. Apply transformaton in bone space
										//   3. Convert transformed vertex to bone space again
										if (aInfo.mMeshAnimationIndex < maxMeshes && aInfo.mMeshLocalBoneIndex < maxBones) {
											*reinterpret_cast<glm::mat4*>(target + aInfo.mMeshAnimationIndex * meshStride + aInfo.mMeshLocalBoneIndex * matStride) = aInverseBindPoseMatrix * aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
										}
									}
			);
			break;
		case bone_matrices_space::global_bone_space:
			animate(aClip, aTime, [target = reinterpret_cast<uint8_t*>(aTargetMemory), meshStride = aMeshStride, matStride = aMatricesStride.value_or(sizeof(glm::mat4)), maxMeshes = aMaxMeshes.value_or(std::numeric_limits<size_t>::max()), maxBones = aMaxBonesPerMesh.value_or(std::numeric_limits<size_t>::max())]
									(mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
										// Construction of the bone matrix for this node:
										//   1. Bring vertex into bone space
										//   2. Apply transformaton in bone space
										//   3. Convert transformed vertex to bone space again
										if (aInfo.mMeshAnimationIndex < maxMeshes && aInfo.mMeshLocalBoneIndex < maxBones) {
											*reinterpret_cast<glm::mat4*>(target + aInfo.mMeshAnimationIndex * meshStride + aInfo.mMeshLocalBoneIndex * matStride) = glm::inverse(aInverseMeshRootMatrix) * aInverseBindPoseMatrix * aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
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
		case bone_matrices_space::mesh_local_bone_space:
			animate(aClip, aTime, [aTargetMemory](mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
				// Construction of the bone matrix for this node:
				//   1. Bring vertex into bone space
				//   2. Apply transformaton in bone space
				//   3. Convert transformed vertex to bone space again, which is relative to the mesh's root
				aTargetMemory[aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex] = aInverseBindPoseMatrix * aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
			});
			break;
		case bone_matrices_space::global_bone_space:
			animate(aClip, aTime, [aTargetMemory](mesh_bone_info aInfo, const glm::mat4& aInverseMeshRootMatrix, const glm::mat4& aTransformMatrix, const glm::mat4& aInverseBindPoseMatrix){
				// Construction of the bone matrix for this node:
				//   1. Bring vertex into bone space
				//   2. Apply transformaton in bone space
				//   3. Convert transformed vertex to bone space again
				aTargetMemory[aInfo.mGlobalBoneIndexOffset + aInfo.mMeshLocalBoneIndex] = glm::inverse(aInverseMeshRootMatrix) * aInverseBindPoseMatrix * aInverseMeshRootMatrix * aTransformMatrix * aInverseBindPoseMatrix;
			});
			break;
		default:
			throw gvk::runtime_error("Unknown target space value.");
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

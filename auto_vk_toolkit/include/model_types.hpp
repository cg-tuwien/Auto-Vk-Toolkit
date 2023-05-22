#pragma once

namespace avk
{
	using model_index_t = size_t;
	using mesh_index_t = size_t;

	/** Convert from an ASSIMP vec3 to a GLM vec3 */
	static glm::vec3 to_vec3(const aiVector3D& aAssimpVector)
	{
		return *reinterpret_cast<const glm::vec3*>(&aAssimpVector.x);
	}

	/** Convert from an ASSIMP quaternion to a GLM quaternion */
	static glm::quat to_quat(const aiQuaternion& aAssimpQuat)
	{
		return glm::quat(aAssimpQuat.w, aAssimpQuat.x, aAssimpQuat.y, aAssimpQuat.z);
	}

	/** Convert from a GLM quaternion to an ASSIMP quaternion */
	static aiQuaternion to_aiQuaternion(const glm::quat& aGlmQuat)
	{
		return aiQuaternion(aGlmQuat.w, aGlmQuat.x, aGlmQuat.y, aGlmQuat.z);
	}

	/** Convert from an ASSIMP mat4 to a GLM mat4 */
	static glm::mat4 to_mat4(const aiMatrix4x4& aAssimpMat)
	{
		return glm::transpose(*reinterpret_cast<const glm::mat4*>(&aAssimpMat[0][0]));
	}

	/** Convert from an ASSIMP string to a std::string */
	static std::string to_string(const aiString& aAssimpString)
	{
		return std::string(aAssimpString.C_Str());
	}

}

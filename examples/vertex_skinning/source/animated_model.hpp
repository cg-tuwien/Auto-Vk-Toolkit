#pragma once
#include "dual_quaternion.hpp"
#include "cor.hpp"

class animated_model {
public:
	struct mesh_data {
		std::vector<uint32_t> mIndices;
		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec3> mNormals;
		std::vector<glm::vec2> mTexCoords;
		std::vector<glm::vec3> mTangents;
		std::vector<glm::vec3> mBiTangents;
		std::vector<glm::uvec4> mBoneIndices;
		std::vector<glm::vec4> mBoneWeights;
		std::vector<glm::vec3> mCentersOfRotation;
		avk::buffer mIndexBuffer;
		avk::buffer mPositionsBuffer;
		avk::buffer mNormalsBuffer;
		avk::buffer mTexCoordsBuffer;
		avk::buffer mTangentsBuffer;
		avk::buffer mBiTangentsBuffer;
		avk::buffer mBoneIndexBuffer;
		avk::buffer mBoneWeightsBuffer;
		avk::buffer mCentersOfRotationBuffer;
		int mMaterialIndex;
	};

	struct part {
		int mMeshIndex;
		glm::mat4 mMeshTransform;
	};

	struct bone_data {
		std::vector<glm::mat4> mBoneMatrices;
		std::vector<dual_quaternion::dual_quaternion_struct> mDualQuaternions;
		avk::buffer mBoneMatricesBuffer;
		avk::buffer mDualQuaternionsBuffer;
	};

	struct material_data {
		std::vector<gvk::material_gpu_data> mMaterialGPUData;
		std::vector<avk::image_sampler> mImageSamplers;
		avk::buffer mMaterialsBuffer;
	};

	animated_model() = default;

	void initialize(
		const std::string& filename,
		const std::string& identifier,
		glm::mat4 model_matrix = glm::mat4(1.0f),
		bool flipUV = false,
		int MAX_BONE_COUNT = 1000,
		bool use_CoR = false,
		int initial_animation_index = 0
	);

	std::string get_identifier();

	void update_bone_matrices();
	void update();

	std::vector<mesh_data> &get_mesh_data();
	std::vector<part> &get_parts();
	bone_data &get_bone_data();
	material_data &get_material_data();
	glm::mat4 get_base_transform();
	glm::mat4 get_model_transform();
	int get_active_animation();
	void switch_skinning_animation();
	void select_skinning_animation(int index);
	bool is_animated();
	bool is_dynamic();
	bool has_bones();
	bool uses_cor();
	std::vector<gvk::animation> &get_animations();
	std::vector<gvk::animation_clip_data> &get_animation_clips();

private:
	static glm::mat4 aiMat4_to_glmMat4(const aiMatrix4x4 &ai);
	void collect_mesh_transforms_from_node(
		int meshId, aiNode *node, const glm::mat4 &accTransform, std::vector<glm::mat4> &transforms);
	void load_or_compute_centers_of_rotations(gvk::model &gvkModel);

	std::string mIdentifier;

	glm::mat4 mModelTrafo{};
	std::vector<mesh_data> mMeshDataAndBuffers;
	std::vector<part> mParts;
	bone_data mBoneData;
	material_data mMaterialData;
	glm::mat4 mBaseTransform{};
	bool mAnyMeshHasBones{};
	bool mHasAnimations{};
	unsigned int mNumAnimations{};
	int mActiveAnimation{};
	float mAnimSpeed = 1.0f;
	float mAnimTime{};
	std::vector<gvk::animation> mAnimations;
	std::vector<gvk::animation_clip_data> mAnimClips;
	bool mFlipTexCoords = false;

	cor_calculator mCoRCalc = cor_calculator(0.1f, 0.1f, false, 128);
	bool mUseCoR = false;
};
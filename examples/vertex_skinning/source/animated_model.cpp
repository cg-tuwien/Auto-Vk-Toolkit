#include "animated_model.hpp"

void animated_model::initialize(
	const std::string& filename,
	const std::string& identifier,
	glm::mat4 model_matrix,
	bool flipUV,
	int MAX_BONE_COUNT,
	skinning_mode skinningMode,
	int initial_animation_index
) {
	mIdentifier = identifier;
	mFlipTexCoords = flipUV;
	mModelTrafo = model_matrix;
	mSkinningMode = skinningMode;

	// NOTE: aiProcess_PreTransformVertices removes bones
	auto gvkModel = gvk::model_t::load_from_file(filename, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

	// check if model has bones
	mAnyMeshHasBones = false;
	for (int i = 0; i < gvkModel->handle()->mNumMeshes; i++) {
		if (gvkModel->handle()->mMeshes[i]->HasBones()) {
			mAnyMeshHasBones = true;
			break;
		}
	}

	mHasAnimations = gvkModel->handle()->HasAnimations();
	mNumAnimations = gvkModel->handle()->mNumAnimations;

	int processed_verts = 0;
	std::vector<glm::uvec4> all_indices;
	std::vector<glm::vec4> all_weights;

	if (mAnyMeshHasBones) {
		all_indices = gvkModel->bone_indices_for_meshes_for_single_target_buffer(gvkModel->select_all_meshes());
		all_weights = gvkModel->bone_weights_for_meshes(gvkModel->select_all_meshes());
	}

	std::cout << "Num Meshes: " << gvkModel->num_meshes() << std::endl;
	std::vector<gvk::material_config> distinctMaterialConfigs;
	for (int meshIndex = 0; meshIndex < gvkModel->num_meshes(); ++meshIndex) {
		gvk::material_config material = gvkModel->material_config_for_mesh(meshIndex);
		material.mCustomData[0] = 1.0f;
		if (material.mAmbientReflectivity == glm::vec4(0)) material.mAmbientReflectivity = glm::vec4(glm::vec3(0.1f), 0);

		int texCoordSet = 0;
		auto& meshData = mMeshDataAndBuffers.emplace_back();
		meshData.mMaterialIndex = meshIndex;
		distinctMaterialConfigs.push_back(material);

		auto selection = gvk::make_models_and_meshes_selection(gvkModel, meshIndex);
		auto [vertices, indices] = gvk::get_vertices_and_indices(selection);
		meshData.mIndices = std::move(indices);
		meshData.mPositions = std::move(vertices);
		meshData.mTexCoords = std::move(
			mFlipTexCoords
				? gvk::get_2d_texture_coordinates_flipped(selection, texCoordSet)
				: gvk::get_2d_texture_coordinates(selection, texCoordSet)
		);
		meshData.mNormals = std::move(gvk::get_normals(selection));
		meshData.mTangents = std::move(gvk::get_tangents(selection));
		meshData.mBiTangents = std::move(gvk::get_bitangents(selection));

		if (mAnyMeshHasBones) {
			meshData.mBoneIndices.resize(meshData.mPositions.size());
			meshData.mBoneWeights.resize(meshData.mPositions.size());
			for (int curr_idx = processed_verts; curr_idx < processed_verts + meshData.mBoneIndices.size(); curr_idx++) {
				meshData.mBoneIndices[curr_idx - processed_verts] = all_indices[curr_idx];
				meshData.mBoneWeights[curr_idx - processed_verts] = all_weights[curr_idx];
			}
			processed_verts = processed_verts + meshData.mPositions.size();
		}

		meshData.mIndexBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::index_buffer_meta::create_from_data(meshData.mIndices)
		);
		meshData.mPositionsBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(meshData.mPositions)
		);
		meshData.mTexCoordsBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(meshData.mTexCoords)
		);
		meshData.mNormalsBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(meshData.mNormals)
		);
		meshData.mTangentsBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(meshData.mTangents)
		);
		meshData.mBiTangentsBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(meshData.mBiTangents)
		);

		if (mAnyMeshHasBones) {
			meshData.mBoneWeightsBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(meshData.mBoneWeights)
			);
			meshData.mBoneIndexBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(meshData.mBoneIndices)
			);
		}

		mBaseTransform = glm::mat4(1.0f);
		std::vector<glm::mat4> transforms;
		collect_mesh_transforms_from_node(static_cast<int>(meshIndex), gvkModel->handle()->mRootNode, mBaseTransform, transforms);
		for (auto& tform : transforms) {
			auto& part = mParts.emplace_back();
			part.mMeshIndex = static_cast<int>(meshIndex);
			part.mMeshTransform = tform;
		}
	}

	auto allMeshIndices = gvkModel->select_all_meshes();
	if (mAnyMeshHasBones) {
		int max_bone_mat_count = 0;
		for (auto meshIdx : allMeshIndices) {
			if (gvkModel->num_bone_matrices(meshIdx) > max_bone_mat_count) {
				max_bone_mat_count = gvkModel->num_bone_matrices(meshIdx);
			}
			if (gvkModel->num_bone_matrices(meshIdx) > MAX_BONE_COUNT) {
				throw avk::runtime_error(
					"Model mesh #" + std::to_string(meshIdx) + " has more bones ("
					+ std::to_string(gvkModel->num_bone_matrices(meshIdx)) + ") than MAX_BONES ("
					+ std::to_string(MAX_BONE_COUNT) + ") !"
				);
			}
		}
		mBoneData.mBoneMatrices.resize(max_bone_mat_count * allMeshIndices.size(), glm::mat4(0));
		mBoneData.mDualQuaternions.resize(
			max_bone_mat_count * allMeshIndices.size(),
			dual_quaternion::dual_quaternion_struct {
				glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
				glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)
			}
		);
	}

	std::cout << "Number Of Animations: " << mNumAnimations << std::endl;
	if (mHasAnimations) {
		size_t total_num_bone_matrices = 0;
		mActiveAnimation = initial_animation_index;

		for (int animId = 0; animId < mNumAnimations; animId++) {
			gvk::animation_clip_data anim_clip = gvkModel->load_animation_clip(animId);
			assert(anim_clip.mTicksPerSecond > 0.0 && anim_clip.mEndTicks > anim_clip.mStartTicks);
			mAnimClips.push_back(anim_clip);
			total_num_bone_matrices += mBoneData.mBoneMatrices.size();
			mAnimations.push_back(gvkModel->prepare_animation(animId, allMeshIndices));
		}

		mBoneData.mBoneMatricesBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::storage_buffer_meta::create_from_size(total_num_bone_matrices * sizeof(glm::mat4))
		);

		mBoneData.mDualQuaternionsBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::storage_buffer_meta::create_from_size(total_num_bone_matrices * sizeof(dual_quaternion::dual_quaternion_struct))
		);
	} else {
		// Allow to render meshes which have no animation
		mBoneData.mBoneMatricesBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::storage_buffer_meta::create_from_size(sizeof(glm::mat4))
		);

		mBoneData.mDualQuaternionsBuffer = gvk::context().create_buffer(
			avk::memory_usage::host_coherent, {},
			avk::storage_buffer_meta::create_from_size(sizeof(dual_quaternion::dual_quaternion_struct))
		);
	}

	auto [gpuMaterials, imageSamplers] = gvk::convert_for_gpu_usage<gvk::material_gpu_data>(
		distinctMaterialConfigs, false, true,
		avk::image_usage::general_texture,
		avk::filter_mode::trilinear,
		avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
	);

	mMaterialData.mMaterialGPUData = std::move(gpuMaterials);
	mMaterialData.mImageSamplers = std::move(imageSamplers);
	mMaterialData.mMaterialsBuffer = gvk::context().create_buffer(
		avk::memory_usage::host_visible, {},
		avk::storage_buffer_meta::create_from_data(mMaterialData.mMaterialGPUData)
	);

	for (auto& md : mMeshDataAndBuffers) {
		md.mIndexBuffer->fill(
			md.mIndices.data(), 0,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);
		md.mPositionsBuffer->fill(
			md.mPositions.data(), 0,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);
		md.mTexCoordsBuffer->fill(
			md.mTexCoords.data(), 0,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);
		md.mNormalsBuffer->fill(
			md.mNormals.data(), 0,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);
		md.mTangentsBuffer->fill(
			md.mTangents.data(), 0,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);
		md.mBiTangentsBuffer->fill(
			md.mBiTangents.data(), 0,
			avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
		);
		if (mAnyMeshHasBones) {
			md.mBoneWeightsBuffer->fill(
				md.mBoneWeights.data(), 0,
				avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
			);
			md.mBoneIndexBuffer->fill(
				md.mBoneIndices.data(), 0,
				avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
			);
		} else {
			// Initialize empty bones buffer for shader.
			md.mBoneWeights = std::vector<glm::vec4>(1);
			md.mBoneWeightsBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(md.mBoneWeights));

			md.mBoneIndices = std::vector<glm::uvec4>(1);
			md.mBoneIndexBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(md.mBoneIndices));
		}
	}

	mMaterialData.mMaterialsBuffer->fill(mMaterialData.mMaterialGPUData.data(), 0, avk::sync::not_required());


	if (mSkinningMode == skinning_mode::optimized_centers_of_rotation_skinning) {
		std::cout << "USING Optimized Centers Of Rotation Skinning." << std::endl;
		load_or_compute_centers_of_rotations(gvkModel);
	} else {
		for (int meshIndex = 0; meshIndex < gvkModel->num_meshes(); ++meshIndex) {
			// Initialize empty centers of rotation buffer for shader.
			mMeshDataAndBuffers[meshIndex].mCentersOfRotation =
				std::vector<glm::vec3>(mMeshDataAndBuffers[meshIndex].mIndices.size());
			mMeshDataAndBuffers[meshIndex].mCentersOfRotationBuffer = gvk::context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_data(mMeshDataAndBuffers[meshIndex].mCentersOfRotation));
			mMeshDataAndBuffers[meshIndex].mCentersOfRotationBuffer->fill(
				mMeshDataAndBuffers[meshIndex].mCentersOfRotation.data(), 0,
				avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler()));
		}
	}

	update();
	update_bone_matrices();
}

std::string animated_model::get_identifier() {
	return  mIdentifier;
}

animated_model::skinning_mode animated_model::get_skinning_mode() {
	return mSkinningMode;
}

void animated_model::update_bone_matrices() {
	if (!mHasAnimations) return;

	gvk::animation anim = mAnimations[mActiveAnimation];
	anim.animate_into_single_target_buffer(
		mAnimClips[mActiveAnimation],
		static_cast<double>(mAnimTime),
		gvk::bone_matrices_space::model_space,
		mBoneData.mBoneMatrices.data()
	);
	
	for (int i = 0; i < mBoneData.mBoneMatrices.size(); i++) {
		auto m = glm::inverse(mParts[0].mMeshTransform) * mBoneData.mBoneMatrices[i];
		mBoneData.mBoneMatrices[i] = m;
		mBoneData.mDualQuaternions[i] = dual_quaternion::to_struct(dual_quaternion::from_mat4(m));
	}

	mBoneData.mBoneMatricesBuffer->fill(
		mBoneData.mBoneMatrices.data(), 0, 0,
		mBoneData.mBoneMatrices.size() * sizeof(glm::mat4),
		avk::sync::not_required()
	);

	mBoneData.mDualQuaternionsBuffer->fill(
		mBoneData.mDualQuaternions.data(), 0, 0,
		mBoneData.mDualQuaternions.size() * sizeof(dual_quaternion::dual_quaternion_struct),
		avk::sync::not_required()
	);
}

void animated_model::update() {
	if (mHasAnimations) {
		static float tLast = 0.f;
		float t = static_cast<float>(glfwGetTime());
		float dt = tLast ? t - tLast : 0.f;
		tLast = t;
		static float tObjAccumulated = 0.f;
		tObjAccumulated += dt;
		auto& clip = mAnimClips[mActiveAnimation];
		float minAnimTime = static_cast<float>(clip.mStartTicks / clip.mTicksPerSecond);
		float maxAnimTime = static_cast<float>(clip.mEndTicks / clip.mTicksPerSecond);
		float timePassed = fmod(abs(mAnimSpeed) * tObjAccumulated, maxAnimTime - minAnimTime);
		mAnimTime = (mAnimSpeed < 0.0f) ? maxAnimTime - timePassed : minAnimTime + timePassed;
	}
}

std::vector<animated_model::mesh_data>& animated_model::get_mesh_data() {
	return mMeshDataAndBuffers;
}

std::vector<animated_model::part>& animated_model::get_parts() {
	return mParts;
}

animated_model::bone_data & animated_model::get_bone_data() {
	return mBoneData;
}

animated_model::material_data & animated_model::get_material_data() {
	return mMaterialData;
}

glm::mat4 animated_model::get_base_transform() {
	return mBaseTransform;
}

glm::mat4 animated_model::get_model_transform() {
	return mModelTrafo;
}

int animated_model::get_active_animation() {
	return mActiveAnimation;
}

void animated_model::switch_skinning_animation() {
	mActiveAnimation = mActiveAnimation + 1;
	if (mActiveAnimation >= mNumAnimations) {
		mActiveAnimation = 0;
	}
}

void animated_model::select_skinning_animation(int index) {
	mActiveAnimation = index;
}

bool animated_model::is_animated() {
	return mHasAnimations;
}

bool animated_model::is_dynamic() {
	return mHasAnimations;
}

bool animated_model::has_bones() {
	return mAnyMeshHasBones;
}

bool animated_model::uses_cor() {
	return mSkinningMode == skinning_mode::optimized_centers_of_rotation_skinning;
}

std::vector<gvk::animation>& animated_model::get_animations() {
	return mAnimations;
}

std::vector<gvk::animation_clip_data>& animated_model::get_animation_clips() {
	return mAnimClips;
}

glm::mat4 animated_model::aiMat4_to_glmMat4(const aiMatrix4x4& ai) {
	glm::mat4 g;
	g[0][0] = ai[0][0]; g[0][1] = ai[1][0]; g[0][2] = ai[2][0]; g[0][3] = ai[3][0];
	g[1][0] = ai[0][1]; g[1][1] = ai[1][1]; g[1][2] = ai[2][1]; g[1][3] = ai[3][1];
	g[2][0] = ai[0][2]; g[2][1] = ai[1][2]; g[2][2] = ai[2][2]; g[2][3] = ai[3][2];
	g[3][0] = ai[0][3]; g[3][1] = ai[1][3]; g[3][2] = ai[2][3]; g[3][3] = ai[3][3];
	return g;
}

void animated_model::collect_mesh_transforms_from_node(
	int meshId,
	aiNode* node,
	const glm::mat4& accTransform,
	std::vector<glm::mat4>& transforms
) {
	glm::mat4 newAccTransform = accTransform * aiMat4_to_glmMat4(node->mTransformation);
	for (size_t i = 0; i < node->mNumMeshes; i++) {
		if (node->mMeshes[i] == meshId) {
			transforms.push_back(newAccTransform);
		}
	}
	for (size_t iChild = 0; iChild < node->mNumChildren; iChild++) {
		collect_mesh_transforms_from_node(meshId, node->mChildren[iChild], newAccTransform, transforms);
	}
}

void animated_model::load_or_compute_centers_of_rotations(gvk::model& gvkModel) {
	int meshIndex = 0; // for now just one mesh
	const std::string cached_file_path = "cached_" + mIdentifier;
	const std::string cached_file_path_bin = cached_file_path + ".cors";
	const std::string cached_file_path_txt = cached_file_path + ".txt";
	bool finished_calc = false;

	if (!std::filesystem::exists(cached_file_path_bin)) {
		auto num_verts = gvkModel->handle()->mMeshes[meshIndex]->mNumVertices;
		auto num_bones = gvkModel->handle()->mMeshes[meshIndex]->mNumBones;
		
		std::vector<glm::vec3> vertices = mMeshDataAndBuffers[meshIndex].mPositions;
		std::vector<uint32_t> indices = mMeshDataAndBuffers[meshIndex].mIndices;

		std::vector<std::vector<unsigned int>> bone_indices;
		std::vector<std::vector<float>> bone_weights;
		
		for (int i = 0; i < num_verts; i++) {
			auto vi = mMeshDataAndBuffers[meshIndex].mBoneIndices[i];
			auto vw = mMeshDataAndBuffers[meshIndex].mBoneWeights[i];
			std::vector<unsigned int> inds;
			std::vector<float> wgts;
			for (int j = 0; j < 4; j++) {
				auto idx = vi[j];
				auto wgt = vw[j];
				inds.push_back(idx);
				wgts.push_back(wgt);
			}
			bone_indices.push_back(inds);
			bone_weights.push_back(wgts);
		}

		std::vector<weights_per_bone> weights_per_bone =
			mCoRCalc.convert_weights(num_bones, bone_indices, bone_weights);
		
		// normalize mWeights
		for (int i = 0; i < weights_per_bone.size(); i++) {
			float sum = 0.0f;
			for (int j = 0; j < weights_per_bone[i].mWeights.size(); j++) {
				sum = sum + weights_per_bone[i].mWeights[j];
			}
			if (sum != 0.0f) {
				for (int j = 0; j < weights_per_bone[i].mWeights.size(); j++) {
					weights_per_bone[i].mWeights[j] = weights_per_bone[i].mWeights[j] / sum;
				}
			}
			else {
				weights_per_bone[i].mWeights[0] = 1.0f;
			}
		}

		cor_calculator c(0.1f, 0.1f, false, 128);

		cor_mesh mesh = c.create_cor_mesh(vertices, indices, weights_per_bone, 0.1f);

		bool finished_calc = false;

		c.calculate_cors_async(
			mesh,
			[&c, &finished_calc, cached_file_path_bin, cached_file_path_txt](std::vector<glm::vec3> &cors) {
				c.save_cors_to_binary_file(cached_file_path_bin, cors);
				c.save_cors_to_text_file(cached_file_path_txt, cors);
				finished_calc = true;
			});

		// wait for threads to finish
		while (!finished_calc) {
			Sleep(300);
		}
		LOG_INFO(std::string("All threads of CoRs calculation joined."));
	}

	if (!std::filesystem::exists(cached_file_path_bin)) {
		throw avk::runtime_error(
			"Just finished export of cors but could not locate cache file !"
		);
	}

	mMeshDataAndBuffers[meshIndex].mCentersOfRotation =
		mCoRCalc.load_cors_from_binary_file(cached_file_path_bin);
	for (int i = 0; i < mMeshDataAndBuffers[meshIndex].mCentersOfRotation.size(); i++) {
		auto m = mParts[meshIndex].mMeshTransform;
		mMeshDataAndBuffers[meshIndex].mCentersOfRotation[i] = glm::vec3(m * glm::vec4(mMeshDataAndBuffers[meshIndex].mCentersOfRotation[i], 1.0f));
	}

	mMeshDataAndBuffers[meshIndex].mCentersOfRotationBuffer = gvk::context().create_buffer(
		avk::memory_usage::device, {},
		avk::vertex_buffer_meta::create_from_data(mMeshDataAndBuffers[meshIndex].mCentersOfRotation)
	);
	mMeshDataAndBuffers[meshIndex].mCentersOfRotationBuffer->fill(
		mMeshDataAndBuffers[meshIndex].mCentersOfRotation.data(), 0,
		avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
	);
	std::cout << "\t\tNumber of Positions:" << mMeshDataAndBuffers[meshIndex].mPositions.size() << std::endl;
	std::cout << "\t\tNumber of CORS:" << mMeshDataAndBuffers[meshIndex].mCentersOfRotation.size() << std::endl;
}
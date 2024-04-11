
#include <sstream>

#include "model.hpp"
#include <assimp/qnan.h>

namespace avk
{

	avk::owning_resource<model_t> model_t::load_from_file(const std::string& aPath, aiProcessFlagsType aAssimpFlags)
	{
		model_t result;
		result.mModelPath = avk::clean_up_path(aPath);
		result.mImporter = std::make_unique<Assimp::Importer>();
		result.mScene = result.mImporter->ReadFile(aPath, aAssimpFlags);
		if (nullptr == result.mScene) {
			throw avk::runtime_error(std::format("Loading model from '{}' failed.", aPath));
		}
		result.initialize_materials();
		return result;
	}
	
	avk::owning_resource<model_t> model_t::load_from_memory(const std::string& aMemory, aiProcessFlagsType aAssimpFlags)
	{
		model_t result;
		result.mModelPath = "";
		result.mImporter = std::make_unique<Assimp::Importer>();
		result.mScene = result.mImporter->ReadFileFromMemory(aMemory.c_str(), aMemory.size(), aAssimpFlags);
		if (nullptr == result.mScene) {
			throw avk::runtime_error("Loading model from memory failed.");
		}
		result.initialize_materials();
		return result;
	}

	
	void model_t::initialize_materials()
	{
		auto n = static_cast<size_t>(mScene->mNumMeshes);
		mMaterialConfigPerMesh.reserve(n);
		for (size_t i = 0; i < n; ++i) {
			mMaterialConfigPerMesh.emplace_back();
		}
	}

	aiNode* model_t::find_mesh_root_node(unsigned int aMeshIndexToFind) const
	{
		return mesh_node_traverser(aMeshIndexToFind, mScene->mRootNode);
	}
	
	aiNode* model_t::mesh_node_traverser(unsigned int aMeshIndexToFind, aiNode* aNode) const
	{
		for (unsigned int i = 0; i < aNode->mNumMeshes; i++)
		{
			if (aNode->mMeshes[i] == aMeshIndexToFind) {
				return aNode;
			}
		}
		// Not found => go deeper
		for (unsigned int i = 0; i < aNode->mNumChildren; i++)
		{
			auto* node = mesh_node_traverser(aMeshIndexToFind, aNode->mChildren[i]);
			if (nullptr != node) {
				return node;
			}
		}
		return nullptr;
	}
	
	std::optional<glm::mat4> model_t::transformation_matrix_traverser(unsigned int aMeshIndexToFind, const aiNode* aNode, const aiMatrix4x4& aM) const
	{
		aiMatrix4x4 nodeM = aM * aNode->mTransformation;
		for (unsigned int i = 0; i < aNode->mNumMeshes; i++)
		{
			if (aNode->mMeshes[i] == aMeshIndexToFind) {
				return to_mat4(nodeM);
			}
		}
		// Not found => go deeper
		for (unsigned int i = 0; i < aNode->mNumChildren; i++)
		{
			auto mat = transformation_matrix_traverser(aMeshIndexToFind, aNode->mChildren[i], nodeM);
			if (mat.has_value()) {
				return mat;
			}
		}
		return {};
	}

	glm::mat4 model_t::transformation_matrix_for_mesh(mesh_index_t aMeshIndex) const
	{
		// Find the mesh in Assim's node hierarchy
		return transformation_matrix_traverser(static_cast<unsigned int>(aMeshIndex), mScene->mRootNode, aiMatrix4x4{}).value();
	}

	glm::mat4 model_t::mesh_root_matrix(mesh_index_t aMeshIndex) const
	{
		return transformation_matrix_for_mesh(aMeshIndex);
	}

	uint32_t model_t::num_actual_bones(mesh_index_t aMeshIndex) const
	{
		assert(mScene->mNumMeshes > aMeshIndex && 0 <= aMeshIndex);
		if (!mScene->mMeshes[aMeshIndex]->HasBones()) {
			return 0u;
		}
		return static_cast<uint32_t>(mScene->mMeshes[aMeshIndex]->mNumBones);
	}
	
	uint32_t model_t::num_bone_matrices(mesh_index_t aMeshIndex) const
	{
		return std::max(num_actual_bones(aMeshIndex), 1u); // :O what's that, a minimum of one bone matrix? You are seeing it right. That's one bone matrix at least => animation needs that for meshes participating in animation but without bone assignments (i.e. no real indices)
	}

	uint32_t model_t::num_bone_matrices(std::vector<mesh_index_t> aMeshIndices) const
	{
		uint32_t accumulatedCount = 0u;
		for (auto mi : aMeshIndices) {
			accumulatedCount += num_bone_matrices(mi);
		}
		return accumulatedCount;
	}
	
	std::vector<glm::mat4> model_t::inverse_bind_pose_matrices(mesh_index_t aMeshIndex, bone_matrices_space aSourceSpace) const
	{
		assert(mScene->mNumMeshes > aMeshIndex && 0 <= aMeshIndex);
		std::vector<glm::mat4> result;
		if (mScene->mMeshes[aMeshIndex]->HasBones()) {
			auto nb = mScene->mMeshes[aMeshIndex]->mNumBones;
			switch (aSourceSpace) {
			case bone_matrices_space::mesh_space:
				for (decltype(nb) i = 0; i < nb; ++i) {
					result.push_back(to_mat4(mScene->mMeshes[aMeshIndex]->mBones[i]->mOffsetMatrix));
				}
				break;
			case bone_matrices_space::model_space:
				{
					const auto meshRootMatrix = transformation_matrix_for_mesh(aMeshIndex);
					const auto inverseMeshRootMatrix = glm::inverse(meshRootMatrix);
				
					for (decltype(nb) i = 0; i < nb; ++i) {
						result.push_back(meshRootMatrix * to_mat4(mScene->mMeshes[aMeshIndex]->mBones[i]->mOffsetMatrix) * inverseMeshRootMatrix);
					}
				}
				break;
			default:
				throw avk::runtime_error("Given source space is not supported. Supported are bone_matrices_space::mesh_space and bone_matrices_space::model_space.");
			}
		}
		else {
			result.emplace_back(1.0f); // Identity matrix
		}
		return result;
	}

	std::string model_t::name_of_mesh(mesh_index_t aMeshIndex) const
	{
		assert(mScene->mNumMeshes > aMeshIndex && 0 <= aMeshIndex);
		return mScene->mMeshes[aMeshIndex]->mName.data;
	}

	size_t model_t::material_index_for_mesh(mesh_index_t aMeshIndex) const
	{
		assert(mScene->mNumMeshes > aMeshIndex && 0 <= aMeshIndex);
		return mScene->mMeshes[aMeshIndex]->mMaterialIndex;
	}

	std::string model_t::name_of_material(size_t aMaterialIndex) const
	{
		aiMaterial* pMaterial = mScene->mMaterials[aMaterialIndex];
		if (!pMaterial) return "";
		aiString name;
		if (pMaterial->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
			return name.data;
		else
			return "";
	}

	material_config model_t::material_config_for_mesh(mesh_index_t aMeshIndex)
	{
		assert (mMaterialConfigPerMesh.size() > aMeshIndex);
		if (mMaterialConfigPerMesh[aMeshIndex].has_value()) {
			return mMaterialConfigPerMesh[aMeshIndex].value();
		}
		
		material_config result;

		aiString strVal;
		aiColor3D color(0.0f, 0.0f, 0.0f);
		int intVal;
		aiBlendMode blendMode;
		float floatVal;
		aiTextureMapping texMapping;
		unsigned uvSetIndex;
		std::array<aiTextureMapMode, 2> texMappingModes {};
		aiUVTransform uvTransformVal;

		auto toAvkBorderHandling = [](aiTextureMapMode in) {
			switch (in) {
			case aiTextureMapMode_Wrap:		return avk::border_handling_mode::repeat;
			case aiTextureMapMode_Clamp:	return avk::border_handling_mode::clamp_to_edge;
			case aiTextureMapMode_Decal:	return avk::border_handling_mode::clamp_to_edge;
			case aiTextureMapMode_Mirror:	return avk::border_handling_mode::mirrored_repeat;
			default:						return avk::border_handling_mode::clamp_to_edge;
			}
		};
		
		auto materialIndex = material_index_for_mesh(aMeshIndex);
		assert(materialIndex <= mScene->mNumMaterials);
		aiMaterial* aimat = mScene->mMaterials[materialIndex];

		// CPU-only parameters:
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_NAME, strVal)) {
			result.mName = strVal.data;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHADING_MODEL, intVal)) {
			switch (static_cast<aiShadingMode>(intVal)) {
			case aiShadingMode_Flat: 
				result.mShadingModel = "Flat";
				break;
			case aiShadingMode_Gouraud: 
				result.mShadingModel = "Gouraud";
				break;
			case aiShadingMode_Phong: 
				result.mShadingModel = "Phong";
				break;
			case aiShadingMode_Blinn: 
				result.mShadingModel = "Blinn";
				break;
			case aiShadingMode_Toon: 
				result.mShadingModel = "Toon";
				break;
			case aiShadingMode_OrenNayar: 
				result.mShadingModel = "OrenNayar";
				break;
			case aiShadingMode_Minnaert: 
				result.mShadingModel = "Minnaert";
				break;
			case aiShadingMode_CookTorrance: 
				result.mShadingModel = "CookTorrance";
				break;
			case aiShadingMode_NoShading: 
				result.mShadingModel = "NoShading";
				break;
			case aiShadingMode_Fresnel: 
				result.mShadingModel = "Fresnel";
				break;
			default: 
				result.mShadingModel = "";
			}
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_ENABLE_WIREFRAME, intVal)) {
			result.mWireframeMode = 0 != intVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_TWOSIDED, intVal)) {
			result.mTwosided = 0 != intVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_BLEND_FUNC, blendMode)) {
			result.mBlendMode = blendMode == aiBlendMode_Additive
				? avk::cfg::color_blending_config::enable_additive_for_all_attachments()
				: avk::cfg::color_blending_config::enable_alpha_blending_for_all_attachments();
		}

		// Shader parameters:
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
			result.mDiffuseReflectivity = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_AMBIENT, color)) {
			result.mAmbientReflectivity = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
			result.mSpecularReflectivity = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_EMISSIVE, color)) {
			result.mEmissiveColor = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_TRANSPARENT, color)) {
			result.mTransparentColor = glm::vec4(color.r, color.g, color.b, 0.0f);
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_REFLECTIVE, color)) {
			result.mReflectiveColor = glm::vec4(color.r, color.g, color.b, 0.0f);
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_OPACITY, floatVal)) {
			result.mOpacity = floatVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_BUMPSCALING, floatVal)) {
			result.mBumpScaling = floatVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHININESS, floatVal)) {
			result.mShininess = floatVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHININESS_STRENGTH, floatVal)) {
			result.mShininessStrength = floatVal;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_REFRACTI, floatVal)) {
			result.mRefractionIndex = floatVal;
		}
		if (AI_SUCCESS == aimat->Get(AI_MATKEY_REFLECTIVITY, floatVal)) {
			result.mReflectivity = floatVal;
		}
		
		// TODO: reading aiTextureMapMode (last parameter) corrupts stack for some .dae files (like "level01c.dae")
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DIFFUSE, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mDiffuseTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mDiffuseTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mDiffuseTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mDiffuseTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);
			
			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_DIFFUSE(0), uvTransformVal)) {
				result.mDiffuseTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mDiffuseTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mDiffuseTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mDiffuseTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mDiffuseTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mDiffuseTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mDiffuseTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_SPECULAR, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mSpecularTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mSpecularTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mSpecularTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mSpecularTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_SPECULAR(0), uvTransformVal)) {
				result.mSpecularTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mSpecularTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mSpecularTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mSpecularTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mSpecularTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mSpecularTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mSpecularTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_AMBIENT, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mAmbientTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mAmbientTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mAmbientTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mAmbientTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_AMBIENT(0), uvTransformVal)) {
				result.mAmbientTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mAmbientTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mAmbientTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mAmbientTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mAmbientTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mAmbientTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mAmbientTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_EMISSIVE, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mEmissiveTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mEmissiveTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mEmissiveTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mEmissiveTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_EMISSIVE(0), uvTransformVal)) {
				result.mEmissiveTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mEmissiveTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mEmissiveTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mEmissiveTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mEmissiveTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mEmissiveTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mEmissiveTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_HEIGHT, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mHeightTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mHeightTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mHeightTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mHeightTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_HEIGHT(0), uvTransformVal)) {
				result.mHeightTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mHeightTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mHeightTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mHeightTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mHeightTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mHeightTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mHeightTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_NORMALS, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mNormalsTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mNormalsTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mNormalsTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mNormalsTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_NORMALS(0), uvTransformVal)) {
				result.mNormalsTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mNormalsTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mNormalsTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mNormalsTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mNormalsTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mNormalsTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mNormalsTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_SHININESS, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mShininessTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mShininessTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mShininessTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mShininessTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_SHININESS(0), uvTransformVal)) {
				result.mShininessTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mShininessTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mShininessTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mShininessTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mShininessTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mShininessTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mShininessTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_OPACITY, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mOpacityTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mOpacityTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mOpacityTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mOpacityTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_OPACITY(0), uvTransformVal)) {
				result.mOpacityTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mOpacityTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mOpacityTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mOpacityTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mOpacityTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mOpacityTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mOpacityTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DISPLACEMENT, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mDisplacementTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mDisplacementTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mDisplacementTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mDisplacementTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_DISPLACEMENT(0), uvTransformVal)) {
				result.mDisplacementTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mDisplacementTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mDisplacementTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mDisplacementTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mDisplacementTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mDisplacementTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mDisplacementTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_REFLECTION, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mReflectionTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mReflectionTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mReflectionTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mReflectionTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_REFLECTION(0), uvTransformVal)) {
				result.mReflectionTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mReflectionTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mReflectionTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mReflectionTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mReflectionTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mReflectionTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mReflectionTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}
		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_LIGHTMAP, 0, &strVal, &texMapping, &uvSetIndex, nullptr, nullptr, texMappingModes.data())) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.mLightmapTex = avk::combine_paths(avk::extract_base_path(mModelPath), strVal.data);
			result.mLightmapTexUvSet = static_cast<uint32_t>(uvSetIndex);
			result.mLightmapTexBorderHandlingMode[0] = toAvkBorderHandling(texMappingModes[0]);
			result.mLightmapTexBorderHandlingMode[1] = toAvkBorderHandling(texMappingModes[1]);

			if (AI_SUCCESS == aimat->Get(AI_MATKEY_UVTRANSFORM_LIGHTMAP(0), uvTransformVal)) {
				result.mLightmapTexOffsetTiling[0] = uvTransformVal.mTranslation.x;
				result.mLightmapTexOffsetTiling[1] = uvTransformVal.mTranslation.y;
				if (avk::equals_one_of(result.mLightmapTexBorderHandlingMode[0], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mLightmapTexOffsetTiling[2] = uvTransformVal.mScaling.x;
				}
				if (avk::equals_one_of(result.mLightmapTexBorderHandlingMode[1], avk::border_handling_mode::repeat, avk::border_handling_mode::mirrored_repeat)) {
					result.mLightmapTexOffsetTiling[3] = uvTransformVal.mScaling.y;
				}
				result.mLightmapTexRotation = static_cast<float>(uvTransformVal.mRotation);
			}
		}

		mMaterialConfigPerMesh[aMeshIndex] = result; // save
		return result;
	}

	void model_t::set_material_config_for_mesh(mesh_index_t aMeshIndex, const material_config& aMaterialConfig)
	{
		assert(aMeshIndex < mMaterialConfigPerMesh.size());
		mMaterialConfigPerMesh[aMeshIndex] = aMaterialConfig;
	}

	std::unordered_map<material_config, std::vector<size_t>> model_t::distinct_material_configs(bool aAlsoConsiderCpuOnlyDataForDistinctMaterials)
	{
		assert(mScene);
		std::unordered_map<material_config, std::vector<size_t>> result;
		auto n = mScene->mNumMeshes;
		for (decltype(n) i = 0; i < n; ++i) {
			const aiMesh* paiMesh = mScene->mMeshes[i];
			auto matConf = material_config_for_mesh(i);
			matConf.mIgnoreCpuOnlyDataForEquality = !aAlsoConsiderCpuOnlyDataForDistinctMaterials;
			result[matConf].emplace_back(i);
		}
		return result;
	}

	size_t model_t::number_of_vertices_for_mesh(mesh_index_t aMeshIndex) const
	{
		assert(mScene);
		assert(aMeshIndex < mScene->mNumMeshes);
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		return static_cast<size_t>(paiMesh->mNumVertices);
	}

	std::vector<glm::vec3> model_t::positions_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		for (decltype(n) i = 0; i < n; ++i) {
			result.push_back(glm::vec3(paiMesh->mVertices[i][0], paiMesh->mVertices[i][1], paiMesh->mVertices[i][2]));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::normals_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		if (nullptr == paiMesh->mNormals) {
			LOG_WARNING(std::format("The mesh at index {} does not contain normals. Will return (0,0,1) normals for each vertex.", aMeshIndex));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(0.f, 0.f, 1.f);
			}
		}
		else {
			// We've got normals. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(paiMesh->mNormals[i][0], paiMesh->mNormals[i][1], paiMesh->mNormals[i][2]);
			}
		}
		return result;
	}

	std::vector<glm::vec3> model_t::tangents_for_mesh(mesh_index_t aMeshIndex) const
	{
		std::vector<glm::vec3> result;

		if (mTangentsAndBitangents.contains(aMeshIndex)) {
			result = std::get<0>(mTangentsAndBitangents.at(aMeshIndex));
		}
		else {
			const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
			auto n = paiMesh->mNumVertices;
			result.reserve(n);
			if (nullptr == paiMesh->mTangents) {
				LOG_WARNING(std::format("The mesh at index {} does not contain tangents. Will return (1,0,0) tangents for each vertex.", aMeshIndex));
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(1.f, 0.f, 0.f);
				}
			}
			else {
				// We've got tangents. Proceed as planned.
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTangents[i][0], paiMesh->mTangents[i][1], paiMesh->mTangents[i][2]);
				}
			}
		}

		return result;
	}

	std::vector<glm::vec3> model_t::bitangents_for_mesh(mesh_index_t aMeshIndex) const
	{
		std::vector<glm::vec3> result;

		if (mTangentsAndBitangents.contains(aMeshIndex)) {
			result = std::get<1>(mTangentsAndBitangents.at(aMeshIndex));
		}
		else {
			const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
			auto n = paiMesh->mNumVertices;
			result.reserve(n);
			if (nullptr == paiMesh->mBitangents) {
				LOG_WARNING(std::format("The mesh at index {} does not contain bitangents. Will return (0,1,0) bitangents for each vertex.", aMeshIndex));
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(0.f, 1.f, 0.f);
				}
			}
			else {
				// We've got bitangents. Proceed as planned.
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mBitangents[i][0], paiMesh->mBitangents[i][1], paiMesh->mBitangents[i][2]);
				}
			}
		}

		return result;
	}

	std::vector<glm::vec4> model_t::colors_for_mesh(mesh_index_t aMeshIndex, int aSet) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec4> result;
		result.reserve(n);
		assert(aSet >= 0 && aSet < AI_MAX_NUMBER_OF_COLOR_SETS);
		if (nullptr == paiMesh->mColors[aSet]) {
			LOG_WARNING(std::format("The mesh at index {} does not contain a color set at index {}. Will return opaque magenta for each vertex.", aMeshIndex, aSet));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(1.f, 0.f, 1.f, 1.f);
			}
		}
		else {
			// We've got colors[_Set]. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(paiMesh->mColors[aSet][i][0], paiMesh->mColors[aSet][i][1], paiMesh->mColors[aSet][i][2], paiMesh->mColors[aSet][i][3]);
			}
		}
		return result;
	}

	std::vector<glm::vec4> model_t::bone_weights_for_mesh(mesh_index_t aMeshIndex, bool aNormalizeBoneWeights) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec4> result;
		result.reserve(n);
		if (!paiMesh->HasBones()) {
			LOG_WARNING(std::format("The mesh at index {} does not contain bones. Will return (1,0,0,0) bone weights for each vertex.", aMeshIndex));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(1.f, 0.f, 0.f, 0.f);
			}
		}
		else {
			// read bone indices and weights for bone animation
			std::vector<std::vector<std::tuple<uint32_t, float>>> vTempWeightsPerVertex(paiMesh->mNumVertices);
			for (unsigned int j = 0; j < paiMesh->mNumBones; j++) 
			{
				const aiBone* pBone = paiMesh->mBones[j];
				for (uint32_t b = 0; b < pBone->mNumWeights; b++) 
				{
					vTempWeightsPerVertex[pBone->mWeights[b].mVertexId].emplace_back(j, pBone->mWeights[b].mWeight);
				}
			}
			
			// We've got bone weights. Proceed as planned.
			bool hasNonNormalizedBoneWeights = false;
			for (decltype(n) i = 0; i < n; ++i) {
				// sort the current vertex' <bone id, weight> pairs descending by weight (so we can take the four most important ones)
				std::sort(vTempWeightsPerVertex[i].begin(), vTempWeightsPerVertex[i].end(), [](std::tuple<uint32_t, float> a, std::tuple<uint32_t, float> b) { return std::get<float>(a) > std::get<float>(b); });

				auto& weights = result.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
				const auto numIndexWeightPairs = std::min(int{ 4 }, static_cast<int>(vTempWeightsPerVertex[i].size()));
				for (int j = 0; j < numIndexWeightPairs; ++j) {
					weights[j] = std::get<float>(vTempWeightsPerVertex[i][j]);
				}

				// "normalize" the weights, if requested, so they add up to one
				if (aNormalizeBoneWeights) {
					// Blender can save meshes with a total weight sum > 1. So first scale down by the total sum (we need to consider all weights, not only the first four!)
					float sum = 0.0f;
					for (auto& w : vTempWeightsPerVertex[i]) {
						sum += std::get<float>(w);
					}
					if (sum > 0.0f) {
						weights /= sum;
					}
					hasNonNormalizedBoneWeights = sum > 1.001f || hasNonNormalizedBoneWeights;
					// if we have more than 4 weights, assign all the unconsidered ones to the 4th bone
					weights.w = 1.0f - weights.x - weights.y - weights.z;
				}
			}
			if (hasNonNormalizedBoneWeights) {
				LOG_WARNING(std::format("The mesh at index {} contains non-normalized bone weights, adding up to more than 1.001.", aMeshIndex));
			}
		}
		return result;
	}

	std::vector<glm::uvec4> model_t::bone_indices_for_mesh(mesh_index_t aMeshIndex, uint32_t aBoneIndexOffset) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::uvec4> result;
		result.reserve(n);
		if (!paiMesh->HasBones()) {
			const uint32_t fallbackIndex = aBoneIndexOffset;
			LOG_WARNING(std::format("The mesh at index {} does not contain bones. Will return ({},{},{},{}) bone indices for each vertex.", aMeshIndex, fallbackIndex, fallbackIndex, fallbackIndex, fallbackIndex));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(fallbackIndex, fallbackIndex, fallbackIndex, fallbackIndex);
			}
		}
		else {
			// read bone indices and weights for bone animation
			std::vector<std::vector<std::tuple<uint32_t, float>>> vTempWeightsPerVertex(paiMesh->mNumVertices);
			for (unsigned int j = 0; j < paiMesh->mNumBones; j++) 
			{
				const aiBone * pBone = paiMesh->mBones[j];
				for (uint32_t b = 0; b < pBone->mNumWeights; b++) 
				{
					vTempWeightsPerVertex[pBone->mWeights[b].mVertexId].emplace_back(j, pBone->mWeights[b].mWeight);
				}
			}
			
			// We've got bone weights. Proceed as planned.
			for (decltype(n) i = 0; i < n; ++i) {
				// sort the current vertex' <bone id, weight> pairs descending by weight (so we can take the four most important ones)
				std::sort(vTempWeightsPerVertex[i].begin(), vTempWeightsPerVertex[i].end(), [](std::tuple<uint32_t, float> a, std::tuple<uint32_t, float> b) { return std::get<float>(a) > std::get<float>(b); });

				auto& indices = result.emplace_back(aBoneIndexOffset, aBoneIndexOffset, aBoneIndexOffset, aBoneIndexOffset);
				const auto numIndexWeightPairs = std::min(int{ 4 }, static_cast<int>(vTempWeightsPerVertex[i].size()));
				for (int j = 0; j < numIndexWeightPairs; ++j) {
					indices[j] = std::get<uint32_t>(vTempWeightsPerVertex[i][j]) + aBoneIndexOffset;
				}
			}
		}
		return result;
	}

	std::vector<glm::uvec4> model_t::bone_indices_for_meshes_for_single_target_buffer(const std::vector<mesh_index_t>& aMeshIndices, uint32_t aInitialBoneIndexOffset) const
	{
		std::vector<glm::uvec4> result;
		uint32_t offset = aInitialBoneIndexOffset;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = bone_indices_for_mesh(meshIndex, offset);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
			offset += num_bone_matrices(meshIndex);
		}
		return result;
	}

	std::vector<glm::uvec4> model_t::bone_indices_for_mesh_for_single_target_buffer(mesh_index_t aMeshIndex, const std::vector<mesh_index_t>& aMeshIndicesWithBonesInOrder) const
	{
		uint32_t offset = 0u;
		for (auto mi : aMeshIndicesWithBonesInOrder) {
			if (mi == aMeshIndex) {
				return bone_indices_for_mesh(mi, offset);
			}
			offset += num_bone_matrices(mi);
		}
		throw std::runtime_error("Invalid arguments to model_t::bone_indices_for_mesh_for_single_target_buffer: The list of aMeshIndicesWithBonesInOrder must contain the aMeshIndex.");
	}
	
	int model_t::num_uv_components_for_mesh(mesh_index_t aMeshIndex, int aSet) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		assert(aSet >= 0 && aSet < AI_MAX_NUMBER_OF_TEXTURECOORDS);
		if (nullptr == paiMesh->mTextureCoords[aSet]) { return 0; }
		return paiMesh->mNumUVComponents[aSet];
	}

	int model_t::number_of_indices_for_mesh(mesh_index_t aMeshIndex) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		size_t indicesCount = 0;
		for (unsigned int i = 0; i < paiMesh->mNumFaces; i++)
		{
			const aiFace& paiFace = paiMesh->mFaces[i];
			indicesCount += paiFace.mNumIndices;
		}
		return static_cast<int>(indicesCount);
	}

	std::vector<mesh_index_t> model_t::select_all_meshes() const
	{
		std::vector<mesh_index_t> result;
		auto n = mScene->mNumMeshes;
		result.reserve(n);
		for (decltype(n) i = 0; i < n; ++i) {
			result.push_back(static_cast<mesh_index_t>(i));
		}
		return result;
	}

	std::optional<glm::mat4> model_t::transformation_matrix_traverser_for_light(const aiLight* aLight, const aiNode* aNode, const aiMatrix4x4& aM) const
	{
		aiMatrix4x4 nodeM = aM * aNode->mTransformation;
		if (aNode->mName == aLight->mName) {
			return glm::transpose(glm::make_mat4(&nodeM.a1));
		}
		// Not found => go deeper
		for (unsigned int i = 0; i < aNode->mNumChildren; i++)
		{
			auto mat = transformation_matrix_traverser_for_light(aLight, aNode->mChildren[i], nodeM);
			if (mat.has_value()) {
				return mat;
			}
		}
		return {};
	}

	std::vector<glm::vec3> model_t::positions_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec3> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = positions_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::normals_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec3> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = normals_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::tangents_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec3> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = tangents_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::bitangents_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::vec3> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = bitangents_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec4> model_t::colors_for_meshes(std::vector<mesh_index_t> aMeshIndices, int aSet) const
	{
		std::vector<glm::vec4> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = colors_for_mesh(meshIndex, aSet);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<glm::vec4> model_t::bone_weights_for_meshes(std::vector<mesh_index_t> aMeshIndices, bool aNormalizeBoneWeights) const
	{
		std::vector<glm::vec4> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = bone_weights_for_mesh(meshIndex, aNormalizeBoneWeights);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}
	
	std::vector<glm::uvec4> model_t::bone_indices_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
	{
		std::vector<glm::uvec4> result;
		for (auto meshIndex : aMeshIndices) {
			auto tmp = bone_indices_for_mesh(meshIndex);
			std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
		}
		return result;
	}

	std::vector<lightsource> model_t::lights() const
	{
		std::vector<lightsource> result;
		auto n = mScene->mNumLights;
		result.reserve(n);
		for (decltype(n) i = 0; i < n; ++i) {
			const aiLight* aiLight = mScene->mLights[i];
			glm::mat4 transfo = transformation_matrix_traverser_for_light(aiLight, mScene->mRootNode, aiMatrix4x4()).value();
			glm::mat3 transfoForDirections = glm::mat3(glm::inverse(glm::transpose(transfo))); // TODO: inverse transpose okay for direction??
			lightsource light;
			light.mAngleInnerCone = aiLight->mAngleInnerCone;
			light.mAngleOuterCone = aiLight->mAngleOuterCone;
			light.mAttenuationConstant = aiLight->mAttenuationConstant;
			light.mAttenuationLinear = aiLight->mAttenuationLinear;
			light.mAttenuationQuadratic = aiLight->mAttenuationQuadratic;
			auto aiColor = aiLight->mColorAmbient;
			light.mColorAmbient = glm::vec3(aiColor.r, aiColor.g, aiColor.b);
			aiColor = aiLight->mColorDiffuse;
			light.mColor = glm::vec3(aiColor.r, aiColor.g, aiColor.b); // The "diffuse color" is considered to be the main color of this light source
			aiColor = aiLight->mColorSpecular;
			light.mColorSpecular = glm::vec3(aiColor.r, aiColor.g, aiColor.b);
			auto aiVec = aiLight->mDirection;
			light.mDirection = transfoForDirections * glm::vec3(aiVec.x, aiVec.y, aiVec.z);
			light.mName = std::string(aiLight->mName.C_Str());
			aiVec = aiLight->mPosition;
			light.mPosition = transfo * glm::vec4(aiVec.x, aiVec.y, aiVec.z, 1.0f);
			aiVec = aiLight->mUp;
			light.mUpVector = transfoForDirections * glm::vec3(aiVec.x, aiVec.y, aiVec.z);
			light.mAreaExtent = glm::vec2(aiLight->mSize.x, aiLight->mSize.y);
			switch (aiLight->mType) {
			case aiLightSource_DIRECTIONAL:
				light.mType = lightsource_type::directional;
				break;
			case aiLightSource_POINT:
				light.mType = lightsource_type::point;
				break;
			case aiLightSource_SPOT:
				light.mType = lightsource_type::spot;
				break;
			case aiLightSource_AMBIENT:
				light.mType = lightsource_type::ambient;
			case aiLightSource_AREA:
				light.mType = lightsource_type::area;
			default:
				light.mType = lightsource_type::reserved0;
			}
			result.push_back(light);
		}
		return result;
	}

	std::vector<avk::camera> model_t::cameras() const
	{
		std::vector<avk::camera> result;
		result.reserve(mScene->mNumCameras);
		for (unsigned int i = 0; i < mScene->mNumCameras; ++i) {
			aiCamera* aiCam = mScene->mCameras[i];
			auto cam = avk::camera();
			cam.set_aspect_ratio(aiCam->mAspect);
			cam.set_far_plane_distance(aiCam->mClipPlaneFar);
			cam.set_near_plane_distance(aiCam->mClipPlaneNear);
			cam.set_field_of_view(aiCam->mHorizontalFOV);
			cam.set_translation(glm::make_vec3(&aiCam->mPosition.x));
			glm::vec3 lookdir = glm::make_vec3(&aiCam->mLookAt.x);
			glm::vec3 updir = glm::make_vec3(&aiCam->mUp.x);
			cam.set_rotation(glm::quatLookAt(lookdir, updir));
			aiMatrix4x4 projMat;
			aiCam->GetCameraMatrix(projMat);
			cam.set_projection_matrix(glm::make_mat4(&projMat.a1));
			auto trafo = transformation_matrix_traverser_for_camera(aiCam, mScene->mRootNode, aiMatrix4x4());
			if (trafo.has_value()) {
				glm::vec3 side = glm::normalize(glm::cross(lookdir, updir));
				cam.set_translation(trafo.value() * glm::vec4(cam.translation(), 1));
				glm::mat3 dirtrafo = glm::mat3(glm::inverse(glm::transpose(trafo.value())));
				cam.set_rotation(glm::quatLookAt(dirtrafo * lookdir, dirtrafo * updir));
			}
			result.push_back(cam);
		}
		return result;
	}

	animation_clip_data model_t::load_animation_clip(unsigned int aAnimationIndex, double aStartTimeTicks, double aEndTimeTicks) const
	{
		assert(mScene);
		assert(aEndTimeTicks > aStartTimeTicks);
		assert(aStartTimeTicks >= 0.0);
		if (!mScene->HasAnimations()) {
			throw avk::runtime_error("Model has no animations");
		}
		if (aAnimationIndex > mScene->mNumAnimations) {
			throw avk::runtime_error("Requested animation index out of bounds");
		}

		double ticksPerSec = mScene->mAnimations[aAnimationIndex]->mTicksPerSecond;
		double durationTicks = mScene->mAnimations[aAnimationIndex]->mDuration;
		double endTicks = glm::min(aEndTimeTicks, durationTicks);

		return animation_clip_data{ aAnimationIndex, ticksPerSec, aStartTimeTicks, endTicks };
	}

	animation model_t::prepare_animation(uint32_t aAnimationIndex, const std::vector<mesh_index_t>& aMeshIndices)
	{
		animation result;

		std::unordered_map<mesh_index_t, uint32_t> boneIndexOffsetsPerMesh;
		{
			uint32_t bio = 0u;
			for (auto mi : aMeshIndices) {
				boneIndexOffsetsPerMesh[mi] = bio;
				bio += num_bone_matrices(mi);
			}
		}
		result.mAnimationIndex = aAnimationIndex;

		// --------------------------- helper collections ------------------------------------
		// Contains mappings of bone/node names to aiNode* pointers:
		std::unordered_map<std::string, aiNode*> mapNameToNode;
		add_all_to_node_map(mapNameToNode, mScene->mRootNode);

		// Which node is modified by bone animation? => Only those with an entry in this map:
		std::unordered_map<aiNode*, aiNodeAnim*> mapNodeToBoneAnimation;

		// Matrix information per bone per mesh:
		std::vector<std::unordered_map<aiNode*, bone_mesh_data>> mapsBoneToMatrixInfo;

		// Additional bone_mesh_data entries for mesh root nodes
		std::vector<std::vector<bone_mesh_data>> fakeBoneToMatrixInfos;

		// Which bones have been added per mesh. This is used to keep track of the bones added
		// and also serves to add the then un-added bones in their natural order.
		std::vector<std::vector<bool>> flagsBonesAddedAsAniNodes;

		// At which index has which node been inserted (relevant mostly for keeping track of parent-nodes):
		std::map<aiNode*, size_t> mapNodeToAniNodeIndex;
		// -----------------------------------------------------------------------------------

		// -------------------------------- helper lambdas -----------------------------------
		// Checks whether the given node is modified by bones a.k.a. bone-animated (by searching it in mapNodeToBoneAnimation)
		auto isNodeModifiedByBones = [&](aiNode* bNode) -> bool{
			const auto it = mapNodeToBoneAnimation.find(bNode);
			if (std::end(mapNodeToBoneAnimation) != it) {
				assert(it->first == bNode);
				return true;
			}
			return false;
		};

		// Helper lambda for checking whether a node has already been added and if so, returning its index
		auto isNodeAlreadyAdded = [&](aiNode* bNode) -> std::optional<size_t>{
			auto it = mapNodeToAniNodeIndex.find(bNode);
			if (std::end(mapNodeToAniNodeIndex) != it) {
				return it->second;
			}
			return {};
		};

		// Helper lambda for getting the 'next' parent node which is animated.
		// 'next' means: Next up the parent hierarchy WHICH IS BONE-ANIMATED.
		// If no such parent exists, an empty value will be returned.
		auto getAnimatedParentIndex = [&](aiNode* bNode) -> std::optional<size_t>{
			auto* parent = bNode->mParent;
			while (nullptr != parent) {
				auto already = isNodeAlreadyAdded(parent);
				if (already.has_value()) {
					assert(isNodeModifiedByBones(parent));
					return already;
				}
				else {
					assert(!isNodeModifiedByBones(parent));
				}
				parent = parent->mParent;
			}
			return {};
		};

		// Helper lambda for getting the accumulated parent transforms up the parent
		// hierarchy until a parent node is encountered which is bone-animated. That
		// bone-animated parent is NOT included in the accumulated transformation matrix.
		auto getUnanimatedParentTransform = [&](aiNode* bNode) -> glm::mat4{
			aiMatrix4x4 parentTransform{};
			auto* parent = bNode->mParent;
			while (nullptr != parent) {
				if (!isNodeModifiedByBones(parent)) {
					parentTransform = parent->mTransformation * parentTransform;
					parent = parent->mParent;
				}
				else {
					assert(isNodeAlreadyAdded(parent).has_value());
					parent = nullptr; // stop if the parent is animated
				}
			}
			return to_mat4(parentTransform);
		};

		// Helper-lambda to create an animated_node instance:
		auto addAnimatedNode = [&](aiNodeAnim* bChannel, aiNode* bNode, std::optional<size_t> bAnimatedParentIndex, const glm::mat4& bUnanimatedParentTransform){
			auto& anode = result.mAnimationData.emplace_back();
			mapNodeToAniNodeIndex[bNode] = result.mAnimationData.size() - 1;

			if (nullptr != bChannel) {
				for (unsigned int i = 0; i < bChannel->mNumPositionKeys; ++i) {
					anode.mPositionKeys.emplace_back(position_key{
						bChannel->mPositionKeys[i].mTime, to_vec3(bChannel->mPositionKeys[i].mValue)
					});
				}
				for (unsigned int i = 0; i < bChannel->mNumRotationKeys; ++i) {
					anode.mRotationKeys.emplace_back(rotation_key{
						bChannel->mRotationKeys[i].mTime, glm::normalize(to_quat(bChannel->mRotationKeys[i].mValue))	// normalize the quaternion, just to be on the safe side
					});
				}
				for (unsigned int i = 0; i < bChannel->mNumScalingKeys; ++i) {
					anode.mScalingKeys.emplace_back(scaling_key{
						bChannel->mScalingKeys[i].mTime, to_vec3(bChannel->mScalingKeys[i].mValue)
					});
				}
			}

			// Tidy-up the keys:
			//
			// There is one special case which will occur (probably often) in practice. That is, that there
			// are no keys at all (position + rotation + scaling == 0), because the animation does not modify a
			// given bone.
			// Such a case is created in the last step in this function (prepare_animation_for_meshes_into_strided_consecutive_storage),
			// which is looking for bones which have not been animated by Assimp's channels, but need to receive
			// a proper bone matrix.
			//
			// If it is not the special case, then assure that there ARE keys in each of the keys-collections,
			// that will (hopefully) make animation more performant because it requires fewer ifs.
			if (anode.mPositionKeys.size() + anode.mRotationKeys.size() + anode.mScalingKeys.size() > 0) {
				if (anode.mPositionKeys.empty()) {
					// The time doesn't really matter, but do not apply any translation
					anode.mPositionKeys.emplace_back(position_key{0.0, glm::vec3{0.f}});
				}
				if (anode.mRotationKeys.empty()) {
					// The time doesn't really matter, but do not apply any rotation
					anode.mRotationKeys.emplace_back(rotation_key{0.0, glm::quat(1.f, 0.f, 0.f, 0.f)});
				}
				if (anode.mScalingKeys.empty()) {
					// The time doesn't really matter, but do not apply any scaling
					anode.mScalingKeys.emplace_back(scaling_key{0.0, glm::vec3{1.f}});
				}
			}

			// Some lil' optimization flags:
			anode.mSameRotationAndPositionKeyTimes = have_same_key_times(anode.mPositionKeys, anode.mRotationKeys);
			anode.mSameScalingAndPositionKeyTimes = have_same_key_times(anode.mPositionKeys, anode.mScalingKeys);

			anode.mAnimatedParentIndex = bAnimatedParentIndex;
			anode.mParentTransform = bUnanimatedParentTransform;
			if (anode.mAnimatedParentIndex.has_value()) {
				assert(!(
					result.mAnimationData[anode.mAnimatedParentIndex.value()].mGlobalTransform[0][0] == 0.0f &&
					result.mAnimationData[anode.mAnimatedParentIndex.value()].mGlobalTransform[1][1] == 0.0f &&
					result.mAnimationData[anode.mAnimatedParentIndex.value()].mGlobalTransform[2][2] == 0.0f &&
					result.mAnimationData[anode.mAnimatedParentIndex.value()].mGlobalTransform[3][3] == 0.0f
				));
				anode.mGlobalTransform = result.mAnimationData[anode.mAnimatedParentIndex.value()].mGlobalTransform * anode.mParentTransform;
			}
			else {
				anode.mGlobalTransform = anode.mParentTransform;
			}

			anode.mLocalTransform = to_mat4(bNode->mTransformation);

			// See if we have an inverse bind pose matrix for this node:
			assert(nullptr == bChannel || mapNameToNode.find(to_string(bChannel->mNodeName))->second == bNode);
			for (size_t i = 0; i < mapsBoneToMatrixInfo.size(); ++i) {
				auto it = mapsBoneToMatrixInfo[i].find(bNode);
				if (std::end(mapsBoneToMatrixInfo[i]) != it) {
					anode.mBoneMeshTargets.push_back(it->second);
					flagsBonesAddedAsAniNodes[i][it->second.mMeshBoneInfo.mMeshLocalBoneIndex] = true;
				}
			}
			// Is this node, by chance, one of the mesh roots? 
			for (uint32_t x = 0u; x < bNode->mNumMeshes; ++x) {
				const auto mi = bNode->mMeshes[x];
				// find its index:
				assert (fakeBoneToMatrixInfos.size() == aMeshIndices.size());
				for (size_t i = 0; i < aMeshIndices.size(); ++i) {
					if (aMeshIndices[i] == mi) {
						for (auto& bmi : fakeBoneToMatrixInfos[i]) {
							anode.mBoneMeshTargets.push_back(bmi);
						}
						// We're done with these fakers:
						fakeBoneToMatrixInfos[i].clear();
						break;
					}
				}
			}
		};
		// -----------------------------------------------------------------------------------

		// Evaluate the data from the animation and fill mapNodeToBoneAnimation.
		assert(aAnimationIndex >= 0u && aAnimationIndex <= mScene->mNumAnimations);
		auto* ani = mScene->mAnimations[aAnimationIndex];
		for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
			auto* channel = ani->mChannels[i];

			auto it = mapNameToNode.find(to_string(channel->mNodeName));
			if (it == std::end(mapNameToNode)) {
				LOG_ERROR(std::format("Node name '{}', referenced from channel[{}], could not be found in the nodeMap.", to_string(channel->mNodeName), i));
				continue;
			}

			//if (channel->mNumPositionKeys + channel->mNumRotationKeys + channel->mNumScalingKeys > 0) {
			//requiredForAnimation.insert(it->second);
			mapNodeToBoneAnimation[it->second] = channel;
			//// Also mark all its parent nodes as required for animation (but not modified by bones!):
			//auto* parent = it->second->mParent;
			//while (nullptr != parent) {
			//	requiredForAnimation.insert(parent);
			//	parent = parent->mParent;
			//}
			//}
		}

		//// Checks whether the given node is required for this animation (by searching it in requiredForAnimation)
		//auto isNodeRequiredForAnimation = [&](aiNode* bNode) {
		//	const auto it = requiredForAnimation.find(bNode);
		//	if (std::end(requiredForAnimation) != it) {
		//		assert( *it == bNode );
		//		return true;
		//	}
		//	return false;
		//};

		for (size_t i = 0; i < aMeshIndices.size(); ++i) {
			auto& bmi = mapsBoneToMatrixInfo.emplace_back();
			auto& fkb = fakeBoneToMatrixInfos.emplace_back();
			auto mi = aMeshIndices[i];

			glm::mat4 inverseMeshRootMatrix = glm::inverse(transformation_matrix_for_mesh(mi));

			assert(mi >= 0u && mi < mScene->mNumMeshes);
			flagsBonesAddedAsAniNodes.emplace_back(static_cast<size_t>(num_bone_matrices(mi)), false); // Note: num_bone_matrices(mi) here, but num_actual_bones(mi) down there in the loop!
			// Vector with a flag for each bone

			// For each bone, create boneMatrixInfo:
			const auto nb = num_bone_matrices(mi);
			for (uint32_t bi = 0; bi < nb; ++bi) {
				if (bi < mScene->mMeshes[mi]->mNumBones) {
					auto* bone = mScene->mMeshes[mi]->mBones[bi];

					auto it = mapNameToNode.find(to_string(bone->mName));
					if (it == std::end(mapNameToNode)) {
						LOG_ERROR(std::format("Bone named '{}' could not be found in the nodeMap.", to_string(bone->mName)));
						continue;
					}

					assert(!bmi.contains(it->second));
					bmi[it->second] = bone_mesh_data{
						to_mat4(bone->mOffsetMatrix),
						inverseMeshRootMatrix,
						mesh_bone_info{i, mi, bi, boneIndexOffsetsPerMesh[mi]}
					};
				}
				else {
					fkb.emplace_back(bone_mesh_data{
						glm::mat4{1.0f}, // Offset/inverse bind pose matrix should be the identity, because there is nothing to transform here.
						inverseMeshRootMatrix,
						mesh_bone_info{i, mi, bi, boneIndexOffsetsPerMesh[mi]}
					});
				}
			}
		}

		// ---------------------------------------------
		// AND NOW: Construct the animated_nodes "tree"
#ifdef _DEBUG
		{
			std::vector<aiNode*> sanityCheck;
			for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
				auto* channel = ani->mChannels[i];
				auto it = mapNameToNode.find(to_string(channel->mNodeName));
				if (it == std::end(mapNameToNode)) {
					sanityCheck.push_back(it->second);
				}
			}
			std::sort(std::begin(sanityCheck), std::end(sanityCheck));
			auto uniqueEnd = std::unique(std::begin(sanityCheck), std::end(sanityCheck));
			if (uniqueEnd != std::end(sanityCheck)) {
				LOG_WARNING(
					std::format(
						"Some nodes are contained multiple times in the animation channels of animation[{}]. Don't know if that's going to lead to correct results."
						, aAnimationIndex));
			}
		}
#endif

		// Let's go:
		for (unsigned int i = 0; i < ani->mNumChannels; ++i) {
			auto* channel = ani->mChannels[i];

			auto it = mapNameToNode.find(to_string(channel->mNodeName));
			if (it == std::end(mapNameToNode)) {
				LOG_ERROR(std::format("Node name '{}', referenced from channel[{}], could not be found in the nodeMap.", to_string(channel->mNodeName), i));
				continue;
			}

			auto* node = it->second;
			std::stack<aiNode*> boneAnimatedParents;
			auto* parent = node->mParent;
			while (nullptr != parent) {
				if (isNodeModifiedByBones(parent) && !isNodeAlreadyAdded(parent).has_value()) {
					boneAnimatedParents.push(parent);
					LOG_DEBUG(std::format("Interesting: Node '{}' in parent-hierarchy of node '{}' is also bone-animated, but not encountered them while iterating through channels yet.", parent->mName.C_Str(), node->mName.C_Str()));
				}
				parent = parent->mParent;
			}

			// First, add the stack of parents, then add the node itself
			while (!boneAnimatedParents.empty()) {
				auto parentToBeAdded = boneAnimatedParents.top();
				assert(mapNodeToBoneAnimation.contains(parentToBeAdded));
				addAnimatedNode(mapNodeToBoneAnimation[parentToBeAdded], parentToBeAdded, getAnimatedParentIndex(parentToBeAdded), getUnanimatedParentTransform(parentToBeAdded));
				boneAnimatedParents.pop();
			}
			if (!isNodeAlreadyAdded(node)) { // <-- Mostly relevant for the cases where parent nodes have already been added (see while-loop right above) and should not be added again.
				addAnimatedNode(mapNodeToBoneAnimation[node], node, getAnimatedParentIndex(node), getUnanimatedParentTransform(node));
			}
		}

		// It could be that there are still bones for which we have not set up an animated_node entry and hence,
		// no bone matrix will be written for them.
		// This happened for all bones which are not affected by the given animation. We must write a bone matrix
		// for them as well => Find them and add them as animated_node entry (but without any position/rotation/scaling keys).
		assert(flagsBonesAddedAsAniNodes.size() == aMeshIndices.size());
		for (size_t i = 0; i < aMeshIndices.size(); ++i) {
			const auto mi = aMeshIndices[i];

			// Set the bone matrices that are not affected by animation ONCE/NOW:
			const auto nb = num_bone_matrices(mi); // => i.e. bone MATRICES, not just bones!
			assert(flagsBonesAddedAsAniNodes[i].size() == nb);
			for (uint32_t bi = 0; bi < nb; ++bi) {
				if (flagsBonesAddedAsAniNodes[i][bi]) {
					continue;
				}

				if (bi < mScene->mMeshes[mi]->mNumBones) {
					auto* bone = mScene->mMeshes[mi]->mBones[bi];
					auto it = mapNameToNode.find(to_string(bone->mName));
					assert(std::end(mapNameToNode) != it);

					addAnimatedNode(
						nullptr, // <-- This is fine. This node is just not affected by animation but still needs to receive bone matrix updates
						it->second, getAnimatedParentIndex(it->second), getUnanimatedParentTransform(it->second)
					);
				}
				else {
					auto* meshRootNode = find_mesh_root_node(static_cast<unsigned int>(mi));
					assert(nullptr != meshRootNode);
					addAnimatedNode(
						nullptr, // <-- This is fine. This node is just not affected by animation but still needs to receive bone matrix updates
						meshRootNode, getAnimatedParentIndex(meshRootNode), getUnanimatedParentTransform(meshRootNode)
					);
				}
			}
		}

		return result;
	}

	std::optional<glm::mat4> model_t::transformation_matrix_traverser_for_camera(const aiCamera* aCamera, const aiNode* aNode, const aiMatrix4x4& aM) const
	{
		aiMatrix4x4 nodeM = aM * aNode->mTransformation;
		if (aNode->mName == aCamera->mName) {
			return glm::transpose(glm::make_mat4(&nodeM.a1));
		}
		// Not found => go deeper
		for (unsigned int i = 0; i < aNode->mNumChildren; i++)
		{
			auto mat = transformation_matrix_traverser_for_camera(aCamera, aNode->mChildren[i], nodeM);
			if (mat.has_value()) {
				return mat;
			}
		}
		return {};
	}

	void model_t::add_all_to_node_map(std::unordered_map<std::string, aiNode*>& aNodeMap, aiNode* aNode)
	{
		aNodeMap[to_string(aNode->mName)] = aNode;
		for (unsigned int i = 0; i < aNode->mNumChildren; ++i) {
			add_all_to_node_map(aNodeMap, aNode->mChildren[i]);
		}
	}

	void model_t::calculate_tangent_space_for_mesh(mesh_index_t aMeshIndex, uint32_t aConfigSourceUV)
	{
		mTangentsAndBitangents[aMeshIndex] = std::make_tuple(std::vector<glm::vec3>{}, std::vector<glm::vec3>{});
		auto& [meshTang, meshBitang] = mTangentsAndBitangents[aMeshIndex];
		auto numVerts = number_of_vertices_for_mesh(aMeshIndex);
		// create space for the tangents and bitangents
		meshTang.resize(numVerts);
		meshBitang.resize(numVerts);

		// The following is mostly a copy&paste from ASSIMP's code (but not necessarily from the latest version,
		// since at some point, incorrect tangents/bitangents calculation has been introduced, namely s.t.
		// the bitangents would always be orthogonal to the tangents. This is wrong.)

		const aiMesh* pMesh = mScene->mMeshes[aMeshIndex];

		// If the mesh consists of lines and/or points but not of
		// triangles or higher-order polygons the normal vectors
		// are undefined.
		if (!(pMesh->mPrimitiveTypes & (aiPrimitiveType_TRIANGLE | aiPrimitiveType_POLYGON))) {
		    LOG_INFO("Tangents are undefined for line and point meshes");
		    return;
		}

		// what we can check, though, is if the mesh has normals and texture coordinates. That's a requirement
		if (pMesh->mNormals == nullptr) {
		    LOG_ERROR("Failed to compute tangents; need normals");
		    return;
		}
		if (aConfigSourceUV >= AI_MAX_NUMBER_OF_TEXTURECOORDS || !pMesh->mTextureCoords[aConfigSourceUV]) {
		    LOG_ERROR(std::format("Failed to compute tangents; need UV data in channel[{}]", aConfigSourceUV));
		    return;
		}

		const float angleEpsilon = 0.9999f;

		std::vector<bool> vertexDone(pMesh->mNumVertices, false);
		const float qnan = std::numeric_limits<ai_real>::quiet_NaN();

		const aiVector3D *meshPos = pMesh->mVertices;
		const aiVector3D *meshNorm = pMesh->mNormals;
		const aiVector3D *meshTex = pMesh->mTextureCoords[aConfigSourceUV];

		// calculate the tangent and bitangent for every face
		for (unsigned int a = 0; a < pMesh->mNumFaces; a++) {
		    const aiFace &face = pMesh->mFaces[a];
		    if (face.mNumIndices < 3) {
		        // There are less than three indices, thus the tangent vector
		        // is not defined. We are finished with these vertices now,
		        // their tangent vectors are set to qnan.
		        for (unsigned int i = 0; i < face.mNumIndices; ++i) {
		            unsigned int idx = face.mIndices[i];
		            vertexDone[idx] = true;
		            meshTang[idx] = to_vec3(aiVector3D(qnan));
		            meshBitang[idx] = to_vec3(aiVector3D(qnan));
		        }

		        continue;
		    }

		    // triangle or polygon... we always use only the first three indices. A polygon
		    // is supposed to be planar anyways....
		    // FIXME: (thom) create correct calculation for multi-vertex polygons maybe?
		    const unsigned int p0 = face.mIndices[0], p1 = face.mIndices[1], p2 = face.mIndices[2];

		    // position differences p1->p2 and p1->p3
		    aiVector3D v = meshPos[p1] - meshPos[p0], w = meshPos[p2] - meshPos[p0];

		    // texture offset p1->p2 and p1->p3
		    float sx = meshTex[p1].x - meshTex[p0].x, sy = meshTex[p1].y - meshTex[p0].y;
		    float tx = meshTex[p2].x - meshTex[p0].x, ty = meshTex[p2].y - meshTex[p0].y;
		    float dirCorrection = (tx * sy - ty * sx) < 0.0f ? -1.0f : 1.0f;
		    // when t1, t2, t3 in same position in UV space, just use default UV direction.
		    if (sx * ty == sy * tx) {
		        sx = 0.0;
		        sy = 1.0;
		        tx = 1.0;
		        ty = 0.0;
		    }

		    // tangent points in the direction where to positive X axis of the texture coord's would point in model space
		    // bitangent's points along the positive Y axis of the texture coord's, respectively
		    aiVector3D tangent, bitangent;
		    tangent.x = (w.x * sy - v.x * ty) * dirCorrection;
		    tangent.y = (w.y * sy - v.y * ty) * dirCorrection;
		    tangent.z = (w.z * sy - v.z * ty) * dirCorrection;
		    bitangent.x = (w.x * sx - v.x * tx) * dirCorrection;
		    bitangent.y = (w.y * sx - v.y * tx) * dirCorrection;
		    bitangent.z = (w.z * sx - v.z * tx) * dirCorrection;

		    // store for every vertex of that face
		    for (unsigned int b = 0; b < face.mNumIndices; ++b) {
		        unsigned int p = face.mIndices[b];

		        // project tangent and bitangent into the plane formed by the vertex' normal
		        aiVector3D localTangent = tangent - meshNorm[p] * (tangent * meshNorm[p]);
		        aiVector3D localBitangent = bitangent - meshNorm[p] * (bitangent * meshNorm[p]);
		        localTangent.NormalizeSafe();
		        localBitangent.NormalizeSafe();

		        // reconstruct tangent/bitangent according to normal and bitangent/tangent when it's infinite or NaN.
		        bool invalid_tangent = is_special_float(localTangent.x) || is_special_float(localTangent.y) || is_special_float(localTangent.z);
		        bool invalid_bitangent = is_special_float(localBitangent.x) || is_special_float(localBitangent.y) || is_special_float(localBitangent.z);
		        if (invalid_tangent != invalid_bitangent) {
		            if (invalid_tangent) {
		                localTangent = meshNorm[p] ^ localBitangent;
		                localTangent.NormalizeSafe();
		            } else {
		                localBitangent = localTangent ^ meshNorm[p];
		                localBitangent.NormalizeSafe();
		            }
		        }

		        // and write it into the mesh.
		        meshTang[p] = to_vec3(localTangent);
		        meshBitang[p] = to_vec3(localBitangent);
		    }
		}
	}

	void model_t::calculate_tangent_space_for_all_meshes(uint32_t aConfigSourceUV)
	{
		auto n = mScene->mNumMeshes;
		for (decltype(n) i = 0; i < n; ++i) {
			calculate_tangent_space_for_mesh(i, aConfigSourceUV);
		}
	}
}

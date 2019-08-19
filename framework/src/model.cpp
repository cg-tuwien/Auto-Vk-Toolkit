#include <cg_base.hpp>

#include <sstream>

namespace cgb
{
	owning_resource<model_t> model_t::load_from_file(const std::string& _Path, aiProcessFlagsType _AssimpFlags)
	{
		model_t result;
		result.mImporter = std::make_unique<Assimp::Importer>();
		result.mScene = result.mImporter->ReadFile(_Path, _AssimpFlags);
		if (nullptr == result.mScene) {
			throw std::runtime_error("Loading model from file failed.");
		}
		return result;
	}

	owning_resource<model_t> model_t::load_from_memory(const std::string& _Memory, aiProcessFlagsType _AssimpFlags)
	{
		model_t result;
		result.mImporter = std::make_unique<Assimp::Importer>();
		result.mScene = result.mImporter->ReadFileFromMemory(_Memory.c_str(), _Memory.size(), _AssimpFlags);
		if (nullptr == result.mScene) {
			throw std::runtime_error("Loading model from memory failed.");
		}
		return result;
	}

	std::optional<glm::mat4> model_t::transformation_matrix_tranverser(const unsigned int _MeshIndexToFind, const aiNode* _Node, const aiMatrix4x4& _M)
	{
		aiMatrix4x4 nodeM = _M * _Node->mTransformation;
		for (unsigned int i = 0; i < _Node->mNumMeshes; i++)
		{
			if (_Node->mMeshes[i] == _MeshIndexToFind) {
				return glm::transpose(glm::make_mat4(&nodeM.a1));
			}
		}
		// Not found => go deeper
		for (unsigned int i = 0; i < _Node->mNumChildren; i++)
		{
			auto mat = transformation_matrix_tranverser(_MeshIndexToFind, _Node->mChildren[i], nodeM);
			if (mat.has_value()) {
				return mat;
			}
		}
		return {};
	}

	glm::mat4 model_t::transformation_matrix_for_mesh(size_t _MeshIndex)
	{
		// Find the mesh in Assim's node hierarchy
		return transformation_matrix_tranverser(static_cast<unsigned int>(_MeshIndex), mScene->mRootNode, aiMatrix4x4{}).value();
	}

	std::string model_t::name_of_mesh(size_t _MeshIndex)
	{
		assert(mScene->mNumMeshes >= _MeshIndex);
		return mScene->mMeshes[_MeshIndex]->mName.data;
	}

	size_t model_t::material_index_for_mesh(size_t _MeshIndex)
	{
		assert(mScene->mNumMeshes >= _MeshIndex);
		return mScene->mMeshes[_MeshIndex]->mMaterialIndex;
	}

	std::string model_t::name_of_material(size_t _MaterialIndex)
	{
		aiMaterial* pMaterial = mScene->mMaterials[_MaterialIndex];
		if (!pMaterial) return "";
		aiString name;
		if (pMaterial->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
			return name.data;
		else
			return "";
	}

	material model_t::material_of_mesh(size_t _MeshIndex)
	{
		material result;

		aiString strVal;
		aiColor3D color(0.0f, 0.0f, 0.0f);
		int intVal;
		aiBlendMode blendMode;
		float floatVal;
		aiTextureMapping texMapping;
		aiTextureMapMode texMapMode;

		auto materialIndex = material_index_for_mesh(_MeshIndex);
		assert(materialIndex <= mScene->mNumMaterials);
		aiMaterial* aimat = mScene->mMaterials[materialIndex];

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_NAME, strVal)) {
			result.m_name = strVal.data;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
			result.m_diffuse_reflectivity = glm::vec3(color.r, color.g, color.b);
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
			result.m_specular_reflectivity = glm::vec3(color.r, color.g, color.b);
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_AMBIENT, color)) {
			result.m_ambient_reflectivity = glm::vec3(color.r, color.g, color.b);
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_COLOR_EMISSIVE, color)) {
			result.m_emissive_color = glm::vec3(color.r, color.g, color.b);
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_ENABLE_WIREFRAME, intVal)) {
			result.m_wireframe_mode = 0 != intVal;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_TWOSIDED, intVal)) {
			result.m_twosided = 0 != intVal;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_BLEND_FUNC, blendMode)) {
			result.m_blend_mode = blendMode == aiBlendMode_Additive
				? cfg::color_blending_config::enable_additive_for_all_attachments()
				: cfg::color_blending_config::enable_alpha_blending_for_all_attachments();
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_OPACITY, floatVal)) {
			result.m_opacity = floatVal;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHININESS, floatVal)) {
			result.m_shininess = floatVal;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_SHININESS_STRENGTH, floatVal)) {
			result.m_shininess_strength = floatVal;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_REFRACTI, floatVal)) {
			result.m_refraction_index = floatVal;
		}

		if (AI_SUCCESS == aimat->Get(AI_MATKEY_REFLECTIVITY, floatVal)) {
			result.m_reflectivity = floatVal;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DIFFUSE, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_diffuse_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_SPECULAR, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_specular_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_AMBIENT, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_ambient_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_EMISSIVE, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_emissive_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_HEIGHT, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_height_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_NORMALS, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_normals_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_SHININESS, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_shininess_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_OPACITY, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_opacity_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_DISPLACEMENT, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_displacement_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_REFLECTION, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_reflection_tex = strVal.data;
		}

		if (AI_SUCCESS == aimat->GetTexture(aiTextureType_LIGHTMAP, 0, &strVal, &texMapping, nullptr, nullptr, nullptr, &texMapMode)) {
			if (texMapping != aiTextureMapping_UV) {
				assert(false);
			}
			result.m_lightmap_tex = strVal.data;
		}

		return result;
	}

	std::vector<glm::vec3> model_t::positions_for_mesh(size_t _MeshIndex)
	{
		const aiMesh* paiMesh = mScene->mMeshes[_MeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		for (decltype(n) i = 0; i < n; ++i) {
			result.push_back(glm::vec3(paiMesh->mVertices[i][0], paiMesh->mVertices[i][1], paiMesh->mVertices[i][2]));
		}
		return result;
	}

	std::vector<glm::vec3> model_t::normals_for_mesh(size_t _MeshIndex)
	{
		const aiMesh* paiMesh = mScene->mMeshes[_MeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		for (decltype(n) i = 0; i < n; ++i) {
			result.push_back(glm::vec3(paiMesh->mNormals[i][0], paiMesh->mNormals[i][1], paiMesh->mNormals[i][2]));
		}
		return result;
	}

	std::vector<uint32_t> model_t::indices_for_mesh(size_t _MeshIndex)
	{
		const aiMesh* paiMesh = mScene->mMeshes[_MeshIndex];
		size_t indicesCount = paiMesh->mNumFaces * 3; // There are always 3 faces... TODO: Is that true?
		std::vector<uint32_t> result;
		result.reserve(indicesCount);
		for (unsigned int i = 0; i < paiMesh->mNumFaces; i++)
		{
			// we're working with triangulated meshes only
			const aiFace& Face = paiMesh->mFaces[i];
			result.push_back(Face.mIndices[0]);
			result.push_back(Face.mIndices[1]);
			result.push_back(Face.mIndices[2]);
		}
		return result;
	}
}

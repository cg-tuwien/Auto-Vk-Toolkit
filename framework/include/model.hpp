#pragma once

namespace cgb
{
	struct material
	{
		std::string m_name;
		glm::vec3 m_diffuse_reflectivity;
		glm::vec3 m_specular_reflectivity;
		glm::vec3 m_ambient_reflectivity;
		glm::vec3 m_emissive_color;
		glm::vec3 m_transparent_color;
		bool m_wireframe_mode;
		bool m_twosided;
		cfg::color_blending_config m_blend_mode;
		float m_opacity;
		float m_shininess;
		float m_shininess_strength;
		float m_refraction_index;
		float m_reflectivity;
		std::string m_diffuse_tex;
		std::string m_specular_tex;
		std::string m_ambient_tex;
		std::string m_emissive_tex;
		std::string m_height_tex;
		std::string m_normals_tex;
		std::string m_shininess_tex;
		std::string m_opacity_tex;
		std::string m_displacement_tex;
		std::string m_reflection_tex;
		std::string m_lightmap_tex;
		glm::vec3 m_albedo;
		float m_metallic;
		float m_smoothness;
		float m_sheen;
		float m_thickness;
		float m_roughness;
		float m_anisotropy;
		glm::vec3 m_anisotropy_rotation;
		glm::vec2 m_offset;
		glm::vec2 m_tiling;
	};

	class model_t
	{
	public:
		using aiProcessFlagsType = unsigned int;

		model_t() = default;
		model_t(const model_t&) = delete;
		model_t(model_t&&) = default;
		model_t& operator=(const model_t&) = delete;
		model_t& operator=(model_t&&) = default;
		~model_t() = default;

		glm::mat4 transformation_matrix_for_mesh(size_t _MeshIndex);
		std::string name_of_mesh(size_t _MeshIndex);
		size_t material_index_for_mesh(size_t _MeshIndex);
		std::string name_of_material(size_t _MaterialIndex);
		material material_of_mesh(size_t _MeshIndex);

		std::vector<glm::vec3> positions_for_mesh(size_t _MeshIndex);
		std::vector<glm::vec3> normals_for_mesh(size_t _MeshIndex);
		std::vector<glm::vec3> tangents_for_mesh(size_t _MeshIndex);
		std::vector<glm::vec3> bitangents_for_mesh(size_t _MeshIndex);
		std::vector<glm::vec4> colors_for_mesh(size_t _MeshIndex, int _Set = 0);
		int num_uv_components_for_mesh(size_t _MeshIndex, int _Set = 0);
		template <typename T> std::vector<T> texture_coordinates_for_mesh(size_t _MeshIndex, int _Set = 0) { static_assert(false, "unsupported type T"); }
		int num_indices_for_mesh(size_t _MeshIndex);
		
		template <typename T> 
		std::vector<T> indices_for_mesh(size_t _MeshIndex) 
		{ 
			const aiMesh* paiMesh = mScene->mMeshes[_MeshIndex];
			size_t indicesCount = num_indices_for_mesh(_MeshIndex);
			std::vector<T> result;
			result.reserve(indicesCount);
			for (unsigned int i = 0; i < paiMesh->mNumFaces; ++i) {
				// we're working with triangulated meshes only
				const aiFace& paiFace = paiMesh->mFaces[i];
				for (unsigned int f = 0; f < paiFace.mNumIndices; ++f) {
					result.emplace_back(static_cast<T>(paiFace.mIndices[f]));
				}
			}
			return result;
		}

		/** Return the indices of all meshes which the given _Predicate evaluates true for.
		 *	Function-signature: bool(size_t, const aiMesh*) where the first parameter is the 
		 *									mesh index and the second the pointer to the data
		 */
		template <typename F>
		std::vector<size_t> select_meshes(F _Predicate)
		{
			std::vector<size_t> result;
			for (size_t i = 0; i < mScene->mNumMeshes; ++i) {
				const aiMesh* paiMesh = mScene->mMeshes[i];
				if (_Predicate(i, paiMesh)) {
					result.push_back(i);
				}
			}
			return result;
		}

		std::vector<size_t> select_all_meshes();

		std::vector<glm::vec3> positions_for_meshes(std::vector<size_t> _MeshIndices);
		std::vector<glm::vec3> normals_for_meshes(std::vector<size_t> _MeshIndices);
		std::vector<glm::vec3> tangents_for_meshes(std::vector<size_t> _MeshIndices);
		std::vector<glm::vec3> bitangents_for_meshes(std::vector<size_t> _MeshIndices);
		std::vector<glm::vec4> colors_for_meshes(std::vector<size_t> _MeshIndices, int _Set = 0);

		template <typename T>
		std::vector<T> texture_coordinates_for_meshes(std::vector<size_t> _MeshIndices, int _Set = 0)
		{
			std::vector<T> result;
			for (auto meshIndex : _MeshIndices) {
				auto tmp = texture_coordinates_for_mesh<T>(meshIndex, _Set);
				std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
			}
			return result;
		}

		template <typename T>
		std::vector<T> indices_for_meshes(std::vector<size_t> _MeshIndices)
		{
			std::vector<T> result;
			for (auto meshIndex : _MeshIndices) {
				auto tmp = indices_for_mesh<T>(meshIndex);
				std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
			}
			return result;
		}

		static owning_resource<model_t> load_from_file(const std::string& _Path, aiProcessFlagsType _AssimpFlags = 0);
		static owning_resource<model_t> load_from_memory(const std::string& _Memory, aiProcessFlagsType _AssimpFlags = 0);

	private:
		std::optional<glm::mat4> transformation_matrix_traverser(const unsigned int _MeshIndexToFind, const aiNode* _Node, const aiMatrix4x4& _M);

		std::unique_ptr<Assimp::Importer> mImporter;
		const aiScene* mScene;
	};

	using model = owning_resource<model_t>;


	template <>
	inline std::vector<glm::vec2> model_t::texture_coordinates_for_mesh<glm::vec2>(size_t _MeshIndex, int _Set)
	{
		const aiMesh* paiMesh = mScene->mMeshes[_MeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec2> result;
		result.reserve(n);
		assert(_Set >= 0 && _Set < AI_MAX_NUMBER_OF_TEXTURECOORDS);
		if (nullptr == paiMesh->mTextureCoords[_Set]) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain a texture coordinates at index {}. Will return (0,0) for each vertex.", _MeshIndex, _Set));
			result.emplace_back(0.f, 0.f);
		}
		else {
			const auto nuv = num_uv_components_for_mesh(_MeshIndex, _Set);
			switch (nuv) {
			case 1:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[i][_Set][0], 0.f);
				}
				break;
			case 2:
			case 3:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[i][_Set][0], paiMesh->mTextureCoords[i][_Set][1]);
				}
				break;
			default:
				throw std::logic_error(fmt::format("Can't handle a number of {} uv components for mesh at index {}, set {}.", nuv, _MeshIndex, _Set));
			}
		}
		return result;
	}

	template <>
	inline std::vector<glm::vec3> model_t::texture_coordinates_for_mesh<glm::vec3>(size_t _MeshIndex, int _Set)
	{
		const aiMesh* paiMesh = mScene->mMeshes[_MeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		assert(_Set >= 0 && _Set < AI_MAX_NUMBER_OF_TEXTURECOORDS);
		if (nullptr == paiMesh->mTextureCoords[_Set]) {
			LOG_WARNING(fmt::format("The mesh at index {} does not contain a texture coordinates at index {}. Will return (0,0,0) for each vertex.", _MeshIndex, _Set));
			result.emplace_back(0.f, 0.f, 0.f);
		}
		else {
			const auto nuv = num_uv_components_for_mesh(_MeshIndex, _Set);
			switch (nuv) {
			case 1:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[i][_Set][0], 0.f, 0.f);
				}
				break;
			case 2:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[i][_Set][0], paiMesh->mTextureCoords[i][_Set][1], 0.f);
				}
				break;
			case 3:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[i][_Set][0], paiMesh->mTextureCoords[i][_Set][1], paiMesh->mTextureCoords[i][_Set][2]);
				}
				break;
			default:
				throw std::logic_error(fmt::format("Can't handle a number of {} uv components for mesh at index {}, set {}.", nuv, _MeshIndex, _Set));
			}
		}
		return result;
	}

}

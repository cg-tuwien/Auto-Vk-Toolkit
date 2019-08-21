#pragma once

namespace cgb
{
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

		/** Determine the transformation matrix for the mesh at the given index.
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		Transformation matrix of the given mesh, can be the identity
		 */
		glm::mat4 transformation_matrix_for_mesh(size_t _MeshIndex) const;

		/** Gets the name of the mesh at the given index (not to be confused with the material's name)
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		Mesh name converted from Assimp's internal representation to std::string
		 */
		std::string name_of_mesh(size_t _MeshIndex) const;

		/** Gets Assimp's internal material index for the given mesh index. 
		 *	This value won't be useful if not operating directly on Assimp's internal materials.
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		Mesh index corresponding to Assimp's internal materials structure.
		 */
		size_t material_index_for_mesh(size_t _MeshIndex) const;

		/** Gets the name of material at the given material index
		 *	@param		_MaterialIndex		The index corresponding to the material
		 *	@return		Material name converted from Assimp's internal representation to std::string
		 */
		std::string name_of_material(size_t _MaterialIndex) const;

		/** Gets the `material_config` struct for the mesh at the given index.
		 *	The `material_config` struct is created from Assimp's internal material data.
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		`material_config` struct, representing the "type of material". 
		 *				To actually load all the resources it refers to, you'll have 
		 *				to create a `material` based on it.
		 */
		material_config material_config_for_mesh(size_t _MeshIndex) const;

		/** Gets the number of vertices for the mesh at the given index.
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		Number of vertices, which is also the length of all the vectors,
		 *				which are returned by: `positions_for_mesh`, `normals_for_mesh`,
		 *				`tangents_for_mesh`, `bitangents_for_mesh`, `colors_for_mesh`, 
		 *				and `texture_coordinates_for_mesh`
		 */
		inline size_t number_of_vertices_for_mesh(size_t _MeshIndex) const;

		/** Gets all the positions for a given mesh index
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		Vector  of vertex positions, converted to `glm::vec3`
		 */
		std::vector<glm::vec3> positions_for_mesh(size_t _MeshIndex) const;

		/** Gets all the normals for a given mesh index.
		 *	If the mesh does not have normals, 
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		Normals Normals converted to `glm::vec3`
		 */
		std::vector<glm::vec3> normals_for_mesh(size_t _MeshIndex) const;

		/** Gets all the tangents for a given mesh index
		 *	@param		_MeshIndex		The index corresponding to the mesh
		 *	@return		Normals converted to `glm::vec3`
		 */
		std::vector<glm::vec3> tangents_for_mesh(size_t _MeshIndex) const;
		std::vector<glm::vec3> bitangents_for_mesh(size_t _MeshIndex) const;
		std::vector<glm::vec4> colors_for_mesh(size_t _MeshIndex, int _Set = 0) const;
		int num_uv_components_for_mesh(size_t _MeshIndex, int _Set = 0) const;

		template <typename T> std::vector<T> texture_coordinates_for_mesh(size_t _MeshIndex, int _Set = 0) const
		{
			throw std::logic_error(fmt::format("unsupported type {}", typeid(T).name()));
		}
		
		int num_indices_for_mesh(size_t _MeshIndex) const;
		
		template <typename T> 
		std::vector<T> indices_for_mesh(size_t _MeshIndex) const
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
		std::vector<size_t> select_meshes(F _Predicate) const
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

		std::vector<size_t> select_all_meshes() const;

		std::vector<glm::vec3> positions_for_meshes(std::vector<size_t> _MeshIndices) const;
		std::vector<glm::vec3> normals_for_meshes(std::vector<size_t> _MeshIndices) const;
		std::vector<glm::vec3> tangents_for_meshes(std::vector<size_t> _MeshIndices) const;
		std::vector<glm::vec3> bitangents_for_meshes(std::vector<size_t> _MeshIndices) const;
		std::vector<glm::vec4> colors_for_meshes(std::vector<size_t> _MeshIndices, int _Set = 0) const;

		template <typename T>
		std::vector<T> texture_coordinates_for_meshes(std::vector<size_t> _MeshIndices, int _Set = 0) const
		{
			std::vector<T> result;
			for (auto meshIndex : _MeshIndices) {
				auto tmp = texture_coordinates_for_mesh<T>(meshIndex, _Set);
				std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
			}
			return result;
		}

		template <typename T>
		std::vector<T> indices_for_meshes(std::vector<size_t> _MeshIndices) const
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
		std::optional<glm::mat4> transformation_matrix_traverser(const unsigned int _MeshIndexToFind, const aiNode* _Node, const aiMatrix4x4& _M) const;

		std::unique_ptr<Assimp::Importer> mImporter;
		const aiScene* mScene;
	};

	using model = owning_resource<model_t>;


	template <>
	inline std::vector<glm::vec2> model_t::texture_coordinates_for_mesh<glm::vec2>(size_t _MeshIndex, int _Set) const
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
	inline std::vector<glm::vec3> model_t::texture_coordinates_for_mesh<glm::vec3>(size_t _MeshIndex, int _Set) const
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

#pragma once

#include "animation.hpp"
#include "camera.hpp"
#include "lightsource.hpp"
#include "material_config.hpp"

namespace avk
{
	class model_t
	{
		friend class context_vulkan;

	public:
		using aiProcessFlagsType = unsigned int;

		model_t() = default;
		model_t(model_t&&) noexcept = default;
		model_t(const model_t&) = delete;
		model_t& operator=(model_t&&) noexcept = default;
		model_t& operator=(const model_t&) = delete;
		~model_t() = default;

		const auto* handle() const { return mScene; }

		static avk::owning_resource<model_t> load_from_file(const std::string& aPath, aiProcessFlagsType aAssimpFlags = aiProcess_Triangulate);
		
		static avk::owning_resource<model_t> load_from_memory(const std::string& aMemory, aiProcessFlagsType aAssimpFlags = aiProcess_Triangulate);

		/** Returns this model's path where it has been loaded from */
		auto path() const { return mModelPath; }

		/** Determine the transformation matrix for the mesh at the given index.
		 *	This matrix is also called "mesh root matrix" => see `mesh_root_matrix`!
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 */
		glm::mat4 transformation_matrix_for_mesh(mesh_index_t aMeshIndex) const;

		/** Same as the mesh transformation matrix => see `transformation_matrix_for_mesh`
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 */
		glm::mat4 mesh_root_matrix(mesh_index_t aMeshIndex) const;

		/**	Gets the actual number of bones that are associated to the given mesh index.
		 *	This number corresponds exactly to what ASSIMP's data structure reflects.
		 */
		uint32_t num_actual_bones(mesh_index_t aMeshIndex) const;
		
		/**	Gets the number of bones that are associated to the given mesh index.
		 *	This is the number of bones used for internal indices/animation setup.
		 *	In contrast to num_actual_bones, it returns 1u for meshes which actually
		 *	do not have any bones in their ASSIMP structure.
		 */
		uint32_t num_bone_matrices(mesh_index_t aMeshIndex) const;

		/**	Gets the number of bones that are associated with all the given mesh indices.
		 *	Note that the number of bones returned refer to "accumulated per-mesh bones",
		 *	and "per-mesh bones" means that actual bones (from the skeleton) can occur multiple
		 *	times across multiple meshes.
		 */
		uint32_t num_bone_matrices(std::vector<mesh_index_t> aMeshIndices) const;
		
		/**	Gets the inverse bind pose matrices for each bone assigned to the given mesh index.
		 *
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@param		aSourceSpace	The space which the inverse bone matrices transform from. Only two spaces are
		 *								supported: bone_matrices_space::mesh_space, and bone_matrices_space::model_space.
		 *								I.e. if bone_matrices_space::mesh_space is chosen, the input-vertices that are to
		 *								be transformed into bone-space must be provided in mesh coordinates.
		 *								Likewise, if bone_matrices_space::model_space is chosen, the input-vertices that 
		 *								are to be transformed into bone-space must be provided in model coordinates.
		 */
		std::vector<glm::mat4> inverse_bind_pose_matrices(mesh_index_t aMeshIndex, bone_matrices_space aSourceSpace = bone_matrices_space::mesh_space) const;

		/** Gets the name of the mesh at the given index (not to be confused with the material's name)
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Mesh name converted from Assimp's internal representation to std::string
		 */
		std::string name_of_mesh(mesh_index_t aMeshIndex) const;

		/** Gets Assimp's internal material index for the given mesh index. 
		 *	This value won't be useful if not operating directly on Assimp's internal materials.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Mesh index corresponding to Assimp's internal materials structure.
		 */
		size_t material_index_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets the name of material at the given material index
		 *	@param		aMaterialIndex		The index corresponding to the material
		 *	@return		Material name converted from Assimp's internal representation to std::string
		 */
		std::string name_of_material(size_t aMaterialIndex) const;

		/** Gets the `material_config` struct for the mesh at the given index.
		 *	The `material_config` struct is created from Assimp's internal material data.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		`material_config` struct, representing the "type of material". 
		 *				To actually load all the resources it refers to, you'll have 
		 *				to create a `material` based on it.
		 */
		material_config material_config_for_mesh(mesh_index_t aMeshIndex);

		/**	Sets some material config struct for the mesh at the given index.
		 *	@param	aMeshIndex			The index corresponding to the mesh
		 *	@param	aMaterialConfig		Material config that is to be assigned to the mesh.
		 */
		void set_material_config_for_mesh(mesh_index_t aMeshIndex, const material_config& aMaterialConfig);

		/**	Gets all distinct `material_config` structs foor this model and, as a bonus, so to say,
		 *	also gets all the mesh indices which have the materials assigned to.
		 *	@param	aAlsoConsiderCpuOnlyDataForDistinctMaterials	Setting this parameter to `true` means that for determining if a material is unique or not,
		 *															also the data in the material struct are evaluated which only remain on the CPU. This CPU 
		 *															data will not be transmitted to the GPU. By default, this parameter is set to `false`, i.e.
		 *															only the GPU data of the `material_config` struct will be evaluated when determining the distinct
		 *															materials.
		 *															You'll want to set this parameter to `true` if you are planning to adapt your draw calls based
		 *															on one or all of the following `material_config` members: `mShadingModel`, `mWireframeMode`,
		 *															`mTwosided`, `mBlendMode`. If you don't plan to differentiate based on these, set to `false`.
		 *	@return	A `std::unordered_map` containing the distinct `material_config` structs as the
		 *			keys and a vector of mesh indices as the value type, i.e. `std::vector<size_t>`. 
		 */
		std::unordered_map<material_config, std::vector<mesh_index_t>> distinct_material_configs(bool aAlsoConsiderCpuOnlyDataForDistinctMaterials = false);
			
		/** Gets the number of vertices for the mesh at the given index.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Number of vertices, which is also the length of all the vectors,
		 *				which are returned by: `positions_for_mesh`, `normals_for_mesh`,
		 *				`tangents_for_mesh`, `bitangents_for_mesh`, `colors_for_mesh`, 
		 *				and `texture_coordinates_for_mesh`
		 */
		size_t number_of_vertices_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the positions for the mesh at the given index.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of vertex positions, converted to `glm::vec3` 
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec3> positions_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the normals for the mesh at the given index.
		 *	If the mesh has no normals, a vector filled with values is
		 *	returned regardless. All the values will be set to (0,0,1) in this case.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of normals, converted to `glm::vec3`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec3> normals_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the tangents for the mesh at the given index.
		 *	If the mesh has no tangents, a vector filled with values is
		 *	returned regardless. All the values will be set to (1,0,0) in this case.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of tangents, converted to `glm::vec3`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec3> tangents_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the bitangents for the mesh at the given index.
		 *	If the mesh has no bitangents, a vector filled with values is
		 *	returned regardless. All the values will be set to (0,1,0) in this case.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of bitangents, converted to `glm::vec3`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec3> bitangents_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the colors of a specific color set for the mesh at the given index.
		 *	If the mesh has no colors for the given set index, a vector filled with values is
		 *	returned regardless. All the values will be set to (1,0,1,1) in this case (magenta).
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@param		aSet			Index to a specific set of colors
		 *	@return		Vector of colors, converted to `glm::vec4`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec4> colors_for_mesh(mesh_index_t aMeshIndex, int aSet = 0) const;

		/** Gets all the bone weights for the mesh at the given index.
		 *	If the mesh has no bone weights, a vector filled with values is
		 *	returned regardless. All the values will be set to (1,0,0,0) in this case.
		 *	@param		aMeshIndex		       The index corresponding to the mesh
		 *  @param      aNormalizeBoneWeights  Normalize the weights, so they add up to one
		 *	@return		Vector of bone weights, converted to `glm::vec4`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::vec4> bone_weights_for_mesh(mesh_index_t aMeshIndex, bool aNormalizeBoneWeights = false) const;

		/** Gets all the "mesh-local" bone indices for the mesh at the given index.
		 *	If the mesh has no bone indices, a vector filled with values is
		 *	returned regardless. All the values will be set to (0,0,0,0) in this case.
		 *
		 *	Additional information: The bone indices returned are the "mesh-local bone indices".
		 *                          This means that there can be fewer bone indices referenced than the skeleton
		 *                          has and -- most importantly -- the indices are zero-based w.r.t. to the
		 *                          specific MESH that is referred to by aMeshIndex (hence, "mesh-local").
		 *	
		 *	@param		aMeshIndex			The index corresponding to the mesh
		 *	@param		aBoneIndexOffset	An offset to be added to every single bone index returned by this method.
		 *	@return		Vector of bone indices, of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::uvec4> bone_indices_for_mesh(mesh_index_t aMeshIndex, uint32_t aBoneIndexOffset = 0) const;

		/** Gets unique "mesh-set-global" bone indices for the mesh at the given index w.r.t. the given set of mesh indices.
		 *	If the mesh has no bone indices, a vector filled with values is returned regardless.
		 *
		 *	Additional information: The bone indices returned are the "mesh-set-global bone indices".
		 *	                        "Global" w.r.t. a "mesh-set" means that the returned mesh indices of the
		 *	                        mesh identified by aMeshIndex are the "mesh-local bone indices" + an offset
		 *	                        which is the accumulated number of all bones assigned to the meshes before
		 *	                        
		 *                          This means that there can be fewer bone indices referenced than the skeleton
		 *                          has and -- most importantly -- the indices are zero-based w.r.t. to the
		 *                          specific MESH that is referred to by aMeshIndex (hence, "mesh-local").
		 *	
		 *	@param		aMeshIndex						The index corresponding to the mesh
		 *	@param		aMeshIndicesWithBonesInOrder	The correct offset for the given mesh index is determined based on this set.
		 *												I.e. the offset will be the accumulated value of all previous #bone-matrices
		 *												in the set before the mesh with the given index.
		 *	@return		Vector of bone indices, of length `number_of_vertices_for_mesh()`
		 */
		std::vector<glm::uvec4> bone_indices_for_mesh_for_single_target_buffer(mesh_index_t aMeshIndex, const std::vector<mesh_index_t>& aMeshIndicesWithBonesInOrder) const;
		
		/** Gets unique "mesh-set-global" bone indices for all the given mesh indices.
		 *	If the mesh has no bone indices, a vector filled with values is returned regardless.
		 *
		 *	Additional information: The bone indices returned are the "mesh-set-global bone indices".
		 *	                        "Global" w.r.t. a "mesh-set" means that the returned mesh indices of the
		 *	                        mesh identified by aMeshIndex are the "mesh-local bone indices" + an offset
		 *	                        which is the accumulated number of all bones assigned to the meshes before
		 *	                        
		 *                          This means that there can be fewer bone indices referenced than the skeleton
		 *                          has and -- most importantly -- the indices are zero-based w.r.t. to the
		 *                          specific MESH that is referred to by aMeshIndex (hence, "mesh-local").
		 *	
		 *	@param		aMeshIndex			The index corresponding to the mesh
		 *	@param		aBoneIndexOffset	An offset to be added to every single bone index returned by this method.
		 *	@return		Vector of bone indices, of accumulated length `number_of_vertices_for_mesh` for each one of the given mesh indices.
		 */
		std::vector<glm::uvec4> bone_indices_for_meshes_for_single_target_buffer(const std::vector<mesh_index_t>& aMeshIndices, uint32_t aInitialBoneIndexOffset = 0u) const;

		/** Gets the number of uv-components of a specific UV-set for the mesh at the given index
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@param		aSet			Index to a specific set of texture coordinates
		 *	@return		Number of uv components the given set has. This can, e.g., be used to 
		 *				determine how to retrieve the texture coordinates: as vec2 or as vec3, 
		 *				like follows: `texture_coordinates_for_mesh<vec2>(0)` or `texture_coordinates_for_mesh<vec3>(0)`, respectively.
		 */
		int num_uv_components_for_mesh(mesh_index_t aMeshIndex, int aSet = 0) const;

		/** Gets all the texture coordinates of a UV-set for the mesh at the given index.
		 *	If the mesh has no colors for the given set index, a vector filled with values is
		 *	returned regardless. You'll have to specify the type of UV-coordinates which you
		 *	want to retrieve. Supported types are `glm::vec2` and `glm::vec3`.
		 *	@param		aTransformFunc	A function that can be used to transform the texture coordinates
		 *								while adding them to the resulting vector. This can be, e.g., to
		 *								flip texture coordinates.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@param		aSet			Index to a specific set of UV-coordinates
		 *	@return		Vector of UV-coordinates, converted to `T`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		template <typename T> std::vector<T> texture_coordinates_for_mesh(T(*aTransformFunc)(const T&), mesh_index_t aMeshIndex, int aSet = 0) const
		{
			throw avk::logic_error(std::format("unsupported type {}", typeid(T).name()));
		}

		/** Gets all the texture coordinates of a UV-set for the mesh at the given index.
		 *	If the mesh has no colors for the given set index, a vector filled with values is
		 *	returned regardless. You'll have to specify the type of UV-coordinates which you
		 *	want to retrieve. Supported types are `glm::vec2` and `glm::vec3`.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@param		aSet			Index to a specific set of UV-coordinates
		 *	@return		Vector of UV-coordinates, converted to `T`
		 *				of length `number_of_vertices_for_mesh()`
		 */
		template <typename T> std::vector<T> texture_coordinates_for_mesh(mesh_index_t aMeshIndex, int aSet = 0) const
		{
			return texture_coordinates_for_mesh<T>([](const T& aValue) { return aValue; }, aMeshIndex, aSet);
		}

		/** Gets the number of indices for the mesh at the given index.
		 *	Please note: Theoretically it can happen that a mesh has faces with different 
		 *	numbers of vertices (e.g. triangles and quads). Use the `aiProcess_Triangulate`
		 *	import flag to get only triangles, or make sure to handle them properly.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Number of indices for the given mesh.
		 */
		int number_of_indices_for_mesh(mesh_index_t aMeshIndex) const;

		/** Gets all the indices for the mesh at the given index.
		 *	@param		aMeshIndex		The index corresponding to the mesh
		 *	@return		Vector of vertex positions, converted to type `T`
		 *				of length `number_of_indices_for_mesh()`.
		 *				In most cases, you'll want to pass `uint16_t` or `uint32_t` for `T`.
		 */
		template <typename T> 
		std::vector<T> indices_for_mesh(mesh_index_t aMeshIndex) const
		{ 
			const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
			size_t indicesCount = number_of_indices_for_mesh(aMeshIndex);
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

		/** Returns the number of meshes. */
		mesh_index_t num_meshes() const { return mScene->mNumMeshes; }

		/** Return the indices of all meshes which the given predicate evaluates true for.
		 *	Function-signature: bool(mesh_index_t, const aiMesh*) where the first parameter is the 
		 *									mesh index and the second the pointer to the data
		 */
		template <typename F>
		std::vector<mesh_index_t> select_meshes(F aPredicate) const
		{
			std::vector<mesh_index_t> result;
			for (mesh_index_t i = 0; i < mScene->mNumMeshes; ++i) {
				const aiMesh* paiMesh = mScene->mMeshes[i];
				if (aPredicate(i, paiMesh)) {
					result.push_back(i);
				}
			}
			return result;
		}

		/** Return the indices of all meshes. It's effecively the same as calling
		 *	`select_meshes` with a predicate that always evaluates true.
		 */
		std::vector<mesh_index_t> select_all_meshes() const;

		std::vector<glm::vec3> positions_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::vec3> normals_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::vec3> tangents_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::vec3> bitangents_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;
		std::vector<glm::vec4> colors_for_meshes(std::vector<mesh_index_t> aMeshIndices, int aSet = 0) const;
		std::vector<glm::vec4> bone_weights_for_meshes(std::vector<mesh_index_t> aMeshIndices, bool aNormalizeBoneWeights = false) const;
		std::vector<glm::uvec4> bone_indices_for_meshes(std::vector<mesh_index_t> aMeshIndices) const;

		template <typename T>
		std::vector<T> texture_coordinates_for_meshes(std::vector<mesh_index_t> aMeshIndices, int aSet = 0) const
		{
			std::vector<T> result;
			for (auto meshIndex : aMeshIndices) {
				auto tmp = texture_coordinates_for_mesh<T>(meshIndex, aSet);
				std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
			}
			return result;
		}

		template <typename T>
		std::vector<T> indices_for_meshes(std::vector<mesh_index_t> aMeshIndices) const
		{
			std::vector<T> result;
			for (auto meshIndex : aMeshIndices) {
				auto tmp = indices_for_mesh<T>(meshIndex);
				std::move(std::begin(tmp), std::end(tmp), std::back_inserter(result));
			}
			return result;
		}

		/** Returns all lightsources stored in the model file */
		std::vector<lightsource> lights() const;

		/** Returns all cameras stored in the model file */
		std::vector<avk::camera> cameras() const;

		/** Load an animation clip's data */
		animation_clip_data load_animation_clip(unsigned int aAnimationIndex, double aStartTimeTicks, double aEndTimeTicks) const;

		/**	Prepare an animation data structure for the given animation index and the given mesh indices.
		 *	The bone matrices shall be written into contiguous memory, but the stride between the start
		 *	of each mesh's memory offset can be specified.
		 *	
		 *	@param	aAnimationIndex				The animation index to create the animation data for
		 *	@param	aMeshIndices				Vector of mesh indices to meshes which shall be included in the animation.
		 */
		animation prepare_animation(uint32_t aAnimationIndex, const std::vector<mesh_index_t>& aMeshIndices);

		/** Calculates the tangent space with Auto-Vk-Toolkit's implementation,
		 *  possibly overwriting ASSIMPs tangents/bitangents.
		 *	@param	aMeshIndex			Mesh which the tangent space shall be computed for
		 *	@param	aConfigSourceUV		The set of UV coordinates to be used for tangent space calculation
		 */
		void calculate_tangent_space_for_mesh(mesh_index_t aMeshIndex, uint32_t aConfigSourceUV = 0);

		/** Calculates the tangent space with Auto-Vk-Toolkit's implementation for all meshes,
		 *  possibly overwriting ASSIMPs tangents/bitangents.
		 *	@param	aConfigSourceUV		The set of UV coordinates to be used for tangent space calculation
		 */
		void calculate_tangent_space_for_all_meshes(uint32_t aConfigSourceUV = 0);
		
	private:
		void initialize_materials();
		aiNode* find_mesh_root_node(unsigned int aMeshIndexToFind) const;
		aiNode* mesh_node_traverser(unsigned int aMeshIndexToFind, aiNode* aNode) const;
		std::optional<glm::mat4> transformation_matrix_traverser(unsigned int aMeshIndexToFind, const aiNode* aNode, const aiMatrix4x4& aM) const;
		std::optional<glm::mat4> transformation_matrix_traverser_for_light(const aiLight* aLight, const aiNode* Node, const aiMatrix4x4& aM) const;
		std::optional<glm::mat4> transformation_matrix_traverser_for_camera(const aiCamera* aCamera, const aiNode* aNode, const aiMatrix4x4& aM) const;

		/** Helper function which adds the given node and all its child nodes to the given map
		 */
		void add_all_to_node_map(std::unordered_map<std::string, aiNode*>& aNodeMap, aiNode* aNode);

						
		/** Helper function return true if the two given collections have the same size and
		 *	contain keys with the same timestamp values.
		 *	Each of the types must have a .size() member and must be accessible via an indexer.
		 *	Each type's child element must have a .mTime member of type double.
		 */
		template <typename T1, typename T2>
		bool have_same_key_times(const T1& aCollection1, const T2& aCollection2)
		{
			if (aCollection1.size() != aCollection2.size()) {
				return false;
			}
			for (size_t i = 0; i < aCollection1.size(); ++i) {
				if (glm::abs(aCollection1[i].mTime - aCollection2[i].mTime) > 2.3e-16 /* ~machine epsilon */) {
					return false;
				}
			}
			return true;
		}

		std::unique_ptr<Assimp::Importer> mImporter;
		std::string mModelPath;
		const aiScene* mScene;
		std::vector<std::optional<material_config>> mMaterialConfigPerMesh;
		std::unordered_map<mesh_index_t, std::tuple<std::vector<glm::vec3>, std::vector<glm::vec3>>> mTangentsAndBitangents;
	};

	using model = avk::owning_resource<model_t>;


	template <>
	inline std::vector<glm::vec2> model_t::texture_coordinates_for_mesh<glm::vec2>(glm::vec2(*aTransformFunc)(const glm::vec2&), mesh_index_t aMeshIndex, int aSet) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec2> result;
		result.reserve(n);
		assert(aSet >= 0 && aSet < AI_MAX_NUMBER_OF_TEXTURECOORDS);
		if (nullptr == paiMesh->mTextureCoords[aSet]) {
			LOG_WARNING(std::format("The mesh at index {} does not contain a texture coordinates at index {}. Will return (0,0) for each vertex.", aMeshIndex, aSet));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(0.f, 0.f);
			}
		}
		else {
			const auto nuv = num_uv_components_for_mesh(aMeshIndex, aSet);
			switch (nuv) {
			case 1:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(aTransformFunc(glm::vec2{ paiMesh->mTextureCoords[aSet][i][0], 0.f }));
				}
				break;
			case 2:
			case 3:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(aTransformFunc(glm::vec2{ paiMesh->mTextureCoords[aSet][i][0], paiMesh->mTextureCoords[aSet][i][1] }));
				}
				break;
			default:
				throw avk::logic_error(std::format("Can't handle a number of {} uv components for mesh at index {}, set {}.", nuv, aMeshIndex, aSet));
			}
		}
		return result;
	}

	template <>
	inline std::vector<glm::vec3> model_t::texture_coordinates_for_mesh<glm::vec3>(mesh_index_t aMeshIndex, int aSet) const
	{
		const aiMesh* paiMesh = mScene->mMeshes[aMeshIndex];
		auto n = paiMesh->mNumVertices;
		std::vector<glm::vec3> result;
		result.reserve(n);
		assert(aSet >= 0 && aSet < AI_MAX_NUMBER_OF_TEXTURECOORDS);
		if (nullptr == paiMesh->mTextureCoords[aSet]) {
			LOG_WARNING(std::format("The mesh at index {} does not contain a texture coordinates at index {}. Will return (0,0,0) for each vertex.", aMeshIndex, aSet));
			for (decltype(n) i = 0; i < n; ++i) {
				result.emplace_back(0.f, 0.f, 0.f);
			}
		}
		else {
			const auto nuv = num_uv_components_for_mesh(aMeshIndex, aSet);
			switch (nuv) {
			case 1:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[aSet][i][0], 0.f, 0.f);
				}
				break;
			case 2:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[aSet][i][0], paiMesh->mTextureCoords[aSet][i][1], 0.f);
				}
				break;
			case 3:
				for (decltype(n) i = 0; i < n; ++i) {
					result.emplace_back(paiMesh->mTextureCoords[aSet][i][0], paiMesh->mTextureCoords[aSet][i][1], paiMesh->mTextureCoords[aSet][i][2]);
				}
				break;
			default:
				throw avk::logic_error(std::format("Can't handle a number of {} uv components for mesh at index {}, set {}.", nuv, aMeshIndex, aSet));
			}
		}
		return result;
	}

	/** Helper function used by `avk::append_indices_and_vertex_data` */
	template <typename Vert>
	size_t get_vertex_count(const Vert& aFirst)
	{
		return aFirst.size();
	}

	/** Helper function used by `avk::append_indices_and_vertex_data` */
	template <typename Vert, typename... Verts>
	size_t get_vertex_count(const Vert& aFirst, const Verts&... aRest)
	{
#if defined(_DEBUG) 
		// Check whether all of the vertex data has the same length!
		auto countOfNext = get_vertex_count(aRest...);
		if (countOfNext != aFirst.size()) {
			throw avk::logic_error(std::format("The vertex data passed are not all of the same length, namely {} vs. {}.", countOfNext, aFirst.size()));
		}
#endif
		return aFirst.size();
	}

	/** Inserts the elements from the collection `aToInsert` into the collection `aDestination`. */
	template <typename V>
	void insert_into(V& aDestination, const V& aToInsert)
	{
		aDestination.insert(std::end(aDestination), std::begin(aToInsert), std::end(aToInsert));
	}

	/** Inserts the elements from the collection `aToInsert` into the collection `aDestination` and adds `aToAdd` to them. */
	template <typename V, typename A>
	void insert_into_and_add(V& aDestination, const V& aToInsert, A aToAdd)
	{
		aDestination.reserve(aDestination.size() + aToInsert.size());
		auto addValType = static_cast<typename V::value_type>(aToAdd);
		for (auto& e : aToInsert) {
			aDestination.push_back(e + addValType);
		}
	}

	/** Utility function to concatenate lists of vertex data and according lists of index data.
	 *	The vertex data is concatenated unmodified, and an arbitrary number of vertex data vectors is supported.
	 *	The index data, however, will be modified during concatenation to account for the vertices which come before.
	 *
	 *	Example: 
	 *	If there are already 100 vertices in the vertex data vectors, adding the indices 0, 2, 1 will result in
	 *	actually the values 100+0, 100+2, 100+1, i.e. 100, 102, 101, being added to the vector of existing indices.
	 *
	 *	Usage:
	 *	This method takes `std::tuple`s as parameters to assign source collections to destination collections.
	 *	The destinations are referring to collections, while the sources must be lambdas providing the data.
	 *		Example: `std::vector<glm::vec3> positions;` for the first parameter and `[&]() { return someModel->positions_for_meshes({ 0 }); }` for the second parameter.
	 *	Please note that the first parameter of these tuples is captured by reference, which requires 
	 *	`std::forward_as_tuple` to be used. For better readability, `avk::additional_index_data` and
	 *	`avk::additional_vertex_data` can be used instead, which are actually just the same as `std::forward_as_tuple`.
	 *
	 *
	 */
	template <typename... Vert, typename... Getter, typename Ind, typename IndGetter>
	void append_indices_and_vertex_data(std::tuple<Ind&, IndGetter> aIndDstAndGetter, std::tuple<Vert&, Getter>... aVertDstAndGetterPairs)
	{
		// Count vertices BEFORE appending!
		auto vertexCount = get_vertex_count(std::get<0>(aVertDstAndGetterPairs)...);
		// Append all the vertex data:
		(insert_into(/* Existing vector: */ std::get<0>(aVertDstAndGetterPairs), /* Getter: */ std::move(std::get<1>(aVertDstAndGetterPairs)())), ...);
		// Append the index data:
		insert_into_and_add(std::get<0>(aIndDstAndGetter), std::get<1>(aIndDstAndGetter)(), vertexCount);
		//insert_into_and_add(_A, _B(), vertexCount);
	}

	/** This is actually just an alias to `std::forward_as_tuple`. It does not add any functionality,
	 *	but it should help to express the intent better. Use it with `avk::append_vertex_data_and_indices`!
	 */
	template <class... Types>
	[[nodiscard]] constexpr std::tuple<Types&&...> additional_vertex_data(Types&&... aArgs) noexcept {
		return std::forward_as_tuple(std::forward<Types>(aArgs)...);
	}

	/** This is actually just an alias to `std::forward_as_tuple`. It does not add any functionality,
	 *	but it should help to express the intent better. Use it with `avk::append_vertex_data_and_indices`!
	 */
	template <class... Types>
	[[nodiscard]] constexpr std::tuple<Types&&...> additional_index_data(Types&&... aArgs) noexcept {
		return std::forward_as_tuple(std::forward<Types>(aArgs)...);
	}

	///** This is a convenience function and is actually just an alias to `std::forward_as_tuple`. It does not add any functionality,
	// *	but it should help to express the intent better. 
	// */
	//template <typename M>
	//[[nodiscard]] constexpr std::tuple<std::reference_wrapper<model_t>, std::vector<size_t>> make_tuple_model_and_indices(const M& _Model, std::vector<mesh_index_t> _Indices) noexcept {
	//	return std::forward_as_tuple<std::reference_wrapper<model_t>, std::vector<size_t>>(std::ref(_Model), std::move(_Indices));
	//}


}

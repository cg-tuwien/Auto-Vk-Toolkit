#pragma once
#include <gvk.hpp>

namespace gvk
{
	/** Meshlet struct for the CPU side. */
	struct meshlet
	{
		/** The model of the meshlet. */
		model mModel;
		/** The optional mesh index of the meshlet.
		 *  Only set if the submeshes were not combined upon creation of the meshlet.
		 */
		std::optional<mesh_index_t> mMeshIndex;
		/** Contains indices into the original vertex attributes of the mesh.
		 *  If submeshes were combined, it indexes the vertex attributes of the combined meshes as done with get_vertices_and_indices.
		 */
		std::vector<uint32_t> mVertices;
		/** Contains indices into the mVertices vector. */
		std::vector<uint32_t> mIndices;
		/** The actual number of vertices inside of mVertices; */
		uint32_t mVertexCount;
		/** The actual number of indices in mIndices; */
		uint32_t mIndexCount;
	};

	/** Divides the given index buffer into meshlets by simply aggregating every aMaxVertices indices into a meshlet.
	 *  @param	aIndices			The index buffer.
	 *  @param	aModel				The model these buffers belong to.
	 *	@param	aMeshIndex			The optional mesh index of the mesh these buffers belong to.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet.
	 */
	std::vector<meshlet> basic_meshlets_divider(
		const std::vector<uint32_t>& aIndices,
		const model_t& aModel,
		std::optional<mesh_index_t> aMeshIndex,
		uint32_t aMaxVertices, uint32_t aMaxIndices);

	/** Divides the given vertex and index buffer into meshlets using the given callback function.
	 *  @param	aVertices			The vertex buffer.
	 *  @param	aIndices			The index buffer.
	 *  @param	aModel				The model these buffers belong to.
	 *	@param	aMeshIndex			The optional mesh index of the mesh these buffers belong to.
	 *  @param	aMeshletDivision	Callback used to divide meshes into meshlets with a maximum number of vertices and indices each.
	 *								It can either receive the vertices and indices or just the indices depending on your specific needs.
	 *								Additionally it provides the model and an optional mesh index if more data is needed. If no mesh index is provided then the meshes were combined beforehand.
	 *								Ownership of the model must not be taken within the body of aMeshletDivision. The model will be assigned to each `meshlet` after aMeshletDivision has executed. \n\n
	 *								The callback must follow a specific declaration schema, optional parameters can be omitted, but all of them need to be provided in the following order:
	 *								 - const std::vector<glm::vec3>& tVertices:		optional	The vertices of the mesh or combined meshes of the model
	 *								 - const std::vector<uint32_t>& tIndices:  		mandatory	The indices of the mesh or combined meshes of the model
	 *								 - const model_t& tModel:						mandatory	The model these meshlets are generated from
	 *								 - std::optional<mesh_index_t> tMeshIndex: 		mandatory	The optional mesh index. If no value is provided, it means the meshes of the model are combined into a single vertex and index buffer
	 *								 - uint32_t tMaxVertices:						mandatory	The maximum number of vertices that are allowed in a single meshlet.
	 *								 - uint32_t tMaxIndices:						mandatory	The maximum number of indices that are allowed in a single meshlet.
	 *								Return value:
	 *								 - std::vector<meshlet>										The generated meshlets.\n\n
	 *								Example:
	 *								@code
	 *								[](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices,
	 *											const model_t& aModel, std::optional<mesh_index_t> aMeshIndex,
	 *											uint32_t aMaxVertices, uint32_t aMaxIndices) {
	 *									std::vector<gvk::meshlet> generatedMeshlets;
	 *									// Do your meshlet division here
	 *									return generatedMeshlets;
	 *								}
	 *								@endcode
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet. This value is just passed on to aMeshletDivision.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet. This value is just passed on to aMeshletDivision.
	 */
	template <typename F>
	std::vector<meshlet> divide_indexed_geometry_into_meshlets(
		const std::vector<glm::vec3>& aVertices,
		const std::vector<uint32_t>& aIndices,
		avk::resource_ownership<gvk::model_t> aModel,
		const std::optional<mesh_index_t> aMeshIndex,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices,
		F aMeshletDivision)
	{
		std::vector<meshlet> generatedMeshlets;
		auto ownedModel = aModel.own();
		ownedModel.enable_shared_ownership();

		if constexpr (std::is_assignable_v<std::function<std::vector<meshlet>(const std::vector<uint32_t>& tIndices, const model_t& tModel, std::optional<mesh_index_t> tMeshIndex, uint32_t tMaxVertices, uint32_t tMaxIndices)>, decltype(aMeshletDivision)>) {
			generatedMeshlets = aMeshletDivision(aIndices, ownedModel.get(), aMeshIndex, aMaxVertices, aMaxIndices);
		}
		else if constexpr (std::is_assignable_v<std::function<std::vector<meshlet>(const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& tIndices, const model_t& tModel, std::optional<mesh_index_t> tMeshIndex, uint32_t tMaxVertices, uint32_t tMaxIndices)>, decltype(aMeshletDivision)>) {
			generatedMeshlets = aMeshletDivision(aVertices, aIndices, ownedModel.get(), aMeshIndex, aMaxVertices, aMaxIndices);
		}
		else {
#if defined(_MSC_VER) && defined(__cplusplus)
			static_assert(false);
#else
			assert(false);
#endif
			throw avk::logic_error("No compatible lambda has been passed to divide_into_meshlets.");
		}

		for (auto& meshlet : generatedMeshlets)
		{
			meshlet.mModel = ownedModel;
		}

		return generatedMeshlets;
	}

	/** Divides the given models into meshlets using the default implementation divide_into_meshlets_simple.
	 *  @param	aModelsAndMeshletIndices				All the models and associated meshes that should be divided into meshlets.
	 *	@param	aCombineSubmeshes	If submeshes should be combined into a single vertex/index buffer.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet.
	 */
	std::vector<meshlet> divide_into_meshlets(std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndMeshletIndices,
		const bool aCombineSubmeshes = true, const uint32_t aMaxVertices = 64, const uint32_t aMaxIndices = 378);

	/** Divides the given models into meshlets using the given callback function.
	 *  @param	aModelsAndMeshletIndices				All the models and associated meshes that should be divided into meshlets.
	 *  @param	aMeshletDivision	Callback used to divide meshes into meshlets with a maximum number of vertices and indices each.
	 *								It can either receive the vertices and indices or just the indices depending on your specific needs.
	 *								Additionally it provides the model and an optional mesh index if more data is needed. If no mesh index is provided then the meshes were combined beforehand.
	 *								Ownership of the model must not be taken within the body of aMeshletDivision. The model will be assigned to each `meshlet` after aMeshletDivision has executed. \n\n
	 *								The callback must follow a specific declaration schema, optional parameters can be omitted, but all of them need to be provided in the following order:
	 *								 - const std::vector<glm::vec3>& tVertices:		optional	The vertices of the mesh or combined meshes of the model
	 *								 - const std::vector<uint32_t>& tIndices:  		mandatory	The indices of the mesh or combined meshes of the model
	 *								 - const model_t& tModel:						mandatory	The model these meshlets are generated from
	 *								 - std::optional<mesh_index_t> tMeshIndex: 		mandatory	The optional mesh index. If no value is provided, it means the meshes of the model are combined into a single vertex and index buffer
	 *								 - uint32_t tMaxVertices:						mandatory	The maximum number of vertices that are allowed in a single meshlet.
	 *								 - uint32_t tMaxIndices:						mandatory	The maximum number of indices that are allowed in a single meshlet.
	 *								Return value:
	 *								 - std::vector<meshlet>										The generated meshlets.\n\n
	 *								Example:
	 *								@code
	 *								[](const std::vector<glm::vec3>& tVertices, const std::vector<uint32_t>& aIndices,
	 *											const model_t& aModel, std::optional<mesh_index_t> aMeshIndex,
	 *											uint32_t aMaxVertices, uint32_t aMaxIndices) {
	 *									std::vector<gvk::meshlet> generatedMeshlets;
	 *									// Do your meshlet division here
	 *									return generatedMeshlets;
	 *								}
	 *								@endcode
	 *	@param	aCombineSubmeshes	If submeshes should be combined into a single vertex/index buffer.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet. This value is just passed on to aMeshletDivision.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet. This value is just passed on to aMeshletDivision.
	 */
	template <typename F>
	std::vector<meshlet> divide_into_meshlets(std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndMeshletIndices, F aMeshletDivision,
		const bool aCombineSubmeshes = true, const uint32_t aMaxVertices = 64, const uint32_t aMaxIndices = 378)
	{
		std::vector<meshlet> meshlets;
		for (auto& pair : aModelsAndMeshletIndices) {
			auto& model = std::get<avk::resource_ownership<model_t>>(pair);
			auto& meshIndices = std::get<std::vector<mesh_index_t>>(pair);

			if (aCombineSubmeshes) {
				auto [vertices, indices] = get_vertices_and_indices(std::vector({ std::make_tuple(avk::const_referenced(model.get()), meshIndices) }));
				std::vector<meshlet> tmpMeshlets = divide_indexed_geometry_into_meshlets(vertices, indices, std::move(model), std::nullopt, aMaxVertices, aMaxIndices, std::move(aMeshletDivision));
				// append to meshlets
				meshlets.insert(std::end(meshlets), std::make_move_iterator(std::begin(tmpMeshlets)), std::make_move_iterator(std::end(tmpMeshlets)));
			}
			else {
				for (const auto meshIndex : meshIndices) {
					auto vertices = model.get().positions_for_mesh(meshIndex);
					auto indices = model.get().indices_for_mesh<uint32_t>(meshIndex);
					std::vector<meshlet> tmpMeshlets = divide_indexed_geometry_into_meshlets(vertices, indices, std::move(model), meshIndex, aMaxVertices, aMaxIndices, std::move(aMeshletDivision));
					// append to meshlets
					meshlets.insert(std::end(meshlets), std::make_move_iterator(std::begin(tmpMeshlets)), std::make_move_iterator(std::end(tmpMeshlets)));
				}
			}
		}
		return meshlets;
	}
}


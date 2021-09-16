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

	/** Divides the given models into meshlets using the default implementation divide_into_very_bad_meshlets.
	 *  @param	aModels				All the models and associated meshes that should be divided into meshlets.
	 *								If aCombineSubmeshes is enabled, all the submeshes of a given model will be combined into a single vertex/index buffer.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet.
	 *	@param	aCombineSubmeshes	If submeshes should be combined into a single vertex/index buffer.
	 */
	std::vector<meshlet> divide_into_meshlets(const std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<mesh_index_t>>>& aModels,
		const uint32_t aMaxVertices = 64, const uint32_t aMaxIndices = 378, const bool aCombineSubmeshes = true);


	/** Divides the given models into meshlets using the given callback function.
	 *  @param	aModels				All the models and associated meshes that should be divided into meshlets.
	 *								If aCombineSubmeshes is enabled, all the submeshes of a given model will be combined into a single vertex/index buffer.
	 *  @param	aMeshletDivision	Callback used to divide meshes into meshlets.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet.
	 *	@param	aCombineSubmeshes	If submeshes should be combined into a single vertex/index buffer.
	 */
	template <typename F>
	std::vector<meshlet> divide_into_meshlets(const std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<mesh_index_t>>>& aModels, F&& aMeshletDivision,
		const uint32_t aMaxVertices = 64, const uint32_t aMaxIndices = 378, const bool aCombineSubmeshes = true);

	/** Divides the given vertex and index buffer into meshlets using the given callback function.
	 *  @param	aVertices			The vertex buffer.
	 *  @param	aIndices			The index buffer.
	 *  @param	aModel				The model these buffers belong to.
	 *	@param	aMeshIndex			The optional mesh index of the mesh these buffers belong to.
	 *  @param	aMeshletDivision	Callback used to divide meshes into meshlets.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet.
	 */
	template <typename F>
	std::vector<meshlet> divide_vertices_into_meshlets(
		const std::vector<glm::vec3>& aVertices,
		const std::vector<uint32_t>& aIndices,
		avk::resource_ownership<gvk::model_t> aModel,
		const std::optional<mesh_index_t> aMeshIndex,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices,
		F&& aMeshletDivision);

	/** Divides the given index buffer into meshlets using a very bad algorithm. Use something else if possible.
	 *  @param	aIndices			The index buffer.
	 *  @param	aModel				The model these buffers belong to.
	 *	@param	aMeshIndex			The optional mesh index of the mesh these buffers belong to.
	 *	@param	aMaxVertices		The maximum number of vertices of a meshlet.
	 *	@param	aMaxIndices			The maximum number of indices of a meshlet.
	 */
	std::vector<meshlet> divide_into_very_bad_meshlets(
		const std::vector<uint32_t>& aIndices,
		avk::resource_ownership<gvk::model_t> aModel,
		const std::optional<mesh_index_t> aMeshIndex,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices);
}

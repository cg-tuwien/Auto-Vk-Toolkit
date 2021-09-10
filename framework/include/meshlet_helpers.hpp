#pragma once
#include <gvk.hpp>

namespace gvk
{
	struct meshlet
	{
		gvk::model mModel;
		std::optional<mesh_index_t> mMeshIndex;
		std::vector<glm::vec3> mVertices;
		std::vector<uint32_t> mIndices;
		uint32_t mVertexCount;
		uint32_t mIndexCount;
	};

	static std::vector<meshlet> divide_into_meshlets(const std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<mesh_index_t>>>& aModels,
		const uint32_t aMaxVertices = 64, const uint32_t aMaxIndices = 378, const bool aCombineSubmeshes = true);

	static std::vector<meshlet> divide_into_very_bad_meshlets(
		const std::vector<glm::vec3>& aPositions,
		const std::vector<uint32_t>& aIndices,
		avk::resource_ownership<gvk::model_t> aModel,
		const std::optional<mesh_index_t> aMeshIndex,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices);
}

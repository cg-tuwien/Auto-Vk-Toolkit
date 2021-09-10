#pragma once
#include <gvk.hpp>

namespace gvk
{
	struct meshlet
	{
		gvk::model mModel;
		mesh_index_t mMeshIndex;
		std::vector<glm::vec3> vertices;
		std::vector<uint32_t> indices;
		uint32_t mVertexCount;
		uint32_t mIndexCount;
	};

	static std::vector<meshlet> divide_into_meshlets(const std::vector<std::tuple<avk::owning_resource<gvk::model_t>, std::vector<mesh_index_t>>>& aModels,
		const uint32_t aMaxVertices = 64, const uint32_t aMaxIndices = 378);

	static std::vector<meshlet> divide_into_very_bad_meshlets(
		const std::vector<glm::vec3>& aPositions,
		const std::vector<uint32_t>& aIndices,
		avk::owning_resource<gvk::model_t> aModel,
		const mesh_index_t aMeshIndex,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices);
}

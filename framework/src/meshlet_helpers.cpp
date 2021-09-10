#include <gvk.hpp>
#include "meshlet_helpers.hpp"

static std::vector<gvk::meshlet> gvk::divide_into_meshlets(const std::vector<std::tuple<avk::owning_resource<gvk::model_t>, std::vector<mesh_index_t>>>& aModels,
	const uint32_t aMaxVertices, const uint32_t aMaxIndices)
{
	std::vector<meshlet> meshlets;
	for(auto& pair : aModels)
	{
		auto& model = std::get<avk::owning_resource<gvk::model_t>>(pair);
		auto& meshIndices = std::get<std::vector<mesh_index_t>>(pair);
		for (const auto meshIndex : meshIndices) 
		{
			auto vertices = model.get().positions_for_mesh(meshIndex);
			auto indices = model.get().indices_for_mesh<uint32_t>(meshIndex);
			// default to very bad meshlet generation
			auto tmpMeshlets = divide_into_very_bad_meshlets(vertices, indices, model, meshIndex, aMaxVertices, aMaxIndices);
			// append to meshlets
			meshlets.insert(std::end(meshlets), std::begin(tmpMeshlets), std::end(tmpMeshlets));
		}
	}
	return meshlets;
}

std::vector<gvk::meshlet> gvk::divide_into_very_bad_meshlets(const std::vector<glm::vec3>& aPositions,
	const std::vector<uint32_t>& aIndices,
	avk::owning_resource<gvk::model_t> aModel,
	const mesh_index_t aMeshIndex,
	const uint32_t aMaxVertices, const uint32_t aMaxIndices)
{
	std::vector<meshlet> veryBadMeshlets;

	int vertexIndex = 0;
	const int numIndices = static_cast<int>(aIndices.size());
	while (vertexIndex < numIndices) {
		auto& ml = veryBadMeshlets.emplace_back();
		ml.mVertexCount = 0u;
		ml.vertices.reserve(aMaxVertices);
		ml.indices.reserve(aMaxIndices);
		int meshletVertexIndex = 0;
		// this simple algorithm duplicates vertices, therefore vertexCount == indexCount
		while (meshletVertexIndex < aMaxVertices - 3 && ml.mVertexCount < aMaxIndices - 3 && vertexIndex + meshletVertexIndex < numIndices) {
			ml.indices[meshletVertexIndex / 3 + 0] = meshletVertexIndex + 0;
			ml.indices[meshletVertexIndex / 3 + 1] = meshletVertexIndex + 1;
			ml.indices[meshletVertexIndex / 3 + 2] = meshletVertexIndex + 2;
			ml.vertices[meshletVertexIndex + 0] = aPositions[aIndices[vertexIndex + meshletVertexIndex + 0]];
			ml.vertices[meshletVertexIndex + 1] = aPositions[aIndices[vertexIndex + meshletVertexIndex + 1]];
			ml.vertices[meshletVertexIndex + 2] = aPositions[aIndices[vertexIndex + meshletVertexIndex + 2]];
			ml.mVertexCount += 3u;

			meshletVertexIndex += 3;
		}
		ml.mIndexCount = ml.mVertexCount;
		// resize to actual size
		ml.vertices.resize(ml.mVertexCount);
		ml.vertices.shrink_to_fit();
		ml.indices.resize(ml.mVertexCount);
		ml.indices.shrink_to_fit();
		vertexIndex += meshletVertexIndex;
	}

	return veryBadMeshlets;
}

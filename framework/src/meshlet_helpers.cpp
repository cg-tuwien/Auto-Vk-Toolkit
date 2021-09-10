#include <gvk.hpp>
#include "meshlet_helpers.hpp"

namespace gvk
{
	static std::vector<meshlet> divide_into_meshlets(const std::vector<std::tuple<avk::resource_ownership<model_t>, std::vector<mesh_index_t>>>& aModels,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices, const bool aCombineSubmeshes)
	{
		std::vector<meshlet> meshlets;
		for (auto& pair : aModels) {
			auto tmpModel = std::get<avk::resource_ownership<model_t>>(pair);
			auto& meshIndices = std::get<std::vector<mesh_index_t>>(pair);
			model ownedModel = tmpModel.own();

			if (aCombineSubmeshes) {
				auto [vertices, indices] = get_vertices_and_indices(std::vector({ std::make_tuple(avk::const_referenced(ownedModel), meshIndices) }));
				// default to very bad meshlet generation
				auto tmpMeshlets = divide_into_very_bad_meshlets(vertices, indices, avk::owned(ownedModel), std::nullopt, aMaxVertices, aMaxIndices);
				// append to meshlets
				meshlets.insert(std::end(meshlets), std::begin(tmpMeshlets), std::end(tmpMeshlets));
			}
			else {
				for (const auto meshIndex : meshIndices)
				{
					auto vertices = ownedModel.get().positions_for_mesh(meshIndex);
					auto indices = ownedModel.get().indices_for_mesh<uint32_t>(meshIndex);
					// default to very bad meshlet generation
					auto tmpMeshlets = divide_into_very_bad_meshlets(vertices, indices, avk::owned(ownedModel), meshIndex, aMaxVertices, aMaxIndices);
					// append to meshlets
					meshlets.insert(std::end(meshlets), std::begin(tmpMeshlets), std::end(tmpMeshlets));
				}
			}
		}
		return meshlets;
	}

	std::vector<meshlet> divide_into_very_bad_meshlets(const std::vector<glm::vec3>& aPositions,
		const std::vector<uint32_t>& aIndices,
		avk::resource_ownership<model_t> aModel,
		const std::optional<mesh_index_t> aMeshIndex,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices)
	{
		std::vector<meshlet> veryBadMeshlets;

		int vertexIndex = 0;
		const int numIndices = static_cast<int>(aIndices.size());
		while (vertexIndex < numIndices) {
			auto& ml = veryBadMeshlets.emplace_back();
			ml.mVertexCount = 0u;
			ml.mVertices.reserve(aMaxVertices);
			ml.mIndices.reserve(aMaxIndices);
			int meshletVertexIndex = 0;
			// this simple algorithm duplicates vertices, therefore vertexCount == indexCount
			while (meshletVertexIndex < aMaxVertices - 3 && ml.mVertexCount < aMaxIndices - 3 && vertexIndex + meshletVertexIndex < numIndices) {
				ml.mIndices[meshletVertexIndex / 3 + 0] = meshletVertexIndex + 0;
				ml.mIndices[meshletVertexIndex / 3 + 1] = meshletVertexIndex + 1;
				ml.mIndices[meshletVertexIndex / 3 + 2] = meshletVertexIndex + 2;
				ml.mVertices[meshletVertexIndex + 0] = aPositions[aIndices[vertexIndex + meshletVertexIndex + 0]];
				ml.mVertices[meshletVertexIndex + 1] = aPositions[aIndices[vertexIndex + meshletVertexIndex + 1]];
				ml.mVertices[meshletVertexIndex + 2] = aPositions[aIndices[vertexIndex + meshletVertexIndex + 2]];
				ml.mVertexCount += 3u;

				meshletVertexIndex += 3;
			}
			ml.mIndexCount = ml.mVertexCount;
			// resize to actual size
			ml.mVertices.resize(ml.mVertexCount);
			ml.mVertices.shrink_to_fit();
			ml.mIndices.resize(ml.mVertexCount);
			ml.mIndices.shrink_to_fit();
			ml.mModel = aModel.own();
			ml.mMeshIndex = aMeshIndex;
			vertexIndex += meshletVertexIndex;
		}

		return veryBadMeshlets;
	}
}

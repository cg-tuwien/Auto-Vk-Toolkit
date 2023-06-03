

#include "meshlet_helpers.hpp"

namespace avk
{

	std::vector<meshlet> divide_into_meshlets(std::vector<std::tuple<avk::model, std::vector<mesh_index_t>>>& aModelsAndMeshletIndices,
		const bool aCombineSubmeshes, const uint32_t aMaxVertices, const uint32_t aMaxIndices)
	{
		return divide_into_meshlets(aModelsAndMeshletIndices, basic_meshlets_divider, aCombineSubmeshes, aMaxVertices, aMaxIndices);
	}

	std::vector<meshlet> basic_meshlets_divider(const std::vector<uint32_t>& aIndices,
		const model_t& aModel,
		std::optional<mesh_index_t> aMeshIndex,
		uint32_t aMaxVertices, uint32_t aMaxIndices)
	{
		std::vector<meshlet> result;

		int vertexIndex = 0;
		const int numIndices = static_cast<int>(aIndices.size());
		while (vertexIndex < numIndices) {
			auto& ml = result.emplace_back();
			ml.mVertexCount = 0u;
			ml.mVertices.resize(aMaxVertices);
			ml.mIndices.resize(aMaxIndices);
			int meshletVertexIndex = 0;
			// this simple algorithm duplicates vertices, therefore vertexCount == indexCount
			while (meshletVertexIndex < aMaxVertices - 3 && ml.mVertexCount < aMaxIndices - 3 && vertexIndex + meshletVertexIndex < numIndices) {
				ml.mIndices[meshletVertexIndex  + 0] = meshletVertexIndex + 0;
				ml.mIndices[meshletVertexIndex  + 1] = meshletVertexIndex + 1;
				ml.mIndices[meshletVertexIndex  + 2] = meshletVertexIndex + 2;
				ml.mVertices[meshletVertexIndex + 0] = aIndices[vertexIndex + meshletVertexIndex + 0];
				ml.mVertices[meshletVertexIndex + 1] = aIndices[vertexIndex + meshletVertexIndex + 1];
				ml.mVertices[meshletVertexIndex + 2] = aIndices[vertexIndex + meshletVertexIndex + 2];
				ml.mVertexCount += 3u;

				meshletVertexIndex += 3;
			}
			ml.mIndexCount = ml.mVertexCount;
			// resize to actual size
			ml.mVertices.resize(ml.mVertexCount);
			ml.mVertices.shrink_to_fit();
			ml.mIndices.resize(ml.mVertexCount);
			ml.mIndices.shrink_to_fit();
			ml.mMeshIndex = aMeshIndex;
			vertexIndex += meshletVertexIndex;
		}

		return result;
	}
}


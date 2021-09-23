#include <gvk.hpp>
#include "meshlet_helpers.hpp"

namespace gvk
{
	std::vector<meshlet> divide_into_meshlets(const std::vector<std::tuple<avk::resource_ownership<model_t>, std::vector<mesh_index_t>>>& aModels,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices, const bool aCombineSubmeshes)
	{
		return divide_into_meshlets(aModels, divide_into_very_bad_meshlets, aMaxVertices, aMaxIndices, aCombineSubmeshes);
	}

	std::vector<meshlet> divide_into_meshlets_cached(gvk::serializer& aSerializer,
		const std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<mesh_index_t>>>& aModels,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices, const bool aCombineSubmeshes)
	{
		return divide_into_meshlets_cached(aSerializer, aModels, divide_into_very_bad_meshlets, aMaxVertices, aMaxIndices, aCombineSubmeshes);
	}

	template <typename F>
	std::vector<meshlet> divide_into_meshlets(const std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<mesh_index_t>>>& aModels, F&& aMeshletDivision,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices, const bool aCombineSubmeshes)
	{
		std::vector<meshlet> meshlets;
		for (auto& pair : aModels) {
			auto model = std::get<avk::resource_ownership<model_t>>(pair);
			auto& meshIndices = std::get<std::vector<mesh_index_t>>(pair);

			if (aCombineSubmeshes) {
				auto [vertices, indices] = get_vertices_and_indices(std::vector({ std::make_tuple(avk::const_referenced(model.get()), meshIndices) }));
				// default to very bad meshlet generation
				std::vector<meshlet> tmpMeshlets = divide_vertices_into_meshlets(vertices, indices, model, std::nullopt, aMaxVertices, aMaxIndices, aMeshletDivision);
				// append to meshlets
				meshlets.insert(std::end(meshlets), std::begin(tmpMeshlets), std::end(tmpMeshlets));
			}
			else {
				for (const auto meshIndex : meshIndices)
				{
					auto vertices = model.get().positions_for_mesh(meshIndex);
					auto indices = model.get().indices_for_mesh<uint32_t>(meshIndex);
					// default to very bad meshlet generation
					std::vector<meshlet> tmpMeshlets = divide_vertices_into_meshlets(vertices, indices, model, meshIndex, aMaxVertices, aMaxIndices, aMeshletDivision);
					// append to meshlets
					meshlets.insert(std::end(meshlets), std::begin(tmpMeshlets), std::end(tmpMeshlets));
				}
			}
		}
		return meshlets;
	}

	template <typename F>
	std::vector<meshlet> divide_into_meshlets_cached(gvk::serializer& aSerializer,
		const std::vector<std::tuple<avk::resource_ownership<gvk::model_t>, std::vector<mesh_index_t>>>& aModels,
		F&& aMeshletDivision, const uint32_t aMaxVertices, const uint32_t aMaxIndices, const bool aCombineSubmeshes)
	{
		std::vector<meshlet> meshlets;
		if(aSerializer.mode() == gvk::serializer::mode::serialize)
		{
			meshlets = divide_into_meshlets(aModels, aMeshletDivision, aMaxVertices, aMaxIndices, aCombineSubmeshes);
		}
		aSerializer.archive(meshlets);

		if (aSerializer.mode() == serializer::mode::deserialize) {
			std::unordered_map<std::string, gvk::model> modelLookup;
			for(auto&  pair : aModels) {
				auto model = std::get<avk::resource_ownership<gvk::model_t>>(pair);
				modelLookup.insert_or_assign(model->path(), model.own());
			}
			for(auto& meshlet: meshlets)
			{
				meshlet.mModel = modelLookup.at(meshlet.mModelPath);
			}
		}

		return meshlets;
	}

	template <typename F>
	std::vector<meshlet> divide_vertices_into_meshlets(const std::vector<glm::vec3>& aVertices,
		const std::vector<uint32_t>& aIndices, avk::resource_ownership<gvk::model_t> aModel,
		const std::optional<mesh_index_t> aMeshIndex, const uint32_t aMaxVertices, const uint32_t aMaxIndices,
		F&& aMeshletDivision)
	{
		std::vector<meshlet> tmpMeshlets;
		if constexpr (std::is_assignable_v<std::function<std::vector<meshlet>(const std::vector<uint32_t>&tIndices, avk::resource_ownership<model_t> tModel, const std::optional<mesh_index_t> tMeshIndex, const uint32_t tMaxVertices, const uint32_t tMaxIndices)>, decltype(aMeshletDivision)>) {
			tmpMeshlets = aMeshletDivision(aIndices, aModel, aMeshIndex, aMaxVertices, aMaxIndices);
		}
		else if constexpr (std::is_assignable_v<std::function<std::vector<meshlet>(const std::vector<glm::vec3> &tVertices, const std::vector<uint32_t>&tIndices, avk::resource_ownership<model_t> tModel, const std::optional<mesh_index_t> tMeshIndex, const uint32_t tMaxVertices, const uint32_t tMaxIndices)>, decltype(aMeshletDivision)>) {
			tmpMeshlets = aMeshletDivision(aVertices, aIndices, aModel, aMeshIndex, aMaxVertices, aMaxIndices);
		}
		else {
#if defined(_MSC_VER) && defined(__cplusplus)
			static_assert(false);
#else
			assert(false);
#endif
			throw avk::logic_error("No lambda has been passed to divide_into_meshlets.");
		}
		return tmpMeshlets;
	}

	std::vector<meshlet> divide_into_very_bad_meshlets(const std::vector<uint32_t>& aIndices,
		avk::resource_ownership<model_t> aModel,
		const std::optional<mesh_index_t> aMeshIndex,
		const uint32_t aMaxVertices, const uint32_t aMaxIndices)
	{
		std::vector<meshlet> veryBadMeshlets;
		auto ownedModel = aModel.own();

		int vertexIndex = 0;
		const int numIndices = static_cast<int>(aIndices.size());
		while (vertexIndex < numIndices) {
			auto& ml = veryBadMeshlets.emplace_back();
			ml.mVertexCount = 0u;
			ml.mVertices.resize(aMaxVertices);
			ml.mIndices.resize(aMaxIndices);
			int meshletVertexIndex = 0;
			// this simple algorithm duplicates vertices, therefore vertexCount == indexCount
			while (meshletVertexIndex < aMaxVertices - 3 && ml.mVertexCount < aMaxIndices - 3 && vertexIndex + meshletVertexIndex < numIndices) {
				ml.mIndices[meshletVertexIndex / 3 + 0] = meshletVertexIndex + 0;
				ml.mIndices[meshletVertexIndex / 3 + 1] = meshletVertexIndex + 1;
				ml.mIndices[meshletVertexIndex / 3 + 2] = meshletVertexIndex + 2;
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
			ml.mModel = ownedModel;
			ml.mModelPath = ownedModel.get().path();
			ml.mMeshIndex = aMeshIndex;
			vertexIndex += meshletVertexIndex;
		}

		return veryBadMeshlets;
	}
}

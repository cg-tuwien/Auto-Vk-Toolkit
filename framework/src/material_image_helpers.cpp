#include <gvk.hpp>

namespace gvk
{
	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> positionsData;
		std::vector<uint32_t> indicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				gvk::append_indices_and_vertex_data(
					gvk::additional_index_data(	indicesData,	[&]() { return modelRef.get().indices_for_mesh<uint32_t>(meshIndex);	} ),
					gvk::additional_vertex_data(positionsData,	[&]() { return modelRef.get().positions_for_mesh(meshIndex);			} )
				);
			}
		}

		return std::make_tuple( std::move(positionsData), std::move(indicesData) );
	}

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> verticesAndIndices;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			verticesAndIndices = get_vertices_and_indices(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(verticesAndIndices);
		return verticesAndIndices;
	}

	std::vector<glm::vec3> get_normals(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(normalsData, modelRef.get().normals_for_mesh(meshIndex));
			}
		}

		return normalsData;
	}

	std::vector<glm::vec3> get_normals_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			normalsData = get_normals(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(normalsData);

		return normalsData;
	}
	
	std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(tangentsData, modelRef.get().tangents_for_mesh(meshIndex));
			}
		}

		return tangentsData;
	}

	std::vector<glm::vec3> get_tangents_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			tangentsData = get_tangents(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(tangentsData);

		return tangentsData;
	}
	

	std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(bitangentsData, modelRef.get().bitangents_for_mesh(meshIndex));
			}
		}

		return bitangentsData;
	}

	std::vector<glm::vec3> get_bitangents_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			bitangentsData = get_bitangents(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(bitangentsData);

		return bitangentsData;
	}

	std::vector<glm::vec4> get_colors(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(colorsData, modelRef.get().colors_for_mesh(meshIndex, aColorsSet));
			}
		}

		return colorsData;
	}

	std::vector<glm::vec4> get_colors_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			colorsData = get_colors(aModelsAndSelectedMeshes, aColorsSet);
		}
		aSerializer.archive(colorsData);

		return colorsData;
	}


	std::vector<glm::vec4> get_bone_weights(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights)
	{
		std::vector<glm::vec4> boneWeightsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(boneWeightsData, modelRef.get().bone_weights_for_mesh(meshIndex, aNormalizeBoneWeights));
			}
		}

		return boneWeightsData;
	}

	std::vector<glm::vec4> get_bone_weights_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights)
	{
		std::vector<glm::vec4> boneWeightsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights);
		}
		aSerializer.archive(boneWeightsData);

		return boneWeightsData;
	}

	std::vector<glm::uvec4> get_bone_indices(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(boneIndicesData, modelRef.get().bone_indices_for_mesh(meshIndex, aBoneIndexOffset));
			}
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			insert_into(boneIndicesData, modelRef.get().bone_indices_for_meshes_for_single_target_buffer(std::get<std::vector<mesh_index_t>>(pair), aInitialBoneIndexOffset));
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(boneIndicesData, modelRef.get().bone_indices_for_mesh_for_single_target_buffer(meshIndex, aReferenceMeshIndices));
			}
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec2>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_flipped(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec2>([](const glm::vec2& aValue){ return glm::vec2{aValue.x, 1.0f - aValue.y}; }, meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_flipped_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<avk::resource_reference<const gvk::model_t>>(pair);
			for (auto meshIndex : std::get<std::vector<mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.get().texture_coordinates_for_mesh<glm::vec3>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec3> get_3d_texture_coordinates_cached(gvk::serializer& aSerializer, const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	avk::sampler create_sampler_cached(gvk::serializer& aSerializer, avk::filter_mode aFilterMode, std::array<avk::border_handling_mode, 3> aBorderHandlingModes, float aMipMapMaxLod, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation)
	{
		aSerializer.archive(aFilterMode);
		aSerializer.archive(aBorderHandlingModes);
		aSerializer.archive(aMipMapMaxLod);
		return context().create_sampler(aFilterMode, aBorderHandlingModes, aMipMapMaxLod, std::move(aAlterConfigBeforeCreation));
	}

	avk::sampler create_sampler_cached(gvk::serializer& aSerializer, avk::filter_mode aFilterMode, std::array<avk::border_handling_mode, 2> aBorderHandlingModes, float aMipMapMaxLod, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation)
	{
		return create_sampler_cached(aSerializer, aFilterMode, { aBorderHandlingModes[0], aBorderHandlingModes[1], aBorderHandlingModes[1] }, aMipMapMaxLod, std::move(aAlterConfigBeforeCreation));
	}

	avk::sampler create_sampler_cached(gvk::serializer& aSerializer, avk::filter_mode aFilterMode, avk::border_handling_mode aBorderHandlingMode, float aMipMapMaxLod, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation)
	{
		return create_sampler_cached(aSerializer, aFilterMode, { aBorderHandlingMode, aBorderHandlingMode, aBorderHandlingMode }, aMipMapMaxLod, std::move(aAlterConfigBeforeCreation));
	}
}

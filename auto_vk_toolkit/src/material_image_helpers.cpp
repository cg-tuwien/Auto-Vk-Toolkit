
#include "image_data.hpp"
#include "material_image_helpers.hpp"
#include "serializer.hpp"

namespace avk
{
	std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_image_data_cached(image_data& aImageData, avk::layout::image_layout aImageLayout, avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, std::optional<std::reference_wrapper<avk::serializer>> aSerializer)
	{
		// image must have flag set to be used for cube map
		assert((static_cast<int>(aImageUsage) & static_cast<int>(avk::image_usage::cube_compatible)) > 0);

		return create_image_from_image_data_cached(aImageData, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_file_cached(const std::string& aPath, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip,
		int aPreferredNumberOfTextureComponents, avk::layout::image_layout aImageLayout, avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, std::optional<std::reference_wrapper<avk::serializer>> aSerializer)
	{
		auto cubemapImageData = get_image_data(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents);

		return create_cubemap_from_image_data_cached(cubemapImageData, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	std::tuple<avk::image, avk::command::action_type_command> create_cubemap_from_file_cached(const std::vector<std::string>& aPaths, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip,
		int aPreferredNumberOfTextureComponents, avk::layout::image_layout aImageLayout, avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, std::optional<std::reference_wrapper<avk::serializer>> aSerializer)
	{
		auto cubemapImageData = get_image_data(aPaths, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents);

		return create_cubemap_from_image_data_cached(cubemapImageData, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	std::tuple<avk::image, avk::command::action_type_command> create_image_from_image_data_cached(image_data& aImageData, avk::layout::image_layout aImageLayout, avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, std::optional<std::reference_wrapper<avk::serializer>> aSerializer)
	{
		using namespace avk;

		uint32_t width = 0;
		uint32_t height = 0;
		vk::Format format = vk::Format::eUndefined;
		uint32_t numLayers = 0;

		if (!aSerializer ||
			(aSerializer && aSerializer->get().mode() == avk::serializer::mode::serialize)) {

			// load the image to memory
			aImageData.load();

			assert(!aImageData.empty());

			if (aImageData.target() != vk::ImageType::e2D) {
				throw avk::runtime_error(std::format("The image loaded from '{}' is not intended to be used as 2D image. Can't load it.", aImageData.path()));
			}

			bool is_cube_compatible = (static_cast<int>(aImageUsage) & static_cast<int>(avk::image_usage::cube_compatible)) > 0;
			if (is_cube_compatible && aImageData.faces() != 6) {
				throw avk::runtime_error(std::format("The image loaded from '{}' is not intended to be used as a cube map image.", aImageData.path()));
			}

			width = aImageData.extent().width;
			height = aImageData.extent().height;

			format = aImageData.get_format();

			// number of layers in Vulkan image: equals (number of layers) x (number of faces) in image_data
			// a cube map image in Vulkan must have six layers, one for each side of the cube
			// TODO: support texture/cube map arrays
			numLayers = aImageData.faces();
		}

		if (aSerializer) {
			aSerializer->get().archive(width);
			aSerializer->get().archive(height);
			aSerializer->get().archive(format);
			aSerializer->get().archive(numLayers);
		}

		size_t maxLevels = 0;
		size_t maxFaces = 0;

		if (!aSerializer ||
			(aSerializer && aSerializer->get().mode() == avk::serializer::mode::serialize)) {

			// number of levels to load from image resource
			maxLevels = aImageData.levels();

			maxFaces = aImageData.faces();

			// if number of levels is 0, generate all mipmaps after loading
			if (maxLevels == 0)
			{
				maxLevels = 1;
			}

			assert(maxLevels >= 1);
		}

		if (aSerializer) {
			aSerializer->get().archive(maxLevels);
			aSerializer->get().archive(maxFaces);
		}
		
		// TODO: if image resource does not have a full mipmap pyramid, create image with fewer levels
		auto img = context().create_image(width, height, format, numLayers, aMemoryUsage, aImageUsage, [&](avk::image_t& image) {
			if (avk::is_block_compressed_format(format)) {
				// We don't create mip maps in the case of a compressed format. We simply assume the mip maps are contained
				// in the file and use these provided levels only.
				// TODO: this should probably be done in avk.cpp and not here?
				image.create_info().mipLevels = maxLevels;
			}
		});
		
		// TODO: handle the case where some but not all mipmap levels are loaded from image resource?
		assert(maxLevels == 1 || maxLevels == img->create_info().mipLevels);

		// 2. Copy buffer to image
		// Load all Mipmap levels from file, or load only the base level and generate other levels from that

		auto actionTypeCommand = avk::command::action_type_command{
			{},
			{ // Resource-specific sync-hints (will be accumulated later into mSyncHint):
				// No need for a sync hint for the staging buffer; it is in host-visible memory. Nothing must be waited on before we can start to fill it.
				std::make_tuple(img->handle(), avk::sync::sync_hint{ // But create a sync hint for the image:
					avk::stage::none     + avk::access::none,          // No need for any dependencies to previous commands. The host-visible data is made available on the queue submit.
					avk::stage::transfer + avk::access::transfer_write // Dependency chain with the last image_memory_barrier
				})
			}
		};

		// TODO: Do we have to account for gliTex.base_level() and gliTex.max_level()?
		for (uint32_t level = 0; level < maxLevels; ++level)
		{
			for (uint32_t face = 0; face < maxFaces; ++face)
			{
				size_t texSize = 0;
				void* texData = nullptr;
				avk::image_data::extent_type levelExtent;

				if (!aSerializer ||
					(aSerializer && aSerializer->get().mode() == avk::serializer::mode::serialize)) {
					texSize = aImageData.size(level);
					texData = aImageData.get_data(0, face, level);
					levelExtent = aImageData.extent(level);
				}
				if (aSerializer) {
					aSerializer->get().archive(texSize);
					aSerializer->get().archive(levelExtent);
				}
#ifdef _DEBUG
				{
					auto imgExtent = img->create_info().extent;
					auto levelDivisor = 1u << level;
					imgExtent.width = imgExtent.width > levelDivisor ? imgExtent.width / levelDivisor : 1u;
					imgExtent.height = imgExtent.height > levelDivisor ? imgExtent.height / levelDivisor : 1u;
					imgExtent.depth = imgExtent.depth > levelDivisor ? imgExtent.depth / levelDivisor : 1u;
					assert(levelExtent.width == static_cast<int>(imgExtent.width));
					assert(levelExtent.height == static_cast<int>(imgExtent.height));
					assert(levelExtent.depth == static_cast<int>(imgExtent.depth));
				}
#endif

				auto sb = context().create_buffer(
					AVK_STAGING_BUFFER_MEMORY_USAGE,
					vk::BufferUsageFlagBits::eTransferSrc,
					avk::generic_buffer_meta::create_from_size(texSize)
				);

				if (!aSerializer) {
					auto nop = sb->fill(texData, 0);
					assert(!nop.mBeginFun);
					assert(!nop.mEndFun);
				}
				else if (aSerializer && aSerializer->get().mode() == avk::serializer::mode::serialize) {
					auto nop = sb->fill(texData, 0);
					assert(!nop.mBeginFun);
					assert(!nop.mEndFun);
					aSerializer->get().archive_memory(texData, texSize);
				}
				else if (aSerializer && aSerializer->get().mode() == avk::serializer::mode::deserialize) {
					aSerializer->get().archive_buffer(*sb);
					LOG_INFO(std::format("Buffer of size {} loaded from cache", sb->meta_at_index<avk::buffer_meta>(0).total_size()));
				}

				// Memory writes are not overlapping => no barriers should be fine.
				actionTypeCommand.mNestedCommandsAndSyncInstructions.push_back(
					avk::sync::image_memory_barrier(*img,
						avk::stage::none  >> avk::stage::copy,
						avk::access::none >> avk::access::transfer_read | avk::access::transfer_write
					).with_layout_transition(avk::layout::undefined >> avk::layout::transfer_dst)
				);
				actionTypeCommand.mNestedCommandsAndSyncInstructions.push_back(
					avk::copy_buffer_to_image_layer_mip_level(
						sb, img.as_reference(),
						face, level,
						avk::layout::transfer_dst
					)
				);
				actionTypeCommand.mNestedCommandsAndSyncInstructions.push_back(
					avk::sync::image_memory_barrier(*img,
						avk::stage::copy            >> avk::stage::transfer,
						avk::access::transfer_write >> avk::access::none
					).with_layout_transition(avk::layout::transfer_dst >> aImageLayout)
				);
				// There should be no need to make any memory available or visible, the transfer-execution dependency chain should be fine

				actionTypeCommand.handle_lifetime_of(std::move(sb));
			}
		}
		
		if (maxLevels == 1 && img->create_info().mipLevels > 1)
		{
			// can't create MIP-maps for compressed formats
			assert(!avk::is_block_compressed_format(format));

			// For uncompressed formats, create MIP-maps via BLIT:
			actionTypeCommand.mNestedCommandsAndSyncInstructions.push_back(img->generate_mip_maps(aImageLayout >> aImageLayout)); // Keep the layout the same
		}

		actionTypeCommand.infer_sync_hint_from_resource_sync_hints();

		return std::make_tuple(std::move(img), std::move(actionTypeCommand));
	}

	std::tuple<avk::image, avk::command::action_type_command> create_image_from_file_cached(const std::string& aPath, bool aLoadHdrIfPossible, bool aLoadSrgbIfApplicable, bool aFlip, int aPreferredNumberOfTextureComponents, avk::layout::image_layout aImageLayout, avk::memory_usage aMemoryUsage, avk::image_usage aImageUsage, std::optional<std::reference_wrapper<avk::serializer>> aSerializer)
	{
		auto imageData = get_image_data(aPath, aLoadHdrIfPossible, aLoadSrgbIfApplicable, aFlip, aPreferredNumberOfTextureComponents);
		return avk::create_image_from_image_data_cached(imageData, aImageLayout, aMemoryUsage, aImageUsage, aSerializer);
	}

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> positionsData;
		std::vector<uint32_t> indicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				avk::append_indices_and_vertex_data(
					avk::additional_index_data(	indicesData,	[&]() { return modelRef.indices_for_mesh<uint32_t>(meshIndex);	} ),
					avk::additional_vertex_data(positionsData,	[&]() { return modelRef.positions_for_mesh(meshIndex);			} )
				);
			}
		}

		return std::make_tuple( std::move(positionsData), std::move(indicesData) );
	}

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> verticesAndIndices;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			verticesAndIndices = get_vertices_and_indices(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(verticesAndIndices);
		return verticesAndIndices;
	}

	std::vector<glm::vec3> get_normals(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(normalsData, modelRef.normals_for_mesh(meshIndex));
			}
		}

		return normalsData;
	}

	std::vector<glm::vec3> get_normals_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;
		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			normalsData = get_normals(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(normalsData);

		return normalsData;
	}

	std::vector<glm::vec3> get_tangents(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(tangentsData, modelRef.tangents_for_mesh(meshIndex));
			}
		}

		return tangentsData;
	}

	std::vector<glm::vec3> get_tangents_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			tangentsData = get_tangents(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(tangentsData);

		return tangentsData;
	}
	

	std::vector<glm::vec3> get_bitangents(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(bitangentsData, modelRef.bitangents_for_mesh(meshIndex));
			}
		}

		return bitangentsData;
	}

	std::vector<glm::vec3> get_bitangents_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			bitangentsData = get_bitangents(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(bitangentsData);

		return bitangentsData;
	}

	std::vector<glm::vec4> get_colors(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(colorsData, modelRef.colors_for_mesh(meshIndex, aColorsSet));
			}
		}

		return colorsData;
	}

	std::vector<glm::vec4> get_colors_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			colorsData = get_colors(aModelsAndSelectedMeshes, aColorsSet);
		}
		aSerializer.archive(colorsData);

		return colorsData;
	}


	std::vector<glm::vec4> get_bone_weights(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights)
	{
		std::vector<glm::vec4> boneWeightsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(boneWeightsData, modelRef.bone_weights_for_mesh(meshIndex, aNormalizeBoneWeights));
			}
		}

		return boneWeightsData;
	}

	std::vector<glm::vec4> get_bone_weights_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights)
	{
		std::vector<glm::vec4> boneWeightsData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights);
		}
		aSerializer.archive(boneWeightsData);

		return boneWeightsData;
	}

	std::vector<glm::uvec4> get_bone_indices(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(boneIndicesData, modelRef.bone_indices_for_mesh(meshIndex, aBoneIndexOffset));
			}
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			insert_into(boneIndicesData, modelRef.bone_indices_for_meshes_for_single_target_buffer(std::get<std::vector<avk::mesh_index_t>>(pair), aInitialBoneIndexOffset));
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices)
	{
		std::vector<glm::uvec4> boneIndicesData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(boneIndicesData, modelRef.bone_indices_for_mesh_for_single_target_buffer(meshIndex, aReferenceMeshIndices));
			}
		}

		return boneIndicesData;
	}

	std::vector<glm::uvec4> get_bone_indices_for_single_target_buffer_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.texture_coordinates_for_mesh<glm::vec2>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			texCoordsData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_flipped(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.texture_coordinates_for_mesh<glm::vec2>([](const glm::vec2& aValue){ return glm::vec2{aValue.x, 1.0f - aValue.y}; }, meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec2> get_2d_texture_coordinates_flipped_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			texCoordsData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	std::vector<glm::vec3> get_3d_texture_coordinates(const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		for (auto& pair : aModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const avk::model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<avk::mesh_index_t>>(pair)) {
				insert_into(texCoordsData, modelRef.texture_coordinates_for_mesh<glm::vec3>(meshIndex, aTexCoordSet));
			}
		}

		return texCoordsData;
	}

	std::vector<glm::vec3> get_3d_texture_coordinates_cached(avk::serializer& aSerializer, const std::vector<std::tuple<const avk::model_t&, std::vector<avk::mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		if (aSerializer.mode() == avk::serializer::mode::serialize) {
			texCoordsData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	avk::sampler create_sampler_cached(avk::serializer& aSerializer, avk::filter_mode aFilterMode, std::array<avk::border_handling_mode, 3> aBorderHandlingModes, float aMipMapMaxLod, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation)
	{
		aSerializer.archive(aFilterMode);
		aSerializer.archive(aBorderHandlingModes);
		aSerializer.archive(aMipMapMaxLod);
		return context().create_sampler(aFilterMode, aBorderHandlingModes, aMipMapMaxLod, std::move(aAlterConfigBeforeCreation));
	}
	
	avk::sampler create_sampler_cached(avk::serializer& aSerializer, avk::filter_mode aFilterMode, std::array<avk::border_handling_mode, 2> aBorderHandlingModes, float aMipMapMaxLod, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation)
	{
		return create_sampler_cached(aSerializer, aFilterMode, { aBorderHandlingModes[0], aBorderHandlingModes[1], aBorderHandlingModes[1] }, aMipMapMaxLod, std::move(aAlterConfigBeforeCreation));
	}

	avk::sampler create_sampler_cached(avk::serializer& aSerializer, avk::filter_mode aFilterMode, avk::border_handling_mode aBorderHandlingMode, float aMipMapMaxLod, std::function<void(avk::sampler_t&)> aAlterConfigBeforeCreation)
	{
		return create_sampler_cached(aSerializer, aFilterMode, { aBorderHandlingMode, aBorderHandlingMode, aBorderHandlingMode }, aMipMapMaxLod, std::move(aAlterConfigBeforeCreation));
		}
		}

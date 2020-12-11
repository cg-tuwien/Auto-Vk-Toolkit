#include <gvk.hpp>

namespace gvk
{
	std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage_cached(
		const std::vector<gvk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode,
		avk::border_handling_mode aBorderHandlingMode,
		avk::sync aSyncHandler,
		std::optional<gvk::serializer*> aSerializer = {})
	{
		// These are the texture names loaded from file -> mapped to vector of usage-pointers
		std::unordered_map<std::string, std::vector<int*>> texNamesToUsages;
		// Textures contained in this array shall be loaded into an sRGB format
		std::set<std::string> srgbTextures;

		// However, if some textures are missing, provide 1x1 px textures in those spots
		std::vector<int*> whiteTexUsages;				// Provide a 1x1 px almost everywhere in those cases,
		std::vector<int*> straightUpNormalTexUsages;	// except for normal maps, provide a normal pointing straight up there.

		std::vector<material_gpu_data> gpuMaterial;
		if (!aSerializer ||
			(aSerializer && (*aSerializer)->mode() == serializer::mode::serialize)) {
			size_t materialConfigSize = aMaterialConfigs.size();
			gpuMaterial.reserve(materialConfigSize); // important because of the pointers

			for (auto& mc : aMaterialConfigs) {
				auto& gm = gpuMaterial.emplace_back();
				gm.mDiffuseReflectivity = mc.mDiffuseReflectivity;
				gm.mAmbientReflectivity = mc.mAmbientReflectivity;
				gm.mSpecularReflectivity = mc.mSpecularReflectivity;
				gm.mEmissiveColor = mc.mEmissiveColor;
				gm.mTransparentColor = mc.mTransparentColor;
				gm.mReflectiveColor = mc.mReflectiveColor;
				gm.mAlbedo = mc.mAlbedo;

				gm.mOpacity = mc.mOpacity;
				gm.mBumpScaling = mc.mBumpScaling;
				gm.mShininess = mc.mShininess;
				gm.mShininessStrength = mc.mShininessStrength;

				gm.mRefractionIndex = mc.mRefractionIndex;
				gm.mReflectivity = mc.mReflectivity;
				gm.mMetallic = mc.mMetallic;
				gm.mSmoothness = mc.mSmoothness;

				gm.mSheen = mc.mSheen;
				gm.mThickness = mc.mThickness;
				gm.mRoughness = mc.mRoughness;
				gm.mAnisotropy = mc.mAnisotropy;

				gm.mAnisotropyRotation = mc.mAnisotropyRotation;
				gm.mCustomData = mc.mCustomData;

				gm.mDiffuseTexIndex = -1;
				if (mc.mDiffuseTex.empty()) {
					whiteTexUsages.push_back(&gm.mDiffuseTexIndex);
				}
				else {
					auto path = avk::clean_up_path(mc.mDiffuseTex);
					texNamesToUsages[path].push_back(&gm.mDiffuseTexIndex);
					if (aLoadTexturesInSrgb) {
						srgbTextures.insert(path);
					}
				}

				gm.mSpecularTexIndex = -1;
				if (mc.mSpecularTex.empty()) {
					whiteTexUsages.push_back(&gm.mSpecularTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mSpecularTex)].push_back(&gm.mSpecularTexIndex);
				}

				gm.mAmbientTexIndex = -1;
				if (mc.mAmbientTex.empty()) {
					whiteTexUsages.push_back(&gm.mAmbientTexIndex);
				}
				else {
					auto path = avk::clean_up_path(mc.mAmbientTex);
					texNamesToUsages[path].push_back(&gm.mAmbientTexIndex);
					if (aLoadTexturesInSrgb) {
						srgbTextures.insert(path);
					}
				}

				gm.mEmissiveTexIndex = -1;
				if (mc.mEmissiveTex.empty()) {
					whiteTexUsages.push_back(&gm.mEmissiveTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mEmissiveTex)].push_back(&gm.mEmissiveTexIndex);
				}

				gm.mHeightTexIndex = -1;
				if (mc.mHeightTex.empty()) {
					whiteTexUsages.push_back(&gm.mHeightTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mHeightTex)].push_back(&gm.mHeightTexIndex);
				}

				gm.mNormalsTexIndex = -1;
				if (mc.mNormalsTex.empty()) {
					straightUpNormalTexUsages.push_back(&gm.mNormalsTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mNormalsTex)].push_back(&gm.mNormalsTexIndex);
				}

				gm.mShininessTexIndex = -1;
				if (mc.mShininessTex.empty()) {
					whiteTexUsages.push_back(&gm.mShininessTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mShininessTex)].push_back(&gm.mShininessTexIndex);
				}

				gm.mOpacityTexIndex = -1;
				if (mc.mOpacityTex.empty()) {
					whiteTexUsages.push_back(&gm.mOpacityTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mOpacityTex)].push_back(&gm.mOpacityTexIndex);
				}

				gm.mDisplacementTexIndex = -1;
				if (mc.mDisplacementTex.empty()) {
					whiteTexUsages.push_back(&gm.mDisplacementTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mDisplacementTex)].push_back(&gm.mDisplacementTexIndex);
				}

				gm.mReflectionTexIndex = -1;
				if (mc.mReflectionTex.empty()) {
					whiteTexUsages.push_back(&gm.mReflectionTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mReflectionTex)].push_back(&gm.mReflectionTexIndex);
				}

				gm.mLightmapTexIndex = -1;
				if (mc.mLightmapTex.empty()) {
					whiteTexUsages.push_back(&gm.mLightmapTexIndex);
				}
				else {
					texNamesToUsages[avk::clean_up_path(mc.mLightmapTex)].push_back(&gm.mLightmapTexIndex);
				}

				gm.mExtraTexIndex = -1;
				if (mc.mExtraTex.empty()) {
					whiteTexUsages.push_back(&gm.mExtraTexIndex);
				}
				else {
					auto path = avk::clean_up_path(mc.mExtraTex);
					texNamesToUsages[path].push_back(&gm.mExtraTexIndex);
					if (aLoadTexturesInSrgb) {
						srgbTextures.insert(path);
					}
				}

				gm.mDiffuseTexOffsetTiling = mc.mDiffuseTexOffsetTiling;
				gm.mSpecularTexOffsetTiling = mc.mSpecularTexOffsetTiling;
				gm.mAmbientTexOffsetTiling = mc.mAmbientTexOffsetTiling;
				gm.mEmissiveTexOffsetTiling = mc.mEmissiveTexOffsetTiling;
				gm.mHeightTexOffsetTiling = mc.mHeightTexOffsetTiling;
				gm.mNormalsTexOffsetTiling = mc.mNormalsTexOffsetTiling;
				gm.mShininessTexOffsetTiling = mc.mShininessTexOffsetTiling;
				gm.mOpacityTexOffsetTiling = mc.mOpacityTexOffsetTiling;
				gm.mDisplacementTexOffsetTiling = mc.mDisplacementTexOffsetTiling;
				gm.mReflectionTexOffsetTiling = mc.mReflectionTexOffsetTiling;
				gm.mLightmapTexOffsetTiling = mc.mLightmapTexOffsetTiling;
				gm.mExtraTexOffsetTiling = mc.mExtraTexOffsetTiling;
			}
		}

		std::vector<avk::image_sampler> imageSamplers;

		size_t numTexUsages = texNamesToUsages.size();
		size_t numWhiteTexUsages = whiteTexUsages.empty() ? 0 : 1;
		size_t numStraightUpNormalTexUsages = (straightUpNormalTexUsages.empty() ? 0 : 1);

		if (aSerializer) {
			(*aSerializer)->archive(numTexUsages);
			(*aSerializer)->archive(numWhiteTexUsages);
			(*aSerializer)->archive(numStraightUpNormalTexUsages);
		}

		const auto numSamplers = numTexUsages + numWhiteTexUsages + numStraightUpNormalTexUsages;
		imageSamplers.reserve(numSamplers);

		auto getSync = [numSamplers, &aSyncHandler, lSyncCount = size_t{ 0 }]() mutable->avk::sync {
			++lSyncCount;
			if (lSyncCount < numSamplers) {
				return avk::sync::auxiliary_with_barriers(aSyncHandler, avk::sync::steal_before_handler_on_demand, {}); // Invoke external sync exactly once (if there is something to sync)
			}
			assert(lSyncCount == numSamplers);
			return std::move(aSyncHandler); // For the last image, pass the main sync => this will also have the after-handler invoked.
		};

		// Create the white texture and assign its index to all usages
		if (numWhiteTexUsages > 0) {
			// Dont need to check if we have a serializer since create_1px_texture_cached takes it as an optional
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_1px_texture_cached({ 255, 255, 255, 255 }, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, getSync(), aSerializer))
					)),
					owned(context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat))
				)
			);
			if (!aSerializer ||
				(aSerializer && (*aSerializer)->mode() == serializer::mode::serialize)) {
				int index = static_cast<int>(imageSamplers.size() - 1);
				for (auto* img : whiteTexUsages) {
					*img = index;
				}
			}
		}

		// Create the normal texture, containing a normal pointing straight up, and assign to all usages
		if (numStraightUpNormalTexUsages > 0) {
			// Dont need to check if we have a serializer since create_1px_texture_cached takes it as an optional
			imageSamplers.push_back(
				context().create_image_sampler(
					owned(context().create_image_view(
						owned(create_1px_texture_cached({ 255, 255, 255, 255 }, vk::Format::eR8G8B8A8Unorm, avk::memory_usage::device, aImageUsage, getSync(), aSerializer))
					)),
					owned(context().create_sampler(avk::filter_mode::nearest_neighbor, avk::border_handling_mode::repeat))
				)
			);
			if (!aSerializer ||
				(aSerializer && (*aSerializer)->mode() == serializer::mode::serialize)) {
				int index = static_cast<int>(imageSamplers.size() - 1);
				for (auto* img : straightUpNormalTexUsages) {
					*img = index;
				}
			}
		}

		// Load all the images from file, and assign them to all usages
		if (!aSerializer ||
			(aSerializer && (*aSerializer)->mode() == serializer::mode::serialize)) {
			// create_image_from_file_cached takes the serializer as an optional,
			// therefore the call is safe with and without one
			for (auto& pair : texNamesToUsages) {
				assert(!pair.first.empty());

				bool potentiallySrgb = srgbTextures.contains(pair.first);

				imageSamplers.push_back(
					context().create_image_sampler(
						owned(context().create_image_view(
							create_image_from_file_cached(pair.first, true, potentiallySrgb, aFlipTextures, 4, avk::memory_usage::device, aImageUsage, getSync(), aSerializer)
						)),
						owned(context().create_sampler(aTextureFilterMode, aBorderHandlingMode))
					)
				);
				int index = static_cast<int>(imageSamplers.size() - 1);
				for (auto* img : pair.second) {
					*img = index;
				}
			}
		}
		else {
			// We sure have the serializer here
			for (int i = 0; i < numTexUsages; ++i) {
				// create_image_from_file_cached does not need these values, as the
				// image data is loaded from a cache file, so we can just pass some
				// defaults to avoid having to save them to the cache file
				const bool potentiallySrgbDontCare = false;
				const std::string pathDontCare = "";

				imageSamplers.push_back(
					context().create_image_sampler(
						owned(context().create_image_view(
							create_image_from_file_cached(pathDontCare, true, potentiallySrgbDontCare, aFlipTextures, 4, avk::memory_usage::device, aImageUsage, getSync(), aSerializer)
						)),
						owned(context().create_sampler(aTextureFilterMode, aBorderHandlingMode))
					)
				);
			}
		}

		if (aSerializer) {
			(*aSerializer)->archive(gpuMaterial);
		}

		// Hand over ownership to the caller
		return std::make_tuple(std::move(gpuMaterial), std::move(imageSamplers));
	}

	std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage(
		const std::vector<gvk::material_config>& aMaterialConfigs, 
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode, 
		avk::border_handling_mode aBorderHandlingMode,
		avk::sync aSyncHandler)
	{
		return convert_for_gpu_usage_cached(
			aMaterialConfigs,
			aLoadTexturesInSrgb,
			aFlipTextures,
			aImageUsage,
			aTextureFilterMode,
			aBorderHandlingMode,
			std::move(aSyncHandler));
	}

	std::tuple<std::vector<material_gpu_data>, std::vector<avk::image_sampler>> convert_for_gpu_usage_cached(
		const std::vector<gvk::material_config>& aMaterialConfigs,
		bool aLoadTexturesInSrgb,
		bool aFlipTextures,
		avk::image_usage aImageUsage,
		avk::filter_mode aTextureFilterMode,
		avk::border_handling_mode aBorderHandlingMode,
		avk::sync aSyncHandler,
		gvk::serializer& aSerializer)
	{
		return convert_for_gpu_usage_cached(
			aMaterialConfigs,
			aLoadTexturesInSrgb,
			aFlipTextures,
			aImageUsage,
			aTextureFilterMode,
			aBorderHandlingMode,
			std::move(aSyncHandler),
			std::make_optional<gvk::serializer*>(&aSerializer));
	}

	void fill_device_buffer_from_cache_file(avk::buffer& aDeviceBuffer, size_t aTotalSize, avk::sync& aSyncHandler, gvk::serializer& aSerializer)
	{
		// Create host visible staging buffer for filling on host side from file
		auto sb = context().create_buffer(
			AVK_STAGING_BUFFER_MEMORY_USAGE,
			vk::BufferUsageFlagBits::eTransferSrc,
			avk::generic_buffer_meta::create_from_size(aTotalSize)
		);
		// Map buffer and let serializer fill it
		// To unmap the buffer after filling for
		// device buffer transfer, mapping has to
		// go out of scope
		aSerializer.archive_buffer(sb);

		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before
		aSyncHandler.establish_barrier_before_the_operation(avk::pipeline_stage::transfer, avk::read_memory_access{ avk::memory_access::transfer_read_access });

		// Copy host visible staging buffer to device buffer
		avk::copy_buffer_to_another(avk::referenced(sb), avk::referenced(aDeviceBuffer), 0, 0, aTotalSize, avk::sync::with_barriers_into_existing_command_buffer(commandBuffer, {}, {}));

		// Sync after
		aSyncHandler.establish_barrier_after_the_operation(avk::pipeline_stage::transfer, avk::write_memory_access{ avk::memory_access::transfer_write_access });

		// Take care of the lifetime handling of the stagingBuffer, it might still be in use
		commandBuffer.set_custom_deleter([
			lOwnedStagingBuffer{ std::move(sb) }
		]() { /* Nothing to do here, the buffers' destructors will do the cleanup, the lambda is just storing it. */ });

		// Finish him
		aSyncHandler.submit_and_sync();
	}

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

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_vertices_and_indices_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, gvk::serializer& aSerializer)
	{
		std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> verticesAndIndices;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			verticesAndIndices = get_vertices_and_indices(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(verticesAndIndices);
		return verticesAndIndices;
	}

	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers(const std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>>& aVerticesAndIndices, vk::BufferUsageFlags aUsageFlags, avk::sync aSyncHandler)
	{
		auto [positionsData, indicesData] = aVerticesAndIndices;

		auto positionsBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::vertex_buffer_meta::create_from_data(positionsData)
				.describe_only_member(positionsData[0], avk::content_description::position)
		);
		positionsBuffer->fill(positionsData.data(), 0, avk::sync::auxiliary_with_barriers(aSyncHandler, avk::sync::steal_before_handler_on_demand, {}));
		// It is fine to let positionsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		auto indexBuffer = context().create_buffer(
			avk::memory_usage::device, aUsageFlags,
			avk::index_buffer_meta::create_from_data(indicesData)
		);
		indexBuffer->fill(indicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let indicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
	}
	
	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags, avk::sync aSyncHandler)
	{
		return create_vertex_and_index_buffers(get_vertices_and_indices(aModelsAndSelectedMeshes),
			aUsageFlags, std::move(aSyncHandler));
	}

	std::tuple<avk::buffer, avk::buffer> create_vertex_and_index_buffers_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, vk::BufferUsageFlags aUsageFlags, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numPositions = 0;
		size_t totalPositionsSize = 0;
		size_t numIndices = 0;
		size_t totalIndicesSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto [positionsData, indicesData] = get_vertices_and_indices(aModelsAndSelectedMeshes);

			numPositions = positionsData.size();
			totalPositionsSize = sizeof(positionsData[0]) * numPositions;
			numIndices = indicesData.size();
			totalIndicesSize = sizeof(indicesData[0]) * numIndices;

			aSerializer.archive(numPositions);
			aSerializer.archive(totalPositionsSize);
			aSerializer.archive(numIndices);
			aSerializer.archive(totalIndicesSize);

			aSerializer.archive_memory(positionsData.data(), totalPositionsSize);
			aSerializer.archive_memory(indicesData.data(), totalIndicesSize);

			return create_vertex_and_index_buffers(std::make_tuple(positionsData, indicesData), aUsageFlags, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numPositions);
			aSerializer.archive(totalPositionsSize);
			aSerializer.archive(numIndices);
			aSerializer.archive(totalIndicesSize);

			auto positionsBuffer = context().create_buffer(
				avk::memory_usage::device, aUsageFlags,
				avk::vertex_buffer_meta::create_from_total_size(totalPositionsSize, numPositions)
				.describe_member(0, avk::format_for<glm::vec3>(), avk::content_description::position)
			);

			fill_device_buffer_from_cache_file(positionsBuffer, totalPositionsSize, aSyncHandler, aSerializer);

			auto indexBuffer = context().create_buffer(
				avk::memory_usage::device, aUsageFlags,
				avk::index_buffer_meta::create_from_total_size(totalIndicesSize, numIndices)
			);

			fill_device_buffer_from_cache_file(indexBuffer, totalIndicesSize, aSyncHandler, aSerializer);

			return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
		}
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

	std::vector<glm::vec3> get_normals_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, gvk::serializer& aSerializer)
	{
		std::vector<glm::vec3> normalsData;
		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			normalsData = get_normals(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(normalsData);

		return normalsData;
	}

	avk::buffer create_normals_buffer(const std::vector<glm::vec3>& aNormalsData, avk::sync aSyncHandler)
	{
		auto normalsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aNormalsData)
		);
		normalsBuffer->fill(aNormalsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let normalsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return normalsBuffer;
	}
	
	avk::buffer create_normals_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		return create_normals_buffer(get_normals(aModelsAndSelectedMeshes), std::move(aSyncHandler));
	}

	avk::buffer create_normals_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numNormals = 0;
		size_t totalNormalsSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto normalsData = get_normals(aModelsAndSelectedMeshes);

			numNormals = normalsData.size();
			totalNormalsSize = sizeof(normalsData[0]) * numNormals;

			aSerializer.archive(numNormals);
			aSerializer.archive(totalNormalsSize);

			aSerializer.archive_memory(normalsData.data(), totalNormalsSize);

			return create_normals_buffer(normalsData, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numNormals);
			aSerializer.archive(totalNormalsSize);

			auto normalsBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalNormalsSize, numNormals)
			);

			fill_device_buffer_from_cache_file(normalsBuffer, totalNormalsSize, aSyncHandler, aSerializer);

			return normalsBuffer;
		}
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

	std::vector<glm::vec3> get_tangents_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, gvk::serializer& aSerializer)
	{
		std::vector<glm::vec3> tangentsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			tangentsData = get_tangents(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(tangentsData);

		return tangentsData;
	}
	
	avk::buffer create_tangents_buffer(const std::vector<glm::vec3>& aTangentsData, avk::sync aSyncHandler)
	{
		auto tangentsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aTangentsData)
		);
		tangentsBuffer->fill(aTangentsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let tangentsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return tangentsBuffer;
	}

	avk::buffer create_tangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		return create_tangents_buffer(get_tangents(aModelsAndSelectedMeshes), std::move(aSyncHandler));
	}

	avk::buffer create_tangents_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numTangets = 0;
		size_t totalTangentsSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto tangentsData = get_tangents(aModelsAndSelectedMeshes);

			numTangets = tangentsData.size();
			totalTangentsSize = sizeof(tangentsData[0]) * numTangets;

			aSerializer.archive(numTangets);
			aSerializer.archive(totalTangentsSize);

			aSerializer.archive_memory(tangentsData.data(), totalTangentsSize);

			return create_tangents_buffer(tangentsData, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numTangets);
			aSerializer.archive(totalTangentsSize);

			auto tangentsBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalTangentsSize, numTangets)
			);

			fill_device_buffer_from_cache_file(tangentsBuffer, totalTangentsSize, aSyncHandler, aSerializer);

			return tangentsBuffer;
		}
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

	std::vector<glm::vec3> get_bitangents_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, gvk::serializer& aSerializer)
	{
		std::vector<glm::vec3> bitangentsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			bitangentsData = get_bitangents(aModelsAndSelectedMeshes);
		}
		aSerializer.archive(bitangentsData);

		return bitangentsData;
	}

	avk::buffer create_bitangents_buffer(const std::vector<glm::vec3>& aBitangentsData, avk::sync aSyncHandler)
	{
		auto bitangentsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aBitangentsData)
		);
		bitangentsBuffer->fill(aBitangentsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let bitangentsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return bitangentsBuffer;
	}
	
	avk::buffer create_bitangents_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		return create_bitangents_buffer(get_bitangents(aModelsAndSelectedMeshes), std::move(aSyncHandler));
	}

	avk::buffer create_bitangents_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numBitangents = 0;
		size_t totalBitangentsSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto tangentsData = get_bitangents(aModelsAndSelectedMeshes);

			numBitangents = tangentsData.size();
			totalBitangentsSize = sizeof(tangentsData[0]) * numBitangents;

			aSerializer.archive(numBitangents);
			aSerializer.archive(totalBitangentsSize);

			aSerializer.archive_memory(tangentsData.data(), totalBitangentsSize);

			return create_bitangents_buffer(tangentsData, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numBitangents);
			aSerializer.archive(totalBitangentsSize);

			auto bitangentsBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalBitangentsSize, numBitangents)
			);

			fill_device_buffer_from_cache_file(bitangentsBuffer, totalBitangentsSize, aSyncHandler, aSerializer);

			return bitangentsBuffer;
		}
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

	std::vector<glm::vec4> get_colors_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet, gvk::serializer& aSerializer)
	{
		std::vector<glm::vec4> colorsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			colorsData = get_colors(aModelsAndSelectedMeshes, aColorsSet);
		}
		aSerializer.archive(colorsData);

		return colorsData;
	}

	avk::buffer create_colors_buffer(const std::vector<glm::vec4>& aColorsData, int aColorsSet, avk::sync aSyncHandler)
	{
		auto colorsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aColorsData)
		);
		colorsBuffer->fill(aColorsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let colorsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return colorsBuffer;
	}
	
	avk::buffer create_colors_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet, avk::sync aSyncHandler)
	{
		return create_colors_buffer(get_colors(aModelsAndSelectedMeshes, aColorsSet), aColorsSet, std::move(aSyncHandler));
	}

	avk::buffer create_colors_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aColorsSet, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numColors = 0;
		size_t totalColorsSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto colorsData = get_colors_cached(aModelsAndSelectedMeshes, aColorsSet, aSerializer);

			numColors = colorsData.size();
			totalColorsSize = sizeof(colorsData[0]) * numColors;

			aSerializer.archive(numColors);
			aSerializer.archive(totalColorsSize);

			aSerializer.archive_memory(colorsData.data(), totalColorsSize);

			return create_colors_buffer(colorsData, aColorsSet, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numColors);
			aSerializer.archive(totalColorsSize);

			auto colorsBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalColorsSize, numColors)
			);

			fill_device_buffer_from_cache_file(colorsBuffer, totalColorsSize, aSyncHandler, aSerializer);

			return colorsBuffer;
		}
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

	std::vector<glm::vec4> get_bone_weights_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights, gvk::serializer& aSerializer)
	{
		std::vector<glm::vec4> boneWeightsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights);
		}
		aSerializer.archive(boneWeightsData);

		return boneWeightsData;
	}

	avk::buffer create_bone_weights_buffer(const std::vector<glm::vec4>& aBoneWeightsData, avk::sync aSyncHandler)
	{
		auto boneWeightsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aBoneWeightsData)
		);
		boneWeightsBuffer->fill(aBoneWeightsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneWeightsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneWeightsBuffer;
	}
	
	avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights, avk::sync aSyncHandler)
	{
		return create_bone_weights_buffer(get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights), std::move(aSyncHandler));
	}

	avk::buffer create_bone_weights_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler)
	{
		return create_bone_weights_buffer(aModelsAndSelectedMeshes, false, std::move(aSyncHandler));
	}

	avk::buffer create_bone_weights_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, bool aNormalizeBoneWeights, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numBoneWeights = 0;
		size_t totalBoneWeightsSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto boneWeightsData = get_bone_weights(aModelsAndSelectedMeshes, aNormalizeBoneWeights);

			numBoneWeights = boneWeightsData.size();
			totalBoneWeightsSize = sizeof(boneWeightsData[0]) * numBoneWeights;

			aSerializer.archive(numBoneWeights);
			aSerializer.archive(totalBoneWeightsSize);

			aSerializer.archive_memory(boneWeightsData.data(), totalBoneWeightsSize);

			return create_bone_weights_buffer(boneWeightsData, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numBoneWeights);
			aSerializer.archive(totalBoneWeightsSize);

			auto boneWeightsBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalBoneWeightsSize, numBoneWeights)
			);

			fill_device_buffer_from_cache_file(boneWeightsBuffer, totalBoneWeightsSize, aSyncHandler, aSerializer);

			return boneWeightsBuffer;
		}
	}

	avk::buffer create_bone_weights_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		return create_bone_weights_buffer(get_bone_weights_cached(aModelsAndSelectedMeshes, false, aSerializer), std::move(aSyncHandler));
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

	std::vector<glm::uvec4> get_bone_indices_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset, gvk::serializer& aSerializer)
	{
		std::vector<glm::uvec4> boneIndicesData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset);
		}
		aSerializer.archive(boneIndicesData);

		return boneIndicesData;
	}

	avk::buffer create_bone_indices_buffer(const std::vector<glm::uvec4>& aBoneIndicesData, avk::sync aSyncHandler)
	{
		auto boneIndicesBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aBoneIndicesData)
		);
		boneIndicesBuffer->fill(aBoneIndicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneIndicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneIndicesBuffer;
	}
	
	avk::buffer create_bone_indices_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset, avk::sync aSyncHandler)
	{
		return create_bone_indices_buffer(get_bone_indices(aModelsAndSelectedMeshes, aBoneIndexOffset), std::move(aSyncHandler));
	}

	avk::buffer create_bone_indices_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aBoneIndexOffset, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numBoneIndices = 0;
		size_t totalBoneIndicesSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto boneIndicesData = get_bone_indices(aModelsAndSelectedMeshes);

			numBoneIndices = boneIndicesData.size();
			totalBoneIndicesSize = sizeof(boneIndicesData[0]) * numBoneIndices;

			aSerializer.archive(numBoneIndices);
			aSerializer.archive(totalBoneIndicesSize);

			aSerializer.archive_memory(boneIndicesData.data(), totalBoneIndicesSize);

			return create_bone_indices_buffer(boneIndicesData, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numBoneIndices);
			aSerializer.archive(totalBoneIndicesSize);

			auto boneIndicesBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalBoneIndicesSize, numBoneIndices)
			);

			fill_device_buffer_from_cache_file(boneIndicesBuffer, totalBoneIndicesSize, aSyncHandler, aSerializer);

			return boneIndicesBuffer;
		}
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

	avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, uint32_t aInitialBoneIndexOffset, avk::sync aSyncHandler)
	{
		auto boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aInitialBoneIndexOffset);

		auto boneIndicesBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(boneIndicesData)
		);
		boneIndicesBuffer->fill(boneIndicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneIndicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneIndicesBuffer;
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

	avk::buffer create_bone_indices_for_single_target_buffer_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, const std::vector<mesh_index_t>& aReferenceMeshIndices, avk::sync aSyncHandler)
	{
		auto boneIndicesData = get_bone_indices_for_single_target_buffer(aModelsAndSelectedMeshes, aReferenceMeshIndices);

		auto boneIndicesBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(boneIndicesData)
		);
		boneIndicesBuffer->fill(boneIndicesData.data(), 0, std::move(aSyncHandler));
		// It is fine to let boneIndicesData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return boneIndicesBuffer;
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

	std::vector<glm::vec2> get_2d_texture_coordinates_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, gvk::serializer& aSerializer)
	{
		std::vector<glm::vec2> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	avk::buffer create_2d_texture_coordinates_buffer(const std::vector<glm::vec2>& aTexCoordsData, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aTexCoordsData)
		);
		texCoordsBuffer->fill(aTexCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
	}
	
	avk::buffer create_2d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		return create_2d_texture_coordinates_buffer(get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet), aTexCoordSet, std::move(aSyncHandler));
	}

	avk::buffer create_2d_texture_coordinates_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numTexCoords = 0;
		size_t totalTexCoordsSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto texCoordsData = get_2d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);

			numTexCoords = texCoordsData.size();
			totalTexCoordsSize = sizeof(texCoordsData[0]) * numTexCoords;

			aSerializer.archive(numTexCoords);
			aSerializer.archive(totalTexCoordsSize);

			aSerializer.archive_memory(texCoordsData.data(), totalTexCoordsSize);

			return create_2d_texture_coordinates_buffer(texCoordsData, aTexCoordSet, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numTexCoords);
			aSerializer.archive(totalTexCoordsSize);

			auto texCoordsBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalTexCoordsSize, numTexCoords)
			);

			fill_device_buffer_from_cache_file(texCoordsBuffer, totalTexCoordsSize, aSyncHandler, aSerializer);

			return texCoordsBuffer;
		}
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

	std::vector<glm::vec2> get_2d_texture_coordinates_flipped_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, gvk::serializer& aSerializer)
	{
		std::vector<glm::vec2> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	avk::buffer create_2d_texture_coordinates_flipped_buffer(const std::vector<glm::vec2>& aTexCoordsData, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aTexCoordsData)
		);
		texCoordsBuffer->fill(aTexCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
	}
	
	avk::buffer create_2d_texture_coordinates_flipped_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		return create_2d_texture_coordinates_flipped_buffer(get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet), aTexCoordSet, std::move(aSyncHandler));
	}

	avk::buffer create_2d_texture_coordinates_flipped_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numTexCoords = 0;
		size_t totalTexCoordsSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto texCoordsData = get_2d_texture_coordinates_flipped(aModelsAndSelectedMeshes, aTexCoordSet);

			numTexCoords = texCoordsData.size();
			totalTexCoordsSize = sizeof(texCoordsData[0]) * numTexCoords;

			aSerializer.archive(numTexCoords);
			aSerializer.archive(totalTexCoordsSize);

			aSerializer.archive_memory(texCoordsData.data(), totalTexCoordsSize);

			return create_2d_texture_coordinates_flipped_buffer(texCoordsData, aTexCoordSet, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numTexCoords);
			aSerializer.archive(totalTexCoordsSize);

			auto texCoordsBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalTexCoordsSize, numTexCoords)
			);

			fill_device_buffer_from_cache_file(texCoordsBuffer, totalTexCoordsSize, aSyncHandler, aSerializer);

			return texCoordsBuffer;
		}
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

	std::vector<glm::vec3> get_3d_texture_coordinates_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, gvk::serializer& aSerializer)
	{
		std::vector<glm::vec3> texCoordsData;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			texCoordsData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);
		}
		aSerializer.archive(texCoordsData);

		return texCoordsData;
	}

	avk::buffer create_3d_texture_coordinates_buffer(const std::vector<glm::vec3>& aTexCoordsData, int aTexCoordSet, avk::sync aSyncHandler)
	{
		auto texCoordsBuffer = context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(aTexCoordsData)
		);
		texCoordsBuffer->fill(aTexCoordsData.data(), 0, std::move(aSyncHandler));
		// It is fine to let texCoordsData go out of scope, since its data has been copied to a
		// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.

		return texCoordsBuffer;
	}
	
	avk::buffer create_3d_texture_coordinates_buffer(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler)
	{
		return create_3d_texture_coordinates_buffer(get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet), aTexCoordSet, std::move(aSyncHandler));
	}

	avk::buffer create_3d_texture_coordinates_buffer_cached(const std::vector<std::tuple<avk::resource_reference<const gvk::model_t>, std::vector<mesh_index_t>>>& aModelsAndSelectedMeshes, int aTexCoordSet, avk::sync aSyncHandler, gvk::serializer& aSerializer)
	{
		size_t numTexCoords = 0;
		size_t totalTexCoordsSize = 0;

		if (aSerializer.mode() == gvk::serializer::mode::serialize) {
			auto texCoordsData = get_3d_texture_coordinates(aModelsAndSelectedMeshes, aTexCoordSet);

			numTexCoords = texCoordsData.size();
			totalTexCoordsSize = sizeof(texCoordsData[0]) * numTexCoords;

			aSerializer.archive(numTexCoords);
			aSerializer.archive(totalTexCoordsSize);

			aSerializer.archive_memory(texCoordsData.data(), totalTexCoordsSize);

			return create_3d_texture_coordinates_buffer(texCoordsData, aTexCoordSet, std::move(aSyncHandler));
		}
		else {
			aSerializer.archive(numTexCoords);
			aSerializer.archive(totalTexCoordsSize);

			auto texCoordsBuffer = context().create_buffer(
				avk::memory_usage::device, {},
				avk::vertex_buffer_meta::create_from_total_size(totalTexCoordsSize, numTexCoords)
			);

			fill_device_buffer_from_cache_file(texCoordsBuffer, totalTexCoordsSize, aSyncHandler, aSerializer);

			return texCoordsBuffer;
		}
	}

}

#include <cg_base.hpp>

namespace cgb
{
	std::optional<command_buffer> copy_image_to_another(image_t& aSrcImage, image_t& aDstImage, sync aSyncHandler, bool aRestoreSrcLayout, bool aRestoreDstLayout)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		// Citing the specs: "srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		auto initialSrcLayout = aSrcImage.target_layout();
		const bool suitableSrcLayout = initialSrcLayout == vk::ImageLayout::eTransferSrcOptimal; // For optimal performance, only allow eTransferSrcOptimal
									//|| initialSrcLayout == vk::ImageLayout::eGeneral
									//|| initialSrcLayout == vk::ImageLayout::eSharedPresentKHR;
		if (suitableSrcLayout) {
			// Just make sure that is really is in target layout:
			aSrcImage.transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {})); 
		}
		else {
			// Not a suitable src layout => must transform
			aSrcImage.transition_to_layout(vk::ImageLayout::eTransferSrcOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		// Citing the specs: "dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		auto initialDstLayout = aDstImage.target_layout();
		const bool suitableDstLayout = initialDstLayout == vk::ImageLayout::eTransferDstOptimal
									|| initialDstLayout == vk::ImageLayout::eGeneral
									|| initialDstLayout == vk::ImageLayout::eSharedPresentKHR;
		if (suitableDstLayout) {
			// Just make sure that is really is in target layout:
			aDstImage.transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {})); 
		}
		else {
			// Not a suitable dst layout => must transform
			aDstImage.transition_to_layout(vk::ImageLayout::eTransferDstOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}
		
		// Operation:
		auto copyRegion = vk::ImageCopy{}
			.setExtent(aSrcImage.config().extent) // TODO: Support different ranges/extents
			.setSrcOffset({0, 0})
			.setSrcSubresource(vk::ImageSubresourceLayers{} // TODO: Add support for the other parameters
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0u)
				.setLayerCount(1u)
				.setMipLevel(0u)
			)
			.setDstOffset({0, 0})
			.setDstSubresource(vk::ImageSubresourceLayers{} // TODO: Add support for the other parameters
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0u)
				.setLayerCount(1u)
				.setMipLevel(0u));

		commandBuffer.handle().copyImage(
			aSrcImage.handle(),
			aSrcImage.current_layout(),
			aDstImage.handle(),
			aDstImage.current_layout(),
			1u, &copyRegion);

		if (!suitableSrcLayout && aRestoreSrcLayout) { // => restore original layout of the src image
			aSrcImage.transition_to_layout(initialSrcLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		if (!suitableDstLayout && aRestoreDstLayout) { // => restore original layout of the dst image
			aDstImage.transition_to_layout(initialDstLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}
		
		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::optional<command_buffer> blit_image(image_t& aSrcImage, image_t& aDstImage, sync aSyncHandler, bool aRestoreSrcLayout, bool aRestoreDstLayout)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		
		auto& commandBuffer = aSyncHandler.get_or_create_command_buffer();
		// Sync before:
		aSyncHandler.establish_barrier_before_the_operation(pipeline_stage::transfer, read_memory_access{memory_access::transfer_read_access});

		// Citing the specs: "srcImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		auto initialSrcLayout = aSrcImage.target_layout();
		const bool suitableSrcLayout = initialSrcLayout == vk::ImageLayout::eTransferSrcOptimal; // For optimal performance, only allow eTransferSrcOptimal
									//|| initialSrcLayout == vk::ImageLayout::eGeneral
									//|| initialSrcLayout == vk::ImageLayout::eSharedPresentKHR;
		if (suitableSrcLayout) {
			// Just make sure that is really is in target layout:
			aSrcImage.transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {})); 
		}
		else {
			// Not a suitable src layout => must transform
			aSrcImage.transition_to_layout(vk::ImageLayout::eTransferSrcOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		// Citing the specs: "dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, or VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR"
		auto initialDstLayout = aDstImage.target_layout();
		const bool suitableDstLayout = initialDstLayout == vk::ImageLayout::eTransferDstOptimal
									|| initialDstLayout == vk::ImageLayout::eGeneral
									|| initialDstLayout == vk::ImageLayout::eSharedPresentKHR;
		if (suitableDstLayout) {
			// Just make sure that is really is in target layout:
			aDstImage.transition_to_layout({}, sync::auxiliary_with_barriers(aSyncHandler, {}, {})); 
		}
		else {
			// Not a suitable dst layout => must transform
			aDstImage.transition_to_layout(vk::ImageLayout::eTransferDstOptimal, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}


		std::array<vk::Offset3D, 2> srcOffsets = { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ static_cast<int32_t>(aSrcImage.width()), static_cast<int32_t>(aSrcImage.height()), static_cast<int32_t>(aSrcImage.depth()) } };
		std::array<vk::Offset3D, 2> dstOffsets = { vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ static_cast<int32_t>(aDstImage.width()), static_cast<int32_t>(aDstImage.height()), static_cast<int32_t>(aDstImage.depth()) } };
		
		// Operation:
		auto blitRegion = vk::ImageBlit{}
			.setSrcSubresource(vk::ImageSubresourceLayers{} // TODO: Add support for the other parameters
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0u)
				.setLayerCount(1u)
				.setMipLevel(0u)
			)
			.setSrcOffsets(srcOffsets)
			.setDstSubresource(vk::ImageSubresourceLayers{} // TODO: Add support for the other parameters
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0u)
				.setLayerCount(1u)
				.setMipLevel(0u)
			)
			.setDstOffsets(dstOffsets);

		commandBuffer.handle().blitImage(
			aSrcImage.handle(),
			aSrcImage.current_layout(),
			aDstImage.handle(),
			aDstImage.current_layout(),
			1u, &blitRegion, 
			vk::Filter::eNearest); // TODO: Support other filters and everything

		if (!suitableSrcLayout && aRestoreSrcLayout) { // => restore original layout of the src image
			aSrcImage.transition_to_layout(initialSrcLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}

		if (!suitableDstLayout && aRestoreDstLayout) { // => restore original layout of the dst image
			aDstImage.transition_to_layout(initialDstLayout, sync::auxiliary_with_barriers(aSyncHandler, {}, {}));
		}
		
		// Sync after:
		aSyncHandler.establish_barrier_after_the_operation(pipeline_stage::transfer, write_memory_access{memory_access::transfer_write_access});

		// Finish him:
		return aSyncHandler.submit_and_sync();
	}

	std::tuple<std::vector<material_gpu_data>, std::vector<image_sampler>> convert_for_gpu_usage(
		std::vector<cgb::material_config> aMaterialConfigs, 
		cgb::image_usage aImageUsage,
		cgb::filter_mode aTextureFilteMode, 
		cgb::border_handling_mode aBorderHandlingMode,
		sync aSyncHandler)
	{
		// These are the texture names loaded from file -> mapped to vector of usage-pointers
		std::unordered_map<std::string, std::vector<int*>> texNamesToUsages;
		// Textures contained in this array shall be loaded into an sRGB format
		std::set<std::string> srgbTextures;
		
		// However, if some textures are missing, provide 1x1 px textures in those spots
		std::vector<int*> whiteTexUsages;				// Provide a 1x1 px almost everywhere in those cases,
		std::vector<int*> straightUpNormalTexUsages;	// except for normal maps, provide a normal pointing straight up there.

		std::vector<material_gpu_data> gpuMaterial;
		gpuMaterial.reserve(aMaterialConfigs.size()); // important because of the pointers

		for (auto& mc : aMaterialConfigs) {
			auto& gm = gpuMaterial.emplace_back();
			gm.mDiffuseReflectivity			= mc.mDiffuseReflectivity		 ;
			gm.mAmbientReflectivity			= mc.mAmbientReflectivity		 ;
			gm.mSpecularReflectivity		= mc.mSpecularReflectivity		 ;
			gm.mEmissiveColor				= mc.mEmissiveColor				 ;
			gm.mTransparentColor			= mc.mTransparentColor			 ;
			gm.mReflectiveColor				= mc.mReflectiveColor			 ;
			gm.mAlbedo						= mc.mAlbedo					 ;
																			 
			gm.mOpacity						= mc.mOpacity					 ;
			gm.mBumpScaling					= mc.mBumpScaling				 ;
			gm.mShininess					= mc.mShininess					 ;
			gm.mShininessStrength			= mc.mShininessStrength			 ;
																			
			gm.mRefractionIndex				= mc.mRefractionIndex			 ;
			gm.mReflectivity				= mc.mReflectivity				 ;
			gm.mMetallic					= mc.mMetallic					 ;
			gm.mSmoothness					= mc.mSmoothness				 ;
																			 
			gm.mSheen						= mc.mSheen						 ;
			gm.mThickness					= mc.mThickness					 ;
			gm.mRoughness					= mc.mRoughness					 ;
			gm.mAnisotropy					= mc.mAnisotropy				 ;
																			 
			gm.mAnisotropyRotation			= mc.mAnisotropyRotation		 ;
			gm.mCustomData					= mc.mCustomData				 ;
																			 
			gm.mDiffuseTexIndex				= -1;	
			if (mc.mDiffuseTex.empty()) {
				whiteTexUsages.push_back(&gm.mDiffuseTexIndex);
			}
			else {
				auto path = cgb::clean_up_path(mc.mDiffuseTex);
				texNamesToUsages[path].push_back(&gm.mDiffuseTexIndex);
				if (settings::gLoadImagesInSrgbFormatByDefault) {
					srgbTextures.insert(path);
				}
			}

			gm.mSpecularTexIndex			= -1;							 
			if (mc.mSpecularTex.empty()) {
				whiteTexUsages.push_back(&gm.mSpecularTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mSpecularTex)].push_back(&gm.mSpecularTexIndex);
			}

			gm.mAmbientTexIndex				= -1;							 
			if (mc.mAmbientTex.empty()) {
				whiteTexUsages.push_back(&gm.mAmbientTexIndex);
			}
			else {
				auto path = cgb::clean_up_path(mc.mAmbientTex);
				texNamesToUsages[path].push_back(&gm.mAmbientTexIndex);
				if (settings::gLoadImagesInSrgbFormatByDefault) {
					srgbTextures.insert(path);
				}
			}

			gm.mEmissiveTexIndex			= -1;							 
			if (mc.mEmissiveTex.empty()) {
				whiteTexUsages.push_back(&gm.mEmissiveTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mEmissiveTex)].push_back(&gm.mEmissiveTexIndex);
			}

			gm.mHeightTexIndex				= -1;							 
			if (mc.mHeightTex.empty()) {
				whiteTexUsages.push_back(&gm.mHeightTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mHeightTex)].push_back(&gm.mHeightTexIndex);
			}

			gm.mNormalsTexIndex				= -1;							 
			if (mc.mNormalsTex.empty()) {
				straightUpNormalTexUsages.push_back(&gm.mNormalsTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mNormalsTex)].push_back(&gm.mNormalsTexIndex);
			}

			gm.mShininessTexIndex			= -1;							 
			if (mc.mShininessTex.empty()) {
				whiteTexUsages.push_back(&gm.mShininessTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mShininessTex)].push_back(&gm.mShininessTexIndex);
			}

			gm.mOpacityTexIndex				= -1;							 
			if (mc.mOpacityTex.empty()) {
				whiteTexUsages.push_back(&gm.mOpacityTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mOpacityTex)].push_back(&gm.mOpacityTexIndex);
			}

			gm.mDisplacementTexIndex		= -1;							 
			if (mc.mDisplacementTex.empty()) {
				whiteTexUsages.push_back(&gm.mDisplacementTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mDisplacementTex)].push_back(&gm.mDisplacementTexIndex);
			}

			gm.mReflectionTexIndex			= -1;							 
			if (mc.mReflectionTex.empty()) {
				whiteTexUsages.push_back(&gm.mReflectionTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mReflectionTex)].push_back(&gm.mReflectionTexIndex);
			}

			gm.mLightmapTexIndex			= -1;							 
			if (mc.mLightmapTex.empty()) {
				whiteTexUsages.push_back(&gm.mLightmapTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mLightmapTex)].push_back(&gm.mLightmapTexIndex);
			}

			gm.mExtraTexIndex				= -1;							 
			if (mc.mExtraTex.empty()) {
				whiteTexUsages.push_back(&gm.mExtraTexIndex);
			}
			else {
				texNamesToUsages[cgb::clean_up_path(mc.mExtraTex)].push_back(&gm.mExtraTexIndex);
			}
																			 
			gm.mDiffuseTexOffsetTiling		= mc.mDiffuseTexOffsetTiling	 ;
			gm.mSpecularTexOffsetTiling		= mc.mSpecularTexOffsetTiling	 ;
			gm.mAmbientTexOffsetTiling		= mc.mAmbientTexOffsetTiling	 ;
			gm.mEmissiveTexOffsetTiling		= mc.mEmissiveTexOffsetTiling	 ;
			gm.mHeightTexOffsetTiling		= mc.mHeightTexOffsetTiling		 ;
			gm.mNormalsTexOffsetTiling		= mc.mNormalsTexOffsetTiling	 ;
			gm.mShininessTexOffsetTiling	= mc.mShininessTexOffsetTiling	 ;
			gm.mOpacityTexOffsetTiling		= mc.mOpacityTexOffsetTiling	 ;
			gm.mDisplacementTexOffsetTiling	= mc.mDisplacementTexOffsetTiling;
			gm.mReflectionTexOffsetTiling	= mc.mReflectionTexOffsetTiling	 ;
			gm.mLightmapTexOffsetTiling		= mc.mLightmapTexOffsetTiling	 ;
			gm.mExtraTexOffsetTiling		= mc.mExtraTexOffsetTiling		 ;
		}

		std::vector<image_sampler> imageSamplers;
		const auto numSamplers = texNamesToUsages.size() + (whiteTexUsages.empty() ? 0 : 1) + (straightUpNormalTexUsages.empty() ? 0 : 1);
		imageSamplers.reserve(numSamplers);

		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		
		auto getSync = [numSamplers, &aSyncHandler, lSyncCount = size_t{0}] () mutable -> sync {
			++lSyncCount;
			if (lSyncCount < numSamplers) {
				return sync::auxiliary_with_barriers(aSyncHandler, sync::steal_before_handler_on_demand, {}); // Invoke external sync exactly once (if there is something to sync)
			}
			assert (lSyncCount == numSamplers);
			return std::move(aSyncHandler); // For the last image, pass the main sync => this will also have the after-handler invoked.
		};

		// Create the white texture and assign its index to all usages
		if (!whiteTexUsages.empty()) {
			imageSamplers.push_back(
				image_sampler_t::create(
					image_view_t::create(
						create_1px_texture({ 255, 255, 255, 255 }, cgb::memory_usage::device, cgb::image_usage::read_only_sampled_image, getSync())
					),
					sampler_t::create(filter_mode::nearest_neighbor, border_handling_mode::repeat)
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : whiteTexUsages) {
				*img = index;
			}
		}

		// Create the normal texture, containing a normal pointing straight up, and assign to all usages
		if (!straightUpNormalTexUsages.empty()) {
			imageSamplers.push_back(
				image_sampler_t::create(
					image_view_t::create(
						create_1px_texture({ 127, 127, 255, 0 }, cgb::memory_usage::device, cgb::image_usage::read_only_sampled_image, getSync())
					),
					sampler_t::create(filter_mode::nearest_neighbor, border_handling_mode::repeat)
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : straightUpNormalTexUsages) {
				*img = index;
			}
		}

		// Load all the images from file, and assign them to all usages
		for (auto& pair : texNamesToUsages) {
			assert (!pair.first.empty());

			bool potentiallySrgb = srgbTextures.contains(pair.first);
			
			imageSamplers.push_back(
				image_sampler_t::create(
					image_view_t::create(
						create_image_from_file(pair.first, true, potentiallySrgb, 4, cgb::memory_usage::device, aImageUsage, getSync())),
					sampler_t::create(aTextureFilteMode, aBorderHandlingMode)
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : pair.second) {
				*img = index;
			}
		}

		// Hand over ownership to the caller
		return std::make_tuple(std::move(gpuMaterial), std::move(imageSamplers));
	}

	std::tuple<std::vector<glm::vec3>, std::vector<uint32_t>> get_combined_vertices_and_indices_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> positionsData;
		std::vector<uint32_t> indicesData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				cgb::append_indices_and_vertex_data(
					cgb::additional_index_data(	indicesData,	[&]() { return modelRef.indices_for_mesh<uint32_t>(meshIndex);	} ),
					cgb::additional_vertex_data(positionsData,	[&]() { return modelRef.positions_for_mesh(meshIndex);			} )
				);
			}
		}

		return std::make_tuple( std::move(positionsData), std::move(indicesData) );
	}
	
	std::tuple<vertex_buffer, index_buffer> get_combined_vertex_and_index_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto [positionsData, indicesData] = get_combined_vertices_and_indices_for_selected_meshes(std::move(_ModelsAndSelectedMeshes));
		
		vertex_buffer positionsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(positionsData)
				.describe_only_member(positionsData[0], 0, content_description::position),
			cgb::memory_usage::device,
			positionsData.data(),
			sync::auxiliary_with_barriers(aSyncHandler, sync::steal_before_handler_on_demand, {})
			// It is fine to let positionsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		index_buffer indexBuffer = cgb::create_and_fill(
			cgb::index_buffer_meta::create_from_data(indicesData),
			cgb::memory_usage::device,
			indicesData.data(),
			std::move(aSyncHandler)
			// It is fine to let indicesData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
	}

	std::vector<glm::vec3> get_combined_normals_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> normalsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(normalsData, modelRef.normals_for_mesh(meshIndex));
			}
		}

		return normalsData;
	}
	
	vertex_buffer get_combined_normal_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto normalsData = get_combined_normals_for_selected_meshes(std::move(_ModelsAndSelectedMeshes));
		
		vertex_buffer normalsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(normalsData),
			cgb::memory_usage::device,
			normalsData.data(),
			std::move(aSyncHandler)
			// It is fine to let normalsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);
		
		return normalsBuffer;
	}

	std::vector<glm::vec3> get_combined_tangents_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> tangentsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(tangentsData, modelRef.tangents_for_mesh(meshIndex));
			}
		}

		return tangentsData;
	}
	
	vertex_buffer get_combined_tangent_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto tangentsData = get_combined_tangents_for_selected_meshes(std::move(_ModelsAndSelectedMeshes));
		
		vertex_buffer tangentsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(tangentsData),
			cgb::memory_usage::device,
			tangentsData.data(),
			std::move(aSyncHandler)
			// It is fine to let tangentsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return tangentsBuffer;
	}

	std::vector<glm::vec3> get_combined_bitangents_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes)
	{
		std::vector<glm::vec3> bitangentsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(bitangentsData, modelRef.bitangents_for_mesh(meshIndex));
			}
		}

		return bitangentsData;
	}
	
	vertex_buffer get_combined_bitangent_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto bitangentsData = get_combined_bitangents_for_selected_meshes(std::move(_ModelsAndSelectedMeshes));
		
		vertex_buffer bitangentsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(bitangentsData),
			cgb::memory_usage::device,
			bitangentsData.data(),
			std::move(aSyncHandler)
			// It is fine to let bitangentsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return bitangentsBuffer;
	}

	std::vector<glm::vec4> get_combined_colors_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _ColorsSet)
	{
		std::vector<glm::vec4> colorsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(colorsData, modelRef.colors_for_mesh(meshIndex, _ColorsSet));
			}
		}

		return colorsData;
	}
	
	vertex_buffer get_combined_color_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _ColorsSet, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto colorsData = get_combined_colors_for_selected_meshes(std::move(_ModelsAndSelectedMeshes), _ColorsSet);
		
		vertex_buffer colorsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(colorsData),
			cgb::memory_usage::device,
			colorsData.data(),
			std::move(aSyncHandler)
			// It is fine to let colorsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return colorsBuffer;
	}

	std::vector<glm::vec2> get_combined_2d_texture_coordinates_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.texture_coordinates_for_mesh<glm::vec2>(meshIndex, _TexCoordSet));
			}
		}

		return texCoordsData;
	}
	
	vertex_buffer get_combined_2d_texture_coordinate_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto texCoordsData = get_combined_2d_texture_coordinates_for_selected_meshes(std::move(_ModelsAndSelectedMeshes), _TexCoordSet);
		
		vertex_buffer texCoordsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(texCoordsData),
			cgb::memory_usage::device,
			texCoordsData.data(),
			std::move(aSyncHandler)
			// It is fine to let texCoordsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return texCoordsBuffer;
	}

	std::vector<glm::vec3> get_combined_3d_texture_coordinates_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet)
	{
		std::vector<glm::vec3> texCoordsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.texture_coordinates_for_mesh<glm::vec3>(meshIndex, _TexCoordSet));
			}
		}

		return texCoordsData;
	}
	
	vertex_buffer get_combined_3d_texture_coordinate_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet, sync aSyncHandler)
	{
		aSyncHandler.set_queue_hint(cgb::context().transfer_queue());
		auto texCoordsData = get_combined_3d_texture_coordinates_for_selected_meshes(std::move(_ModelsAndSelectedMeshes), _TexCoordSet);
		
		vertex_buffer texCoordsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(texCoordsData),
			cgb::memory_usage::device,
			texCoordsData.data(),
			std::move(aSyncHandler)
			// It is fine to let texCoordsData go out of scope, since its data has been copied to a
			// staging buffer within create_and_fill, which is lifetime-handled by the command buffer.
		);

		return texCoordsBuffer;
	}

}
#include <cg_base.hpp>

namespace cgb
{
	void transition_image_layout(image_t& _Image, vk::Format _Format, vk::ImageLayout _NewLayout, const semaphore_t* _WaitSemaphore, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		//auto commandBuffer = context().create_command_buffers_for_graphics(1, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		auto commandBuffer = context().graphics_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Immediately start recording the command buffer:
		commandBuffer.begin_recording();

		vk::AccessFlags sourceAccessMask, destinationAccessMask;
		vk::PipelineStageFlags sourceStageFlags, destinationStageFlags;

		// TODO: This has to be reworked entirely!!

		auto oldImageLayout = _Image.current_layout();

		// There are two transitions we need to handle [3]:
		//  - Undefined --> transfer destination : transfer writes that don't need to wait on anything
		//  - Transfer destination --> shader reading : shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where we're going to use the texture
		if (oldImageLayout == vk::ImageLayout::eUndefined && _NewLayout == vk::ImageLayout::eTransferDstOptimal) {
			sourceAccessMask = vk::AccessFlags();
			destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
			sourceStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStageFlags = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (oldImageLayout == vk::ImageLayout::eTransferDstOptimal && _NewLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
			destinationAccessMask = vk::AccessFlagBits::eShaderRead;
			sourceStageFlags = vk::PipelineStageFlagBits::eTransfer;
			destinationStageFlags = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else if (oldImageLayout == vk::ImageLayout::eUndefined && _NewLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
			sourceAccessMask = vk::AccessFlags();
			destinationAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			sourceStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStageFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		}
		else {
			// TODO: This has to be reworked entirely! (I have no idea what I'm doing...)
			sourceStageFlags = vk::PipelineStageFlagBits::eAllCommands; // TODO: Clearly, this is not optimal
			destinationStageFlags = vk::PipelineStageFlagBits::eAllCommands; // TODO: Clearly, this is not optimal

			// The following is copied from VulkanTools.cpp from Sascha Willems' Vulkan Examples
			//
			// * Assorted commonly used Vulkan helper functions
			// *
			// * Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
			// *
			// * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case vk::ImageLayout::eUndefined:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				sourceAccessMask = {};
				break;

			case vk::ImageLayout::ePreinitialized:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				sourceAccessMask = vk::AccessFlagBits::eHostWrite;
				break;

			case vk::ImageLayout::eColorAttachmentOptimal:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				sourceAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
				break;

			case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				sourceAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				break;

			case vk::ImageLayout::eTransferSrcOptimal:
				// Image is a transfer source 
				// Make sure any reads from the image have been finished
				sourceAccessMask = vk::AccessFlagBits::eTransferRead;
				break;

			case vk::ImageLayout::eTransferDstOptimal:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
				break;

			case vk::ImageLayout::eShaderReadOnlyOptimal:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				sourceAccessMask = vk::AccessFlagBits::eShaderRead;
				break;
			default:
				// Other source layouts aren't handled (yet)
				throw std::runtime_error("Other source layouts aren't handled (yet)");
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (_NewLayout)
			{
			case vk::ImageLayout::eTransferDstOptimal:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
				break;

			case vk::ImageLayout::eTransferSrcOptimal:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				destinationAccessMask = vk::AccessFlagBits::eTransferRead;
				break;

			case vk::ImageLayout::eColorAttachmentOptimal:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				destinationAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
				break;

			case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				destinationAccessMask = destinationAccessMask | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				break;

			case vk::ImageLayout::eShaderReadOnlyOptimal:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (sourceAccessMask)
				{
					sourceAccessMask = vk::AccessFlagBits::eHostWrite | vk::AccessFlagBits::eTransferWrite;
				}
				destinationAccessMask = vk::AccessFlagBits::eShaderRead;
				break;

			case vk::ImageLayout::eGeneral:
				// TODO: This should be valid... but set something? I have no idea what I'm doing...
				break;

			default:
				// Other destination layouts aren't handled (yet)
				throw std::runtime_error("Other destination layouts aren't handled (yet)");
			}
		}


		// One of the most common ways to perform layout transitions is using an image memory barrier. A pipeline barrier like that 
		// is generally used to synchronize access to resources, like ensuring that a write to a buffer completes before reading from 
		// it, but it can also be used to transition image layouts and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE 
		// is used.There is an equivalent buffer memory barrier to do this for buffers. [3]
		auto barrier = _Image.create_barrier(sourceAccessMask, destinationAccessMask, oldImageLayout, _NewLayout);

		// The pipeline stages that you are allowed to specify before and after the barrier depend on how you use the resource before and 
		// after the barrier.The allowed values are listed in this table of the specification.For example, if you're going to read from a 
		// uniform after the barrier, you would specify a usage of VK_ACCESS_UNIFORM_READ_BIT and the earliest shader that will read from 
		// the uniform as pipeline stage, for example VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT. It would not make sense to specify a non-shader 
		// pipeline stage for this type of usage and the validation layers will warn you when you specify a pipeline stage that does not 
		// match the type of usage. [3]
		commandBuffer.handle().pipelineBarrier(
			sourceStageFlags,
			destinationStageFlags,
			vk::DependencyFlags(), // The third parameter is either 0 or VK_DEPENDENCY_BY_REGION_BIT. The latter turns the barrier into a per-region condition. That means that the implementation is allowed to already begin reading from the parts of a resource that were written so far, for example. [3]
			{},
			{},
			{ barrier });

		// That's all
		commandBuffer.end_recording();

		// Create a semaphore which can, or rather, MUST be used to wait for the results
		auto transitionCompleteSemaphore = semaphore_t::create();
		transitionCompleteSemaphore->set_designated_queue(cgb::context().graphics_queue()); //< Just store the info

		vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eAllCommands; // Just set to all commands. Don't know if this could be optimized somehow?!
		auto submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1u)
			.setPCommandBuffers(commandBuffer.handle_addr())
			.setWaitSemaphoreCount(nullptr != _WaitSemaphore ? 1u : 0u)
			.setPWaitSemaphores(nullptr != _WaitSemaphore ? _WaitSemaphore->handle_addr() : nullptr)
			.setPWaitDstStageMask(&waitMask)
			.setSignalSemaphoreCount(1u)
			.setPSignalSemaphores(transitionCompleteSemaphore->handle_addr());

		cgb::context().graphics_queue().handle().submit({ submitInfo }, nullptr);
		_Image.set_current_layout(_NewLayout); // Just optimistically set it

		// Take care of the lifetime of:
		//  - commandBuffer
		transitionCompleteSemaphore->set_custom_deleter([
			ownedCommandBuffer{ std::move(commandBuffer) }
		]() { /* Nothing to do here, the destructors will do the cleanup, the lambda is just holding them. */ });

		if (_SemaphoreHandler) { // Did the user provide a handler?
			_SemaphoreHandler(std::move(transitionCompleteSemaphore)); // Transfer ownership and be done with it
		}
		else {
			LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
			cgb::context().logical_device().waitIdle();
		}

	}

	void copy_image_to_another(const image_t& pSrcImage, image_t& pDstImage, const semaphore_t* _WaitSemaphore, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		auto commandBuffer = cgb::context().transfer_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Start recording the command buffer:
		commandBuffer.begin_recording();

		auto copyRegion = vk::ImageCopy{}
			.setExtent(pSrcImage.config().extent) // TODO: Support different ranges/extents
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
			pSrcImage.image_handle(),
			pSrcImage.current_layout(),
			pDstImage.image_handle(),
			pSrcImage.current_layout(), // same layout as src?
			1u, &copyRegion);

		// That's all
		commandBuffer.end_recording();

		// Create a semaphore which can, or rather, MUST be used to wait for the results
		auto copyCompleteSemaphore = semaphore_t::create();
		copyCompleteSemaphore->set_designated_queue(cgb::context().transfer_queue()); //< Just store the info

		vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eAllCommands; // Just set to all commands. Don't know if this could be optimized somehow?!
		auto submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1u)
			.setPCommandBuffers(commandBuffer.handle_addr())
			.setWaitSemaphoreCount(nullptr != _WaitSemaphore ? 1u : 0u)
			.setPWaitSemaphores(nullptr != _WaitSemaphore ? _WaitSemaphore->handle_addr() : nullptr)
			.setPWaitDstStageMask(&waitMask)
			.setSignalSemaphoreCount(1u)
			.setPSignalSemaphores(copyCompleteSemaphore->handle_addr());

		cgb::context().transfer_queue().handle().submit({ submitInfo }, nullptr);
		pDstImage.set_current_layout(pSrcImage.current_layout()); // Just optimistically set it

		if (_SemaphoreHandler) { // Did the user provide a handler?
			copyCompleteSemaphore->set_custom_deleter([
				ownedCommandBuffer{ std::move(commandBuffer) } // Just take care of the commandBuffer's lifetime
			](){});
			_SemaphoreHandler(std::move(copyCompleteSemaphore)); // Transfer ownership and be done with it
		}
		else {
			LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
			cgb::context().logical_device().waitIdle();
		}
	}

	std::tuple<std::vector<material_gpu_data>, std::vector<image_sampler>> convert_for_gpu_usage(std::vector<cgb::material_config> _MaterialConfigs, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler, cgb::image_usage _ImageUsage)
	{
		// These are the texture names loaded from file:
		std::unordered_map<std::string, std::vector<int*>> texNamesToUsages;
		// However, if some textures are missing, provide 1x1 px textures in those spots
		std::vector<int*> whiteTexUsages;				// Provide a 1x1 px almost everywhere in those cases,
		std::vector<int*> straightUpNormalTexUsages;	// except for normal maps, provide a normal pointing straight up there.

		std::vector<material_gpu_data> gpuMaterial;
		gpuMaterial.reserve(_MaterialConfigs.size()); // important because of the pointers

		for (auto& mc : _MaterialConfigs) {
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
				texNamesToUsages[cgb::clean_up_path(mc.mDiffuseTex)].push_back(&gm.mDiffuseTexIndex);
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
				texNamesToUsages[cgb::clean_up_path(mc.mAmbientTex)].push_back(&gm.mAmbientTexIndex);
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
		imageSamplers.reserve(texNamesToUsages.size() + 2); // + 2 => one for the white tex, one for the normals tex

		// Create the white texture and assign its index to all usages
		if (whiteTexUsages.size() > 0) {
			imageSamplers.push_back(
				image_sampler_t::create(
					image_view_t::create(create_1px_texture({ 255, 255, 255, 255 }, cgb::memory_usage::device, cgb::image_usage::read_only_sampled_image, _SemaphoreHandler)),
					sampler_t::create(filter_mode::nearest_neighbor, border_handling_mode::repeat)
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : whiteTexUsages) {
				*img = index;
			}
		}

		// Create the normal texture, containing a normal pointing straight up, and assign to all usages
		if (straightUpNormalTexUsages.size() > 0) {
			imageSamplers.push_back(
				image_sampler_t::create(
					image_view_t::create(create_1px_texture({ 127, 127, 255, 0 }, cgb::memory_usage::device, cgb::image_usage::read_only_sampled_image, _SemaphoreHandler)),
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

			imageSamplers.push_back(
				image_sampler_t::create(
					image_view_t::create(create_image_from_file(pair.first, cgb::memory_usage::device, _ImageUsage, _SemaphoreHandler)),
					sampler_t::create(filter_mode::nearest_neighbor, border_handling_mode::repeat)
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

	std::tuple<vertex_buffer, index_buffer> get_combined_vertex_and_index_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
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

		vertex_buffer positionsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(positionsData)
				.describe_only_member(positionsData[0], 0, content_description::position),
			cgb::memory_usage::device,
			positionsData.data(),
			[&] (semaphore _Semaphore) {  
				if (_SemaphoreHandler) { // Did the user provide a handler?
					_Semaphore->set_custom_deleter([
						ownedData{ std::move(positionsData) } // Let the semaphore handle the lifetime of the data buffer
					](){});

					_SemaphoreHandler( std::move(*_Semaphore) ); // Transfer ownership and be done with it
				}
				else {
					if (_Semaphore->has_designated_queue()) {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the queue via waitIdle until the operation has completed.");
						_Semaphore->designated_queue()->handle().waitIdle();
					}
					else {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
						cgb::context().logical_device().waitIdle();
					}
				}
			}
		);

		index_buffer indexBuffer = cgb::create_and_fill(
			cgb::index_buffer_meta::create_from_data(indicesData),
			cgb::memory_usage::device,
			indicesData.data(),
			[&] (semaphore _Semaphore) {  
				if (_SemaphoreHandler) { // Did the user provide a handler?
					_Semaphore->set_custom_deleter([
						ownedData{ std::move(indicesData) } // Let the semaphore handle the lifetime of the data buffer
					](){});

					_SemaphoreHandler( std::move(*_Semaphore) ); // Transfer ownership and be done with it
				}
				else {
					if (_Semaphore->has_designated_queue()) {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the queue via waitIdle until the operation has completed.");
						_Semaphore->designated_queue()->handle().waitIdle();
					}
					else {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
						cgb::context().logical_device().waitIdle();
					}
				}
			}
		);

		return std::make_tuple(std::move(positionsBuffer), std::move(indexBuffer));
	}

	vertex_buffer get_combined_normal_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		std::vector<glm::vec3> normalsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(normalsData, modelRef.normals_for_mesh(meshIndex));
			}
		}

		vertex_buffer normalsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(normalsData),
			cgb::memory_usage::device,
			normalsData.data(),
			[&] (semaphore _Semaphore) {  
				if (_SemaphoreHandler) { // Did the user provide a handler?
					_Semaphore->set_custom_deleter([
						ownedData{ std::move(normalsData) } // Let the semaphore handle the lifetime of the data buffer
					](){});

					_SemaphoreHandler( std::move(*_Semaphore) ); // Transfer ownership and be done with it
				}
				else {
					if (_Semaphore->has_designated_queue()) {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the queue via waitIdle until the operation has completed.");
						_Semaphore->designated_queue()->handle().waitIdle();
					}
					else {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
						cgb::context().logical_device().waitIdle();
					}
				}
			}
		);

		return normalsBuffer;
	}

	vertex_buffer get_combined_tangent_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		std::vector<glm::vec3> tangentsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(tangentsData, modelRef.tangents_for_mesh(meshIndex));
			}
		}

		vertex_buffer tangentsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(tangentsData),
			cgb::memory_usage::device,
			tangentsData.data(),
			[&] (semaphore _Semaphore) {  
				if (_SemaphoreHandler) { // Did the user provide a handler?
					_Semaphore->set_custom_deleter([
						ownedData{ std::move(tangentsData) } // Let the semaphore handle the lifetime of the data buffer
					](){});

					_SemaphoreHandler( std::move(*_Semaphore) ); // Transfer ownership and be done with it
				}
				else {
					if (_Semaphore->has_designated_queue()) {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the queue via waitIdle until the operation has completed.");
						_Semaphore->designated_queue()->handle().waitIdle();
					}
					else {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
						cgb::context().logical_device().waitIdle();
					}
				}
			}
		);

		return tangentsBuffer;
	}

	vertex_buffer get_combined_bitangent_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		std::vector<glm::vec3> bitangentsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(bitangentsData, modelRef.bitangents_for_mesh(meshIndex));
			}
		}

		vertex_buffer bitangentsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(bitangentsData),
			cgb::memory_usage::device,
			bitangentsData.data(),
			[&] (semaphore _Semaphore) {  
				if (_SemaphoreHandler) { // Did the user provide a handler?
					_Semaphore->set_custom_deleter([
						ownedData{ std::move(bitangentsData) } // Let the semaphore handle the lifetime of the data buffer
					](){});

					_SemaphoreHandler( std::move(*_Semaphore) ); // Transfer ownership and be done with it
				}
				else {
					if (_Semaphore->has_designated_queue()) {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the queue via waitIdle until the operation has completed.");
						_Semaphore->designated_queue()->handle().waitIdle();
					}
					else {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
						cgb::context().logical_device().waitIdle();
					}
				}
			}
		);

		return bitangentsBuffer;
	}

	vertex_buffer get_combined_color_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _ColorsSet, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		std::vector<glm::vec4> colorsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(colorsData, modelRef.colors_for_mesh(meshIndex, _ColorsSet));
			}
		}

		vertex_buffer colorsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(colorsData),
			cgb::memory_usage::device,
			colorsData.data(),
			[&] (semaphore _Semaphore) {  
				if (_SemaphoreHandler) { // Did the user provide a handler?
					_Semaphore->set_custom_deleter([
						ownedData{ std::move(colorsData) } // Let the semaphore handle the lifetime of the data buffer
					](){});

					_SemaphoreHandler( std::move(*_Semaphore) ); // Transfer ownership and be done with it
				}
				else {
					if (_Semaphore->has_designated_queue()) {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the queue via waitIdle until the operation has completed.");
						_Semaphore->designated_queue()->handle().waitIdle();
					}
					else {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
						cgb::context().logical_device().waitIdle();
					}
				}
			}
		);

		return colorsBuffer;
	}

	vertex_buffer get_combined_2d_texture_coordinate_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		std::vector<glm::vec2> texCoordsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.texture_coordinates_for_mesh<glm::vec2>(meshIndex, _TexCoordSet));
			}
		}

		vertex_buffer texCoordsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(texCoordsData),
			cgb::memory_usage::device,
			texCoordsData.data(),
			[&] (semaphore _Semaphore) {  
				if (_SemaphoreHandler) { // Did the user provide a handler?
					_Semaphore->set_custom_deleter([
						ownedData{ std::move(texCoordsData) } // Let the semaphore handle the lifetime of the data buffer
					](){});

					_SemaphoreHandler( std::move(*_Semaphore) ); // Transfer ownership and be done with it
				}
				else {
					if (_Semaphore->has_designated_queue()) {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the queue via waitIdle until the operation has completed.");
						_Semaphore->designated_queue()->handle().waitIdle();
					}
					else {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
						cgb::context().logical_device().waitIdle();
					}
				}
			}
		);

		return texCoordsBuffer;
	}

	vertex_buffer get_combined_3d_texture_coordinate_buffers_for_selected_meshes(std::vector<std::tuple<const model_t&, std::vector<size_t>>> _ModelsAndSelectedMeshes, int _TexCoordSet, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		std::vector<glm::vec3> texCoordsData;

		for (auto& pair : _ModelsAndSelectedMeshes) {
			const auto& modelRef = std::get<const model_t&>(pair);
			for (auto meshIndex : std::get<std::vector<size_t>>(pair)) {
				insert_into(texCoordsData, modelRef.texture_coordinates_for_mesh<glm::vec3>(meshIndex, _TexCoordSet));
			}
		}

		vertex_buffer texCoordsBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(texCoordsData),
			cgb::memory_usage::device,
			texCoordsData.data(),
			[&] (semaphore _Semaphore) {  
				if (_SemaphoreHandler) { // Did the user provide a handler?
					_Semaphore->set_custom_deleter([
						ownedData{ std::move(texCoordsData) } // Let the semaphore handle the lifetime of the data buffer
					](){});

					_SemaphoreHandler( std::move(*_Semaphore) ); // Transfer ownership and be done with it
				}
				else {
					if (_Semaphore->has_designated_queue()) {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the queue via waitIdle until the operation has completed.");
						_Semaphore->designated_queue()->handle().waitIdle();
					}
					else {
						LOG_WARNING("No semaphore handler was provided but a semaphore emerged. Will block the device via waitIdle until the operation has completed.");
						cgb::context().logical_device().waitIdle();
					}
				}
			}
		);

		return texCoordsBuffer;
	}

}
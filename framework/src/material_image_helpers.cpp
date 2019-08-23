#include <cg_base.hpp>

namespace cgb
{
	owning_resource<semaphore_t> transition_image_layout(const image_t& pImage, vk::Format pFormat, vk::ImageLayout pOldLayout, vk::ImageLayout pNewLayout, const semaphore_t* _WaitSemaphore)
	{
		//auto commandBuffer = context().create_command_buffers_for_graphics(1, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		auto commandBuffer = context().graphics_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Immediately start recording the command buffer:
		commandBuffer.begin_recording();

		vk::AccessFlags sourceAccessMask, destinationAccessMask;
		vk::PipelineStageFlags sourceStageFlags, destinationStageFlags;

		// There are two transitions we need to handle [3]:
		//  - Undefined --> transfer destination : transfer writes that don't need to wait on anything
		//  - Transfer destination --> shader reading : shader reads should wait on transfer writes, specifically the shader reads in the fragment shader, because that's where we're going to use the texture
		if (pOldLayout == vk::ImageLayout::eUndefined && pNewLayout == vk::ImageLayout::eTransferDstOptimal) {
			sourceAccessMask = vk::AccessFlags();
			destinationAccessMask = vk::AccessFlagBits::eTransferWrite;
			sourceStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStageFlags = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (pOldLayout == vk::ImageLayout::eTransferDstOptimal && pNewLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			sourceAccessMask = vk::AccessFlagBits::eTransferWrite;
			destinationAccessMask = vk::AccessFlagBits::eShaderRead;
			sourceStageFlags = vk::PipelineStageFlagBits::eTransfer;
			destinationStageFlags = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else if (pOldLayout == vk::ImageLayout::eUndefined && pNewLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
			sourceAccessMask = vk::AccessFlags();
			destinationAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			sourceStageFlags = vk::PipelineStageFlagBits::eTopOfPipe;
			destinationStageFlags = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		}
		else {
			throw std::invalid_argument("unsupported layout transition");
		}


		// One of the most common ways to perform layout transitions is using an image memory barrier. A pipeline barrier like that 
		// is generally used to synchronize access to resources, like ensuring that a write to a buffer completes before reading from 
		// it, but it can also be used to transition image layouts and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE 
		// is used.There is an equivalent buffer memory barrier to do this for buffers. [3]
		auto barrier = pImage.create_barrier(sourceAccessMask, destinationAccessMask, pOldLayout, pNewLayout);

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

		// Take care of the lifetime of:
		//  - commandBuffer
		transitionCompleteSemaphore->set_custom_deleter([
			ownedCommandBuffer{ std::move(commandBuffer) }
		]() { /* Nothing to do here, the destructors will do the cleanup, the lambda is just holding them. */ });
		return transitionCompleteSemaphore;
	}

	std::tuple<std::vector<material_gpu_data>, std::vector<image_sampler>> convert_for_gpu_usage(std::vector<cgb::material_config> _MaterialConfigs, std::function<void(owning_resource<semaphore_t>)> _SemaphoreHandler)
	{
		std::unordered_map<std::string, std::vector<int*>> texNamesToUsages;
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
			texNamesToUsages[cgb::clean_up_path(mc.mDiffuseTex)].push_back(&gm.mDiffuseTexIndex);
			gm.mSpecularTexIndex			= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mSpecularTex)].push_back(&gm.mSpecularTexIndex);
			gm.mAmbientTexIndex				= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mAmbientTex)].push_back(&gm.mAmbientTexIndex);
			gm.mEmissiveTexIndex			= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mEmissiveTex)].push_back(&gm.mEmissiveTexIndex);
			gm.mHeightTexIndex				= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mHeightTex)].push_back(&gm.mHeightTexIndex);
			gm.mNormalsTexIndex				= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mNormalsTex)].push_back(&gm.mNormalsTexIndex);
			gm.mShininessTexIndex			= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mShininessTex)].push_back(&gm.mShininessTexIndex);
			gm.mOpacityTexIndex				= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mOpacityTex)].push_back(&gm.mOpacityTexIndex);
			gm.mDisplacementTexIndex		= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mDisplacementTex)].push_back(&gm.mDisplacementTexIndex);
			gm.mReflectionTexIndex			= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mReflectionTex)].push_back(&gm.mReflectionTexIndex);
			gm.mLightmapTexIndex			= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mLightmapTex)].push_back(&gm.mLightmapTexIndex);
			gm.mExtraTexIndex				= -1;							 
			texNamesToUsages[cgb::clean_up_path(mc.mExtraTex)].push_back(&gm.mExtraTexIndex);
																			 
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
		imageSamplers.reserve(texNamesToUsages.size());
		for (auto& pair : texNamesToUsages) {
			if (pair.first.empty()) {
				continue;
			}

			imageSamplers.push_back(
				image_sampler_t::create(
					image_view_t::create(create_image_from_file(pair.first, _SemaphoreHandler)),
					sampler_t::create(filter_mode::nearest_neighbor, border_handling_mode::repeat)
				)
			);
			int index = static_cast<int>(imageSamplers.size() - 1);
			for (auto* img : pair.second) {
				*img = index;
			}
		}

		return std::make_tuple(std::move(gpuMaterial), std::move(imageSamplers));
	}
}
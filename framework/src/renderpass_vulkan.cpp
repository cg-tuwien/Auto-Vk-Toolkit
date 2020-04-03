#include <cg_base.hpp>

namespace cgb
{
	using namespace cpplinq;

	owning_resource<renderpass_t> renderpass_t::create(std::vector<attachment> pAttachments, std::function<void(renderpass_sync&)> aSync, cgb::context_specific_function<void(renderpass_t&)> pAlterConfigBeforeCreation)
	{
		renderpass_t result;

		std::map<uint32_t, vk::AttachmentReference> mSpecificColorLocations;
		std::queue<vk::AttachmentReference> mArbitraryColorLocations;
		std::map<uint32_t, vk::AttachmentReference> mSpecificDepthLocations;
		std::queue<vk::AttachmentReference> mArbitraryDepthLocations;
		std::map<uint32_t, vk::AttachmentReference> mSpecificResolveLocations;
		std::queue<vk::AttachmentReference> mArbitraryResolveLocations;
		std::map<uint32_t, vk::AttachmentReference> mSpecificInputLocations;
		std::queue<vk::AttachmentReference> mArbitraryInputLocations;

		uint32_t maxColorLoc = 0u;
		uint32_t maxDepthLoc = 0u;
		uint32_t maxResolveLoc = 0u;
		uint32_t maxInputLoc = 0u;

		for (const auto& a : pAttachments) {
			// Try to infer initial and final image layouts (If this isn't cool => user must use pAlterConfigBeforeCreation)
			vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
			vk::ImageLayout finalLayout = vk::ImageLayout::eUndefined;
			if (a.is_depth_stencil_attachment()) {
				if (cfg::attachment_load_operation::load == a.mLoadOperation) {
					if (a.mStencilLoadOperation.has_value() || cfg::attachment_load_operation::load == a.mStencilLoadOperation.value() || has_stencil_component(a.format())) {
						initialLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
					}
					else {
						initialLayout = vk::ImageLayout::eDepthReadOnlyOptimal;
					}
				}
				else {
					if (a.mStencilLoadOperation.has_value() || cfg::attachment_load_operation::load == a.mStencilLoadOperation.value() || has_stencil_component(a.format())) {
						initialLayout = vk::ImageLayout::eStencilReadOnlyOptimal;
					}
				}
				if (cfg::attachment_store_operation::store == a.mStoreOperation) {
					if (a.mStencilStoreOperation.has_value() || cfg::attachment_store_operation::store == a.mStencilStoreOperation.value() || has_stencil_component(a.format())) {
						finalLayout = initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					}
					else {
						finalLayout = initialLayout = vk::ImageLayout::eDepthAttachmentOptimal;
					}
				}
				else {
					if (a.mStencilStoreOperation.has_value() || cfg::attachment_store_operation::store == a.mStencilStoreOperation.value() || has_stencil_component(a.format())) {
						finalLayout = initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					}
				}
			}
			else {
				if (cfg::attachment_load_operation::load == a.mLoadOperation) {
					initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
					// TODO: right layout for both, color and input attachments?
				}				
				if (cfg::attachment_store_operation::store == a.mStoreOperation) {
					finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
					// TODO: right layout for both, color and input attachments?
					if (a.is_color_attachment() && a.is_presentable()) {
						finalLayout = vk::ImageLayout::ePresentSrcKHR;
					}
				}				
			}

			// 1. Create the attachment descriptions
			result.mAttachmentDescriptions.push_back(vk::AttachmentDescription()
				.setFormat(a.format().mFormat)
				.setSamples(to_vk_sample_count(a.sample_count()))
				.setLoadOp(to_vk_load_op(a.mLoadOperation))
				.setStoreOp(to_vk_store_op(a.mStoreOperation))
				.setStencilLoadOp(to_vk_load_op(a.get_stencil_load_op()))
				.setStencilStoreOp(to_vk_store_op(a.get_stencil_store_op()))
				.setInitialLayout(initialLayout)
				.setFinalLayout(finalLayout)
			);

			auto attachmentIndex = static_cast<uint32_t>(result.mAttachmentDescriptions.size() - 1);

			// 2. Depending on the type, fill one or multiple of the references
			if (a.is_color_attachment()) { //< 2.1. COLOR ATTACHMENT
				// Build the Reference
				auto attachmentRef = vk::AttachmentReference().setAttachment(attachmentIndex).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
				// But where to insert it?
				if (a.has_specific_location()) {
					assert(mSpecificColorLocations.count(a.location()) == 0); // assert that it is not already contained
					mSpecificColorLocations[a.location()] = attachmentRef;
					maxColorLoc = std::max(maxColorLoc, a.location());
				}
				else {
					mArbitraryColorLocations.push(attachmentRef);
				}
			}
			if (a.is_depth_stencil_attachment()) { //< 2.2. DEPTH ATTACHMENT
				// Build the Reference
				auto attachmentRef = vk::AttachmentReference().setAttachment(attachmentIndex).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
				// But where to insert it?
				if (a.has_specific_location()) {
					assert(mSpecificDepthLocations.count(a.location()) == 0); // assert that it is not already contained
					mSpecificDepthLocations[a.location()] = attachmentRef;
					maxDepthLoc = std::max(maxDepthLoc, a.location());
				}
				else {
					mArbitraryDepthLocations.push(attachmentRef);
				}
			}
			if (a.is_to_be_resolved()) { //< 2.3. RESOLVE ATTACHMENT
				// Build the Reference
				auto attachmentRef = vk::AttachmentReference().setAttachment(attachmentIndex).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
				// But where to insert it?
				if (a.has_specific_location()) {
					assert(mSpecificResolveLocations.count(a.location()) == 0); // assert that it is not already contained
					mSpecificResolveLocations[a.location()] = attachmentRef;
					maxResolveLoc = std::max(maxResolveLoc, a.location());
				}
				else {
					mArbitraryResolveLocations.push(attachmentRef);
				}
			}
			if (a.is_shader_input_attachment()) { //< 2.4. INPUT ATTACHMENT
				// Build the Reference
				auto attachmentRef = vk::AttachmentReference().setAttachment(attachmentIndex).setLayout(vk::ImageLayout::eShaderReadOnlyOptimal); // TODO: <---- what about this layout? is it okay?
				// But where to insert it?
				if (a.has_specific_location()) {
					assert(mSpecificInputLocations.count(a.location()) == 0); // assert that it is not already contained
					mSpecificInputLocations[a.location()] = attachmentRef;
					maxInputLoc = std::max(maxInputLoc, a.location());
				}
				else {
					mArbitraryInputLocations.push(attachmentRef);
				}
			}
		}

		// 3. Fill all the vectors in the right order:
		const auto unusedAttachmentRef = vk::AttachmentReference().setAttachment(VK_ATTACHMENT_UNUSED);
		//    colors => mOrderedColorAttachmentRefs
		for (uint32_t loc = 0; loc < maxColorLoc || mArbitraryColorLocations.size() > 0; ++loc) {
			if (mSpecificColorLocations.count(loc) > 0) {
				result.mOrderedColorAttachmentRefs.push_back(mSpecificColorLocations[loc]);
			}
			else {
				if (mArbitraryColorLocations.size() > 0) {
					result.mOrderedColorAttachmentRefs.push_back(mArbitraryColorLocations.front());
					mArbitraryColorLocations.pop();
				}
				else {
					result.mOrderedColorAttachmentRefs.push_back(unusedAttachmentRef);
				}
			}
		}
		//    depths => mOrderedDepthAttachmentRefs
		for (uint32_t loc = 0; loc < maxDepthLoc || mArbitraryDepthLocations.size() > 0; ++loc) {
			if (mSpecificDepthLocations.count(loc) > 0) {
				result.mOrderedDepthAttachmentRefs.push_back(mSpecificDepthLocations[loc]);
			}
			else {
				if (mArbitraryDepthLocations.size() > 0) {
					result.mOrderedDepthAttachmentRefs.push_back(mArbitraryDepthLocations.front());
					mArbitraryDepthLocations.pop();
				}
				else {
					result.mOrderedDepthAttachmentRefs.push_back(unusedAttachmentRef);
				}
			}
		}
		//    resolves => mOrderedResolveAttachmentRefs
		for (uint32_t loc = 0; loc < maxResolveLoc || mArbitraryResolveLocations.size() > 0; ++loc) {
			if (mSpecificResolveLocations.count(loc) > 0) {
				result.mOrderedResolveAttachmentRefs.push_back(mSpecificResolveLocations[loc]);
			}
			else {
				if (mArbitraryResolveLocations.size() > 0) {
					result.mOrderedResolveAttachmentRefs.push_back(mArbitraryResolveLocations.front());
					mArbitraryResolveLocations.pop();
				}
				else {
					result.mOrderedResolveAttachmentRefs.push_back(unusedAttachmentRef);
				}
			}
		}
		//    inputs => mOrderedInputAttachmentRefs
		for (uint32_t loc = 0; loc < maxInputLoc || mArbitraryInputLocations.size() > 0; ++loc) {
			if (mSpecificInputLocations.count(loc) > 0) {
				result.mOrderedInputAttachmentRefs.push_back(mSpecificInputLocations[loc]);
			}
			else {
				if (mArbitraryInputLocations.size() > 0) {
					result.mOrderedInputAttachmentRefs.push_back(mArbitraryInputLocations.front());
					mArbitraryInputLocations.pop();
				}
				else {
					result.mOrderedInputAttachmentRefs.push_back(unusedAttachmentRef);
				}
			}
		}

		// SOME SANITY CHECKS:
		// - The resolve attachments must either be empty or there must be a entry for each color attachment 
		assert(result.mOrderedResolveAttachmentRefs.size() == 0
			|| result.mOrderedResolveAttachmentRefs.size() == result.mOrderedColorAttachmentRefs.size());
		// - There must not be more than 1 depth/stencil attachements
		assert(result.mOrderedDepthAttachmentRefs.size() <= 1);

		// 4. Now we can fill the subpass description
		result.mSubpasses.push_back(vk::SubpassDescription()
			// pipelineBindPoint must be VK_PIPELINE_BIND_POINT_GRAPHICS [1] because subpasses are only relevant for graphics at the moment
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(static_cast<uint32_t>(result.mOrderedColorAttachmentRefs.size()))
			.setPColorAttachments(result.mOrderedColorAttachmentRefs.data())
			// If pResolveAttachments is not NULL, each of its elements corresponds to a color attachment 
			//  (the element in pColorAttachments at the same index), and a multisample resolve operation 
			//  is defined for each attachment. [1]
			.setPResolveAttachments(result.mOrderedResolveAttachmentRefs.size() == 0 ? nullptr : result.mOrderedResolveAttachmentRefs.data())
			// If pDepthStencilAttachment is NULL, or if its attachment index is VK_ATTACHMENT_UNUSED, it 
			//  indicates that no depth/stencil attachment will be used in the subpass. [1]
			.setPDepthStencilAttachment(result.mOrderedDepthAttachmentRefs.size() == 0 ? nullptr : &result.mOrderedDepthAttachmentRefs[0])
			// The following two attachment types are probably totally irrelevant if we only have one subpass
			// TODO: support more subpasses!
			.setInputAttachmentCount(static_cast<uint32_t>(result.mOrderedInputAttachmentRefs.size()))
			.setPInputAttachments(result.mOrderedInputAttachmentRefs.data())
			.setPreserveAttachmentCount(0u)
			.setPPreserveAttachments(nullptr));
		
		// ======== Regarding Subpass Dependencies ==========
		// At this point, we can not know how a subpass shall 
		// be synchronized exactly with whatever comes before
		// and whatever comes after. 
		//  => Let's establish very (overly) cautious dependencies to ensure correctness: // TODO: Let user do some sync here! This should be similar to cgb::sync

		{
			renderpass_sync syncBefore {renderpass_sync::sExternal, 0,
				pipeline_stage::all_commands,			memory_access::any_write_access,
				pipeline_stage::all_graphics_stages,	memory_access::any_graphics_read_access | memory_access::any_graphics_fundamental_write_access
			};
			// Let the user modify this sync
			if (aSync) {
				aSync(syncBefore);
			}
			
			result.mSubpassDependencies.push_back(vk::SubpassDependency()
				// Between which two subpasses is this dependency:
				.setSrcSubpass(VK_SUBPASS_EXTERNAL)
				.setDstSubpass(0u)
				// Which stage from whatever comes before are we waiting on, and which operations from whatever comes before are we waiting on:
				.setSrcStageMask(to_vk_pipeline_stage_flags(syncBefore.mSourceStage))
				.setSrcAccessMask(to_vk_access_flags(to_memory_access(syncBefore.mSourceMemoryDependency)))
				// Which stage and which operations of our subpass ZERO shall wait:
				.setDstStageMask(to_vk_pipeline_stage_flags(syncBefore.mDestinationStage))
				.setDstAccessMask(to_vk_access_flags(syncBefore.mDestinationMemoryDependency))
			);
		}

		{
			renderpass_sync syncAfter {0, renderpass_sync::sExternal, // TODO: Not 0!
				pipeline_stage::all_graphics_stages,	memory_access::any_graphics_fundamental_write_access,
				pipeline_stage::all_commands,			memory_access::any_read_access
			};
			// Let the user modify this sync
			if (aSync) {
				aSync(syncAfter);
			}
			
			result.mSubpassDependencies.push_back(vk::SubpassDependency()
				// Between which two subpasses is this dependency:
				.setSrcSubpass(0u)
				.setDstSubpass(VK_SUBPASS_EXTERNAL)
				// Which stage and which operations of our subpass ZERO shall be waited on by whatever comes after:
				.setSrcStageMask(to_vk_pipeline_stage_flags(syncAfter.mSourceStage))
				.setSrcAccessMask(to_vk_access_flags(to_memory_access(syncAfter.mSourceMemoryDependency)))
				// Which stage of whatever comes after shall wait, and which operations from whatever comes after shall wait:
				.setDstStageMask(to_vk_pipeline_stage_flags(syncAfter.mDestinationStage))
				.setDstAccessMask(to_vk_access_flags(syncAfter.mDestinationMemoryDependency))
			);
		}

		// Maybe alter the config?!
		if (pAlterConfigBeforeCreation.mFunction) {
			pAlterConfigBeforeCreation.mFunction(result);
		}

		// Finally, create the render pass
		auto createInfo = vk::RenderPassCreateInfo()
			.setAttachmentCount(static_cast<uint32_t>(result.mAttachmentDescriptions.size()))
			.setPAttachments(result.mAttachmentDescriptions.data())
			.setSubpassCount(static_cast<uint32_t>(result.mSubpasses.size()))
			.setPSubpasses(result.mSubpasses.data())
			.setDependencyCount(static_cast<uint32_t>(result.mSubpassDependencies.size()))
			.setPDependencies(result.mSubpassDependencies.data());
		result.mRenderPass = context().logical_device().createRenderPassUnique(createInfo);
		//result.mTracker.setTrackee(result);
		return result; 

		// TODO: Support VkSubpassDescriptionDepthStencilResolveKHR in order to enable resolve-settings for the depth attachment (see [1] and [2] for more details)
		
		// References:
		// [1] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkSubpassDescription.html
		// [2] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkSubpassDescriptionDepthStencilResolveKHR.html
		// [3] https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineStageFlagBits.html
	}

	bool renderpass_t::is_color_attachment(size_t _AttachmentIndex) const
	{
		assert(_AttachmentIndex < mAttachmentDescriptions.size());
		for (auto& colorAtt : mOrderedColorAttachmentRefs) {
			if (colorAtt.attachment == static_cast<uint32_t>(_AttachmentIndex)) {
				return true;
			}
		}
		return false;
	}

	bool renderpass_t::is_depth_attachment(size_t _AttachmentIndex) const
	{
		assert(_AttachmentIndex < mAttachmentDescriptions.size());
		for (auto& colorAtt : mOrderedDepthAttachmentRefs) {
			if (colorAtt.attachment == static_cast<uint32_t>(_AttachmentIndex)) {
				return true;
			}
		}
		return false;
	}

	bool renderpass_t::is_resolve_attachment(size_t _AttachmentIndex) const
	{
		assert(_AttachmentIndex < mAttachmentDescriptions.size());
		for (auto& colorAtt : mOrderedResolveAttachmentRefs) {
			if (colorAtt.attachment == static_cast<uint32_t>(_AttachmentIndex)) {
				return true;
			}
		}
		return false;
	}

	bool renderpass_t::is_input_attachment(size_t _AttachmentIndex) const
	{
		assert(_AttachmentIndex < mAttachmentDescriptions.size());
		for (auto& colorAtt : mOrderedInputAttachmentRefs) {
			if (colorAtt.attachment == static_cast<uint32_t>(_AttachmentIndex)) {
				return true;
			}
		}
		return false;
	}

}

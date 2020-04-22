#include <cg_base.hpp>

namespace cgb
{
	using namespace cpplinq;

	struct subpass_desc_helper
	{
		size_t mSubpassId;
		std::map<uint32_t, vk::AttachmentReference> mSpecificInputLocations;
		std::queue<vk::AttachmentReference> mUnspecifiedInputLocations;
		int mInputMaxLoc;
		std::map<uint32_t, vk::AttachmentReference> mSpecificColorLocations;
		std::queue<vk::AttachmentReference> mUnspecifiedColorLocations;
		int mColorMaxLoc;
		std::map<uint32_t, vk::AttachmentReference> mSpecificDepthStencilLocations;
		std::queue<vk::AttachmentReference> mUnspecifiedDepthStencilLocations;
		int mDepthStencilMaxLoc;
		std::map<uint32_t, vk::AttachmentReference> mSpecificResolveLocations;
		std::queue<vk::AttachmentReference> mUnspecifiedResolveLocations;
		std::vector<uint32_t> mPreserveAttachments;
	};

	owning_resource<renderpass_t> renderpass_t::create(std::vector<attachment> aAttachments, std::function<void(renderpass_sync&)> aSync, cgb::context_specific_function<void(renderpass_t&)> aAlterConfigBeforeCreation)
	{
		renderpass_t result;

		std::vector<subpass_desc_helper> subpasses;
		
		if (aAttachments.empty()) {
			throw cgb::runtime_error("No attachments have been passed to the creation of a renderpass.");
		}
		const auto numSubpassesFirst = aAttachments.front().mSubpassUsages.num_subpasses();
		// All further attachments must have the same number of subpasses! It will be checked.
		subpasses.reserve(numSubpassesFirst);
		for (size_t i = 0; i < numSubpassesFirst; ++i) {
			auto& a = subpasses.emplace_back();
			a.mSubpassId = i;
			a.mInputMaxLoc = -1;
			a.mColorMaxLoc = -1;
			a.mDepthStencilMaxLoc = -1;
		}

		result.mAttachmentDescriptions.reserve(aAttachments.size());
		for (const auto& a : aAttachments) {
			// Try to infer initial and final image layouts (If this isn't cool => user must use aAlterConfigBeforeCreation)
			vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
			vk::ImageLayout finalLayout = vk::ImageLayout::eUndefined;

			const auto isLoad = att::on_load::load == a.mLoadOperation;
			const auto isClear = att::on_load::clear == a.mLoadOperation;
			const auto isStore  = att::on_store::store == a.mStoreOperation || att::on_store::store_in_presentable_format == a.mStoreOperation;
			const auto makePresentable = att::on_store::store_in_presentable_format == a.mStoreOperation;
			
			const auto hasSeparateStencilLoad = a.mStencilLoadOperation.has_value();
			const auto hasSeparateStencilStore = a.mStencilStoreOperation.has_value();
			const auto isStencilLoad = att::on_load::load == a.get_stencil_load_op();
			const auto isStencilClear = att::on_load::clear == a.get_stencil_load_op();
			const auto isStencilStore  = att::on_store::store == a.get_stencil_store_op() || att::on_store::store_in_presentable_format == a.get_stencil_store_op();
			const auto makeStencilPresentable = att::on_store::store_in_presentable_format == a.get_stencil_store_op();
			const auto hasStencilComponent = has_stencil_component(a.format());

			switch (a.get_first_usage_type()) {
			case att::usage_type::unused:
				break;
			case att::usage_type::input:
				if (isLoad) {
					initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				}
				break;
			case att::usage_type::color:
				if (isLoad) {
					initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
				}
				break;
			case att::usage_type::depth_stencil:
				if (isLoad) {
					initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
					{
						// TODO: Set other depth/stencil-specific formats
						//       - vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal
						//       - vk::ImageLayout::eDepthStencilReadOnlyOptimal
						//       - vk::ImageLayout::eDepthReadOnlyOptimal
						//       - vk::ImageLayout::eStencilAttachmentOptimal
						//       - vk::ImageLayout::eStencilReadOnlyOptimal
					}
				}
				break;
			case att::usage_type::preserve:
				break;
			}

			switch (a.get_last_usage_type()) {
			case att::usage_type::unused:
				// We can just guess:
				LOG_WARNING("Consider specifying image_usage flags for renderpass attachments that are used as 'unused' attachments in the last subpass.");
				finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				break;
			case att::usage_type::input:
				finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				break;
			case att::usage_type::color:
				finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
				break;
			case att::usage_type::depth_stencil:
				finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
				{
					// TODO: Set other depth/stencil-specific formats
					//       - vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal
					//       - vk::ImageLayout::eDepthStencilReadOnlyOptimal
					//       - vk::ImageLayout::eDepthReadOnlyOptimal
					//       - vk::ImageLayout::eStencilAttachmentOptimal
					//       - vk::ImageLayout::eStencilReadOnlyOptimal
				}
				break;
			case att::usage_type::preserve:
				// We can just guess:
				LOG_WARNING("Consider specifying image_usage flags for renderpass attachments that are used as 'preserve' attachments in the last subpass.");
				finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				break;
			}

			if (a.mImageUsageHintBefore.has_value()) {
				// If we detect the image usage to be more generic, we should change the layout to something more generic
				if (cgb::has_flag(a.mImageUsageHintBefore.value(), cgb::image_usage::sampled)) {
					initialLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				}
				if (cgb::has_flag(a.mImageUsageHintBefore.value(), cgb::image_usage::shader_storage)) {
					initialLayout = vk::ImageLayout::eGeneral;
				}
			}
			if (a.mImageUsageHintAfter.has_value()) {
				// If we detect the image usage to be more generic, we should change the layout to something more generic
				if (cgb::has_flag(a.mImageUsageHintAfter.value(), cgb::image_usage::sampled)) {
					finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
				}
				if (cgb::has_flag(a.mImageUsageHintAfter.value(), cgb::image_usage::shader_storage)) {
					finalLayout = vk::ImageLayout::eGeneral;
				}
			}
			
			if (a.shall_be_presentable()) {
				finalLayout = vk::ImageLayout::ePresentSrcKHR;
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
			
			const auto attachmentIndex = static_cast<uint32_t>(result.mAttachmentDescriptions.size() - 1); // Index of this attachment as used in the further subpasses

			// 2. Go throught the subpasses and gather data for subpass config
			const auto nSubpasses = a.mSubpassUsages.num_subpasses();
			if (nSubpasses != numSubpassesFirst) {
				throw cgb::runtime_error("All attachments must have the exact same number of subpasses!");
			}

			// Determine and fill clear values:
			assert(result.mAttachmentDescriptions.size() == result.mClearValues.size() + 1);
			size_t spId = 0;
			while (result.mAttachmentDescriptions.size() != result.mClearValues.size() && spId < nSubpasses) {
				switch (a.mSubpassUsages.get_subpass_usage(spId)) {
				case att::usage_type::unused: break;
				case att::usage_type::input: break;
				case att::usage_type::color:
					result.mClearValues.emplace_back(vk::ClearColorValue{ *reinterpret_cast<const std::array<float, 4>*>(glm::value_ptr(a.clear_color())) });
					break;
				case att::usage_type::depth_stencil:
					result.mClearValues.emplace_back(vk::ClearDepthStencilValue{ a.depth_clear_value(), a.stencil_clear_value() });
					break;
				case att::usage_type::preserve: break;
				default: break;
				}
				++spId;
			}
			if (result.mAttachmentDescriptions.size() != result.mClearValues.size() ) {
				result.mClearValues.emplace_back(); // just an empty clear value
			}
			assert(result.mAttachmentDescriptions.size() == result.mClearValues.size());
			
			for (size_t i = 0; i < nSubpasses; ++i) {
				auto& sp = subpasses[i];
				const auto hasLoc = a.mSubpassUsages.has_layout_at_subpass(i);
				const auto loc = static_cast<int>(a.mSubpassUsages.layout_at_subpass(i));
				const auto resolve = a.mSubpassUsages.is_to_be_resolved_after_subpass(i);
				switch (a.mSubpassUsages.get_subpass_usage(i)) {
				case att::usage_type::unused:
					// nothing to do here
					break;
				case att::usage_type::input:
					assert(!a.mSubpassUsages.is_to_be_resolved_after_subpass(i)); // Can not resolve input attachments
					if (hasLoc) {
						if (sp.mSpecificInputLocations.count(loc) != 0) {
							throw cgb::runtime_error(fmt::format("Layout location {} is used multiple times for an input attachments in subpass {}. This is not allowed.", loc, i));
						}
						sp.mSpecificInputLocations[loc] = vk::AttachmentReference{attachmentIndex, vk::ImageLayout::eShaderReadOnlyOptimal};
						sp.mInputMaxLoc = std::max(sp.mInputMaxLoc, loc);
					}
					else {
						LOG_WARNING(fmt::format("No layout location is specified for an input attachment in subpass {}. This might be problematic. Consider declaring it 'unused'.", i));
						sp.mUnspecifiedInputLocations.push(vk::AttachmentReference{attachmentIndex, vk::ImageLayout::eShaderReadOnlyOptimal});
					}
					break;
				case att::usage_type::color:
					if (hasLoc) {
						if (sp.mSpecificColorLocations.count(loc) != 0) {
							throw cgb::runtime_error(fmt::format("Layout location {} is used multiple times for a color attachments in subpass {}. This is not allowed.", loc, i));
						}
						sp.mSpecificColorLocations[loc] =	 vk::AttachmentReference{attachmentIndex,									vk::ImageLayout::eColorAttachmentOptimal};
						sp.mSpecificResolveLocations[loc] =	 vk::AttachmentReference{resolve ? a.mSubpassUsages.get_resolve_target_index(i) : VK_ATTACHMENT_UNUSED,	vk::ImageLayout::eColorAttachmentOptimal};
						sp.mColorMaxLoc = std::max(sp.mColorMaxLoc, loc);
					}
					else {
						LOG_WARNING(fmt::format("No layout location is specified for a color attachment in subpass {}. This might be problematic. Consider declaring it 'unused'.", i));
						sp.mUnspecifiedColorLocations.push(	 vk::AttachmentReference{attachmentIndex,									vk::ImageLayout::eColorAttachmentOptimal});
						sp.mUnspecifiedResolveLocations.push(vk::AttachmentReference{resolve ? a.mSubpassUsages.get_resolve_target_index(i) : VK_ATTACHMENT_UNUSED,	vk::ImageLayout::eColorAttachmentOptimal});
					}
					break;
				case att::usage_type::depth_stencil:
					assert(!a.mSubpassUsages.is_to_be_resolved_after_subpass(i)); // TODO: Support depth/stencil resolve by using VkSubpassDescription2
					if (hasLoc) {
						if (sp.mSpecificDepthStencilLocations.count(loc) != 0) {
							throw cgb::runtime_error(fmt::format("Layout location {} is used multiple times for a depth/stencil attachments in subpass {}. This is not allowed.", loc, i));
						}
						sp.mSpecificDepthStencilLocations[loc] = vk::AttachmentReference{attachmentIndex, vk::ImageLayout::eDepthStencilAttachmentOptimal};
						sp.mDepthStencilMaxLoc = std::max(sp.mDepthStencilMaxLoc, loc);
					}
					else {
						LOG_WARNING(fmt::format("No layout location is specified for a depth/stencil attachment in subpass {}. This might be problematic. Consider declaring it 'unused'.", i));
						sp.mUnspecifiedDepthStencilLocations.push(vk::AttachmentReference{attachmentIndex, vk::ImageLayout::eDepthStencilAttachmentOptimal});
					}
					break;
				case att::usage_type::preserve:
					assert(!a.mSubpassUsages.is_to_be_resolved_after_subpass(i)); // Can not resolve preserve attachments
					sp.mPreserveAttachments.push_back(attachmentIndex);
					break;
				default:
					assert(false);
					throw cgb::logic_error("How did we end up here?");
				}
			}
		}

		// 3. Fill all the vectors in the right order:
		const auto unusedAttachmentRef = vk::AttachmentReference().setAttachment(VK_ATTACHMENT_UNUSED);
		result.mSubpassData.reserve(numSubpassesFirst);
		for (size_t i = 0; i < numSubpassesFirst; ++i) {
			auto& a = subpasses[i];
			auto& b = result.mSubpassData.emplace_back();
			assert(result.mSubpassData.size() == i + 1);
			// INPUT ATTACHMENTS
			for (int loc = 0; loc <= a.mInputMaxLoc || !a.mUnspecifiedInputLocations.empty(); ++loc) {
				if (a.mSpecificInputLocations.count(loc) > 0) {
					assert (a.mSpecificInputLocations.count(loc) == 1);
					b.mOrderedInputAttachmentRefs.push_back(a.mSpecificInputLocations[loc]);
				}
				else {
					if (!a.mUnspecifiedInputLocations.empty()) {
						b.mOrderedInputAttachmentRefs.push_back(a.mUnspecifiedInputLocations.front());
						a.mUnspecifiedInputLocations.pop();
					}
					else {
						b.mOrderedInputAttachmentRefs.push_back(unusedAttachmentRef);
					}
				}
			}
			// COLOR ATTACHMENTS
			for (int loc = 0; loc <= a.mColorMaxLoc || !a.mUnspecifiedColorLocations.empty(); ++loc) {
				if (a.mSpecificColorLocations.count(loc) > 0) {
					assert (a.mSpecificColorLocations.count(loc) == 1);
					assert (a.mSpecificResolveLocations.count(loc) == 1);
					b.mOrderedColorAttachmentRefs.push_back(a.mSpecificColorLocations[loc]);
					b.mOrderedResolveAttachmentRefs.push_back(a.mSpecificResolveLocations[loc]);
				}
				else {
					if (!a.mUnspecifiedColorLocations.empty()) {
						assert(a.mUnspecifiedColorLocations.size() == a.mUnspecifiedResolveLocations.size());
						b.mOrderedColorAttachmentRefs.push_back(a.mUnspecifiedColorLocations.front());
						a.mUnspecifiedColorLocations.pop();
						b.mOrderedResolveAttachmentRefs.push_back(a.mUnspecifiedResolveLocations.front());
						a.mUnspecifiedResolveLocations.pop();
					}
					else {
						b.mOrderedColorAttachmentRefs.push_back(unusedAttachmentRef);
						b.mOrderedResolveAttachmentRefs.push_back(unusedAttachmentRef);
					}
				}
			}
			// DEPTH/STENCIL ATTACHMENTS
			for (int loc = 0; loc <= a.mDepthStencilMaxLoc || !a.mUnspecifiedDepthStencilLocations.empty(); ++loc) {
				if (a.mSpecificDepthStencilLocations.count(loc) > 0) {
					assert (a.mSpecificDepthStencilLocations.count(loc) == 1);
					b.mOrderedDepthStencilAttachmentRefs.push_back(a.mSpecificDepthStencilLocations[loc]);
				}
				else {
					if (!a.mUnspecifiedDepthStencilLocations.empty()) {
						b.mOrderedDepthStencilAttachmentRefs.push_back(a.mUnspecifiedDepthStencilLocations.front());
						a.mUnspecifiedDepthStencilLocations.pop();
					}
					else {
						b.mOrderedDepthStencilAttachmentRefs.push_back(unusedAttachmentRef);
					}
				}
			}
			b.mPreserveAttachments = std::move(a.mPreserveAttachments);
			
			// SOME SANITY CHECKS:
			// - The resolve attachments must either be empty or there must be a entry for each color attachment 
			assert(b.mOrderedResolveAttachmentRefs.empty() || b.mOrderedResolveAttachmentRefs.size() == b.mOrderedColorAttachmentRefs.size());
			// - There must not be more than 1 depth/stencil attachements
			assert(b.mOrderedDepthStencilAttachmentRefs.size() <= 1);
		}

		// Done with the helper structure:
		subpasses.clear();
		
		// 4. Now we can fill the subpass description
		result.mSubpasses.reserve(numSubpassesFirst);
		for (size_t i = 0; i < numSubpassesFirst; ++i) {
			auto& b = result.mSubpassData[i];
			
			result.mSubpasses.push_back(vk::SubpassDescription()
				// pipelineBindPoint must be VK_PIPELINE_BIND_POINT_GRAPHICS [1] because subpasses are only relevant for graphics at the moment
				.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				.setColorAttachmentCount(static_cast<uint32_t>(b.mOrderedColorAttachmentRefs.size()))
				.setPColorAttachments(b.mOrderedColorAttachmentRefs.data())
				// If pResolveAttachments is not NULL, each of its elements corresponds to a color attachment 
				//  (the element in pColorAttachments at the same index), and a multisample resolve operation 
				//  is defined for each attachment. [1]
				.setPResolveAttachments(b.mOrderedResolveAttachmentRefs.size() == 0 ? nullptr : b.mOrderedResolveAttachmentRefs.data())
				// If pDepthStencilAttachment is NULL, or if its attachment index is VK_ATTACHMENT_UNUSED, it 
				//  indicates that no depth/stencil attachment will be used in the subpass. [1]
				.setPDepthStencilAttachment(b.mOrderedDepthStencilAttachmentRefs.size() == 0 ? nullptr : &b.mOrderedDepthStencilAttachmentRefs[0])
				// The following two attachment types are probably totally irrelevant if we only have one subpass
				.setInputAttachmentCount(static_cast<uint32_t>(b.mOrderedInputAttachmentRefs.size()))
				.setPInputAttachments(b.mOrderedInputAttachmentRefs.data())
				.setPreserveAttachmentCount(static_cast<uint32_t>(b.mPreserveAttachments.size()))
				.setPPreserveAttachments(b.mPreserveAttachments.data()));
		}
		
		// ======== Regarding Subpass Dependencies ==========
		// At this point, we can not know how a subpass shall 
		// be synchronized exactly with whatever comes before
		// and whatever comes after. 
		//  => Let's establish very (overly) cautious dependencies to ensure correctness, but user can set more tight sync via the callback

		const uint32_t firstSubpassId = 0u;
		const uint32_t lastSubpassId = numSubpassesFirst - 1;
		const auto addDependency = [&result](renderpass_sync& rps){
			result.mSubpassDependencies.push_back(vk::SubpassDependency()
				// Between which two subpasses is this dependency:
				.setSrcSubpass(rps.source_vk_subpass_id())
				.setDstSubpass(rps.destination_vk_subpass_id())
				// Which stage from whatever comes before are we waiting on, and which operations from whatever comes before are we waiting on:
				.setSrcStageMask(to_vk_pipeline_stage_flags(rps.mSourceStage))
				.setSrcAccessMask(to_vk_access_flags(to_memory_access(rps.mSourceMemoryDependency)))
				// Which stage and which operations of our subpass ZERO shall wait:
				.setDstStageMask(to_vk_pipeline_stage_flags(rps.mDestinationStage))
				.setDstAccessMask(to_vk_access_flags(rps.mDestinationMemoryDependency))
			);
		};
		
		{
			renderpass_sync syncBefore {renderpass_sync::sExternal, static_cast<int>(firstSubpassId),
				pipeline_stage::all_commands,			memory_access::any_write_access,
				pipeline_stage::all_graphics_stages,	memory_access::any_graphics_read_access | memory_access::any_graphics_basic_write_access
			};
			// Let the user modify this sync
			if (aSync) {
				aSync(syncBefore);
			}
			assert(syncBefore.source_vk_subpass_id() == VK_SUBPASS_EXTERNAL);
			assert(syncBefore.destination_vk_subpass_id() == 0u);
			addDependency(syncBefore);
		}

		for (auto i = firstSubpassId + 1; i <= lastSubpassId; ++i) {
			auto prevSubpassId = i - 1;
			auto nextSubpassId = i;
			renderpass_sync syncBetween {static_cast<int>(prevSubpassId), static_cast<int>(nextSubpassId),
				pipeline_stage::all_graphics_stages,	memory_access::any_graphics_basic_write_access,
				pipeline_stage::all_graphics_stages,	memory_access::any_graphics_read_access | memory_access::any_graphics_basic_write_access,
			};
			// Let the user modify this sync
			if (aSync) {
				aSync(syncBetween);
			}
			assert(syncBetween.source_vk_subpass_id() == prevSubpassId);
			assert(syncBetween.destination_vk_subpass_id() == nextSubpassId);
			addDependency(syncBetween);
		}

		{
			renderpass_sync syncAfter {static_cast<int>(lastSubpassId), renderpass_sync::sExternal,
				pipeline_stage::all_graphics_stages,	memory_access::any_graphics_basic_write_access,
				pipeline_stage::all_commands,			memory_access::any_read_access
			};
			// Let the user modify this sync
			if (aSync) {
				aSync(syncAfter);
			}
			assert(syncAfter.source_vk_subpass_id() == lastSubpassId);
			assert(syncAfter.destination_vk_subpass_id() == VK_SUBPASS_EXTERNAL);
			addDependency(syncAfter);
		}

		assert(result.mSubpassDependencies.size() == numSubpassesFirst + 1);

		// Maybe alter the config?!
		if (aAlterConfigBeforeCreation.mFunction) {
			aAlterConfigBeforeCreation.mFunction(result);
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

	bool renderpass_t::is_input_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedInputAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedInputAttachmentRefs), std::end(b.mOrderedInputAttachmentRefs), 
			[aAttachmentIndex](const vk::AttachmentReference& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_color_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedColorAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedColorAttachmentRefs), std::end(b.mOrderedColorAttachmentRefs), 
			[aAttachmentIndex](const vk::AttachmentReference& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_depth_stencil_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedDepthStencilAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedDepthStencilAttachmentRefs), std::end(b.mOrderedDepthStencilAttachmentRefs), 
			[aAttachmentIndex](const vk::AttachmentReference& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_resolve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mOrderedResolveAttachmentRefs.end() != std::find_if(std::begin(b.mOrderedResolveAttachmentRefs), std::end(b.mOrderedResolveAttachmentRefs), 
			[aAttachmentIndex](const vk::AttachmentReference& ref) { return ref.attachment == aAttachmentIndex; });
	}

	bool renderpass_t::is_preserve_attachment(uint32_t aSubpassId, size_t aAttachmentIndex) const
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		assert(aAttachmentIndex < mAttachmentDescriptions.size());
		return b.mPreserveAttachments.end() != std::find_if(std::begin(b.mPreserveAttachments), std::end(b.mPreserveAttachments), 
			[aAttachmentIndex](uint32_t idx) { return idx == aAttachmentIndex; });
	}

	const std::vector<vk::AttachmentReference>& renderpass_t::input_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedInputAttachmentRefs;
	}
	
	const std::vector<vk::AttachmentReference>& renderpass_t::color_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedColorAttachmentRefs;
	}
	
	const std::vector<vk::AttachmentReference>& renderpass_t::depth_stencil_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedDepthStencilAttachmentRefs;
	}
	
	const std::vector<vk::AttachmentReference>& renderpass_t::resolve_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mOrderedResolveAttachmentRefs;
	}
	
	const std::vector<uint32_t>& renderpass_t::preserve_attachments_for_subpass(uint32_t aSubpassId)
	{
		assert(aSubpassId < mSubpassData.size());
		auto& b = mSubpassData[aSubpassId];
		return b.mPreserveAttachments;
	}
	
	
}

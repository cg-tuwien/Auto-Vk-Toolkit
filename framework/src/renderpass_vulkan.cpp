namespace cgb
{
	using namespace cpplinq;


	//owning_resource<renderpass_t> renderpass_t::create_good_renderpass(VkFormat format)
	//{

	//		VkAttachmentDescription colorAttachment = {};
 //       colorAttachment.format = format;
 //       colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
 //       colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
 //       colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
 //       colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
 //       colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
 //       colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
 //       colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

 //       VkAttachmentReference colorAttachmentRef = {};
 //       colorAttachmentRef.attachment = 0;
 //       colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

 //       VkSubpassDescription subpass = {};
 //       subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
 //       subpass.colorAttachmentCount = 1;
 //       subpass.pColorAttachments = &colorAttachmentRef;

 //       VkSubpassDependency dependency = {};
 //       dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
 //       dependency.dstSubpass = 0;
 //       dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
 //       dependency.srcAccessMask = 0;
 //       dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
 //       dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

 //       VkRenderPassCreateInfo renderPassInfo = {};
 //       renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
 //       renderPassInfo.attachmentCount = 1;
 //       renderPassInfo.pAttachments = &colorAttachment;
 //       renderPassInfo.subpassCount = 1;
 //       renderPassInfo.pSubpasses = &subpass;
 //       renderPassInfo.dependencyCount = 1;
 //       renderPassInfo.pDependencies = &dependency;

	//	renderpass_t asdf;
	//	asdf.mAttachmentDescriptions.push_back(colorAttachment);
	//	asdf.mOrderedColorAttachmentRefs.push_back(colorAttachmentRef);
	//	asdf.subpasses().push_back(subpass);
	//	asdf.subpass_dependencies().push_back(dependency);
	//	asdf.mRenderPass = cgb::context().logical_device().createRenderPassUnique(renderPassInfo);
	//	return asdf;
	//}

	owning_resource<renderpass_t> renderpass_t::create(std::vector<attachment> pAttachments, cgb::context_specific_function<void(renderpass_t&)> pAlterConfigBeforeCreation)
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
			// 1. Create the attachment descriptions
			result.mAttachmentDescriptions.push_back(vk::AttachmentDescription()
				.setFormat(a.format().mFormat)
				.setSamples(to_vk_sample_count(a.sample_count()))
				// At this point, we can not really make a guess about load/store ops, so.. don't care:
				.setLoadOp(to_vk_load_op(a.mLoadOperation))
				.setStoreOp(to_vk_store_op(a.mStoreOperation))
				// Just set the same load/store ops for the stencil?!
				.setStencilLoadOp(to_vk_load_op(a.mLoadOperation))
				.setStencilStoreOp(to_vk_store_op(a.mStoreOperation))
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout( // The layout is a bit of an (educated) guess:
					a.is_depth_attachment()
					? vk::ImageLayout::eDepthStencilAttachmentOptimal
					: (a.is_to_be_presented() // => not depth => assume color, but what about presenting?
						? vk::ImageLayout::ePresentSrcKHR
						: vk::ImageLayout::eColorAttachmentOptimal))
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
			if (a.is_depth_attachment()) { //< 2.2. DEPTH ATTACHMENT
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
				auto attachmentRef = vk::AttachmentReference().setAttachment(attachmentIndex).setLayout(vk::ImageLayout::eShaderReadOnlyOptimal); // TODO: This layout?
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
		//  => Let's establish very (overly) cautious dependencies to ensure correctness:

		result.mSubpassDependencies.push_back(vk::SubpassDependency()
			// Between which two subpasses is this dependency:
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0u)
			// Which stage from whatever comes before are we waiting on, and which operations from whatever comes before are we waiting on:
			.setSrcStageMask(vk::PipelineStageFlagBits::eAllGraphics) // eAllGraphics contains (among others) eColorAttachmentOutput [3] (this one in particular to wait for the swap chain)
			.setSrcAccessMask(vk::AccessFlagBits::eInputAttachmentRead 
							| vk::AccessFlagBits::eColorAttachmentRead
							| vk::AccessFlagBits::eColorAttachmentWrite
							| vk::AccessFlagBits::eDepthStencilAttachmentRead
							| vk::AccessFlagBits::eDepthStencilAttachmentWrite)
			// Which stage and which operations of our subpass ZERO shall wait:
			.setDstStageMask(vk::PipelineStageFlagBits::eAllGraphics)
			.setDstAccessMask(vk::AccessFlagBits::eInputAttachmentRead 
							| vk::AccessFlagBits::eColorAttachmentRead
							| vk::AccessFlagBits::eColorAttachmentWrite
							| vk::AccessFlagBits::eDepthStencilAttachmentRead
							| vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		);

		result.mSubpassDependencies.push_back(vk::SubpassDependency()
			// Between which two subpasses is this dependency:
			.setSrcSubpass(0u)
			.setDstSubpass(VK_SUBPASS_EXTERNAL)
			// Which stage and which operations of our subpass ZERO shall be waited on by whatever comes after:
			.setSrcStageMask(vk::PipelineStageFlagBits::eAllGraphics) // Super cautious, actually eColorAttachmentOutput (which is included in eAllGraphics [3]) should sufficee
			.setSrcAccessMask(vk::AccessFlagBits::eInputAttachmentRead 
							| vk::AccessFlagBits::eColorAttachmentRead
							| vk::AccessFlagBits::eColorAttachmentWrite
							| vk::AccessFlagBits::eDepthStencilAttachmentRead
							| vk::AccessFlagBits::eDepthStencilAttachmentWrite)
			// Which stage of whatever comes after shall wait, and which operations from whatever comes after shall wait:
			.setDstStageMask(vk::PipelineStageFlagBits::eAllGraphics) 
			.setDstAccessMask(vk::AccessFlagBits::eInputAttachmentRead 
							| vk::AccessFlagBits::eColorAttachmentRead
							| vk::AccessFlagBits::eColorAttachmentWrite
							| vk::AccessFlagBits::eDepthStencilAttachmentRead
							| vk::AccessFlagBits::eDepthStencilAttachmentWrite)
		);

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
}

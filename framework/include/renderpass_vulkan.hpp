#pragma once

namespace cgb
{
	/** A class which represents a Vulkan renderpass and all its attachments,
	 *	subpasses and subpass dependencies
	 */
	class renderpass_t
	{
	public:
		renderpass_t() = default;
		renderpass_t(renderpass_t&&) = default;
		renderpass_t(const renderpass_t&) = delete;
		renderpass_t& operator=(renderpass_t&&) = default;
		renderpass_t& operator=(const renderpass_t&) = delete;
		~renderpass_t() = default;

		/** Create a renderpass from a given set of attachments.
		 *	Also, create default subpass dependencies 
		 *	(which are overly cautious and potentially sync more as required.)
		 */
		static owning_resource<renderpass_t> create(std::vector<attachment> pAttachments, cgb::context_specific_function<void(renderpass_t&)> pAlterConfigBeforeCreation = {});
		//static owning_resource<renderpass_t> create_good_renderpass(VkFormat format);

		const auto& attachment_descriptions() const { return mAttachmentDescriptions; }
		const auto& color_attachments() const { return mOrderedColorAttachmentRefs; }
		const auto& depth_attachments() const { return mOrderedDepthAttachmentRefs; }
		const auto& resolve_attachments() const { return mOrderedResolveAttachmentRefs; }
		const auto& input_attachments() const { return mOrderedInputAttachmentRefs; }
		const auto& subpasses() const { return mSubpasses; }
		const auto& subpass_dependencies() const { return mSubpassDependencies; }

		auto& attachment_descriptions() { return mAttachmentDescriptions; }
		auto& color_attachments() { return mOrderedColorAttachmentRefs; }
		auto& depth_attachments() { return mOrderedDepthAttachmentRefs; }
		auto& resolve_attachments() { return mOrderedResolveAttachmentRefs; }
		auto& input_attachments() { return mOrderedInputAttachmentRefs; }
		auto& subpasses() { return mSubpasses; }
		auto& subpass_dependencies() { return mSubpassDependencies; }

		const auto& handle() const { return mRenderPass.get(); }

	private:
		// All the attachments to this renderpass
		std::vector<vk::AttachmentDescription> mAttachmentDescriptions;

		// The ordered list of color attachments (ordered by shader-location).
		std::vector<vk::AttachmentReference> mOrderedColorAttachmentRefs;

		// The ordered list of depth attachments. Actually, only one or zero are supported.
		std::vector<vk::AttachmentReference> mOrderedDepthAttachmentRefs;

		// The ordered list of attachments that shall be resolved.
		// The length of this list must be zero or the same length as the color attachments.
		std::vector<vk::AttachmentReference> mOrderedResolveAttachmentRefs;

		// Ordered list of input attachments
		std::vector<vk::AttachmentReference> mOrderedInputAttachmentRefs;

		// Subpasses
		std::vector<vk::SubpassDescription> mSubpasses;

		// Dependencies between internal and external subpasses
		std::vector<vk::SubpassDependency> mSubpassDependencies;

		// The native handle
		vk::UniqueRenderPass mRenderPass;

		context_tracker<renderpass_t> mTracker;
	};

	using renderpass = owning_resource<renderpass_t>;
	

	extern bool are_compatible(const renderpass& first, const renderpass& second);

	//// Helper function:
	//template <typename V>
	//void insert_at_first_unused_location_or_push_back(V& pCollection, uint32_t pAttachment, vk::ImageLayout pImageLayout)
	//{
	//	auto newElement = vk::AttachmentReference{}
	//		.setAttachment(pAttachment)
	//		.setLayout(pImageLayout);

	//	size_t n = pCollection.size();
	//	for (size_t i = 0; i < n; ++i) {
	//		if (pCollection[i].attachment == VK_ATTACHMENT_UNUSED) {
	//			// Found a place to insert
	//			pCollection[i] = std::move(newElement);
	//			return;
	//		}
	//	}
	//	// Couldn't find a spot with an unused attachment
	//	pCollection.push_back(std::move(newElement));
	//}

	//// Helper function:
	//template <typename V>
	//std::optional<vk::AttachmentReference> insert_at_location_and_possibly_get_already_existing_element(V& pCollection, uint32_t pTargetLocation, uint32_t pAttachment, vk::ImageLayout pImageLayout)
	//{
	//	auto newElement = vk::AttachmentReference{}
	//		.setAttachment(pAttachment)
	//		.setLayout(pImageLayout);

	//	size_t l = static_cast<size_t>(pTargetLocation);
	//	// Fill up with unused attachments if required:
	//	while (pCollection.size() <= l) {
	//		pCollection.push_back(vk::AttachmentReference{}
	//			.setAttachment(VK_ATTACHMENT_UNUSED)
	//		);
	//	}
	//	// Set at the requested location:
	//	auto existingElement = pCollection[l];
	//	pCollection[l] = newElement;
	//	if (existingElement.attachment == VK_ATTACHMENT_UNUSED) {
	//		return {};
	//	}
	//	else {
	//		return existingElement;
	//	}
	//}
}
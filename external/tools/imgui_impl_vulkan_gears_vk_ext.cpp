
//
// Gears-Vk Addition: Creation and caching of DescriptorSets in ImTextureID for GUI image support
//

#include <unordered_map>

// Key for caching descriptor sets to retrieve descriptors by sampler, image_view and image_layout
struct DescriptorKey
{
	VkSampler sampler;
	VkImageView image_view;
	VkImageLayout image_layout;

	bool operator==(const DescriptorKey& other) const
	{
		return (sampler == other.sampler
			&& image_view == other.image_view
			&& image_layout == other.image_layout);
	}
};

// Call repeatedly to incrementally create a has value from multiple values.
// Taken from boost: https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Custom specialization of std::hash for DescriptorKey can be injected in namespace std
// adopted from https://en.cppreference.com/w/cpp/utility/hash to use hash_combine.
namespace std {
	template <>
	struct hash<DescriptorKey>
	{
		inline std::size_t operator()(const DescriptorKey& k) const
		{
			size_t seed = 0;
			hash_combine(seed, k.sampler);
			hash_combine(seed, k.image_view);
			hash_combine(seed, k.image_layout);
			return seed;
		}
	};
}

// Cache for descriptor sets as ImTextureID, accessible through a DescriptorKey
static std::unordered_map<DescriptorKey, ImTextureID> g_TextureDescriptorSets;

ImTextureID ImGui_ImplVulkan_Create_Or_GetTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout)
{
	DescriptorKey key = { sampler, image_view, image_layout };
	if (!g_TextureDescriptorSets.contains(key))
	{
		// Create DescriptorSet
		ImGui_ImplVulkan_InitInfo* v = &g_VulkanInitInfo;
		VkResult err;
		VkDescriptorSet descriptorSet;

		// Allocate the descriptor set
		{
			VkDescriptorSetAllocateInfo alloc_info = {};
			alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			alloc_info.descriptorPool = v->DescriptorPool;
			alloc_info.descriptorSetCount = 1;
			alloc_info.pSetLayouts = &g_DescriptorSetLayout;
			err = vkAllocateDescriptorSets(v->Device, &alloc_info, &descriptorSet);
			check_vk_result(err);
		}

		// Map the descriptor set to the image + sampler
		{
			VkDescriptorImageInfo desc_image[1] = {};
			desc_image[0].sampler = sampler;
			desc_image[0].imageView = image_view;
			desc_image[0].imageLayout = image_layout;
			VkWriteDescriptorSet write_desc[1] = {};
			write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write_desc[0].dstSet = descriptorSet;
			write_desc[0].descriptorCount = 1;
			write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_desc[0].pImageInfo = desc_image;
			vkUpdateDescriptorSets(v->Device, 1, write_desc, 0, NULL);
		}

		// Cache DescriptorSet
		g_TextureDescriptorSets[key] = (ImTextureID)descriptorSet;
	}
	// Return cached DescriptorSet
	return g_TextureDescriptorSets[key];
}

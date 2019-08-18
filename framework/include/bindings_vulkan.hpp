#pragma once
#include <type_traits>

namespace cgb
{
	template <typename T>
	vk::DescriptorType descriptor_type_of();

	template<>
	inline vk::DescriptorType descriptor_type_of<uniform_buffer_t>() { return vk::DescriptorType::eUniformBuffer; }

	template<>
	inline vk::DescriptorType descriptor_type_of<uniform_texel_buffer_t>() { return vk::DescriptorType::eUniformTexelBuffer; }

	template<>
	inline vk::DescriptorType descriptor_type_of<storage_buffer_t>() { return vk::DescriptorType::eStorageBuffer; }

	template<>
	inline vk::DescriptorType descriptor_type_of<storage_texel_buffer_t>() { return vk::DescriptorType::eStorageTexelBuffer; }

	template<>
	inline vk::DescriptorType descriptor_type_of<image_view_t>() { return vk::DescriptorType::eStorageImage; }

	template<>
	inline vk::DescriptorType descriptor_type_of<image_sampler_t>() { return vk::DescriptorType::eCombinedImageSampler; }

	template<>
	inline vk::DescriptorType descriptor_type_of<acceleration_structure>() { return vk::DescriptorType::eAccelerationStructureNV; }



	template<typename T, typename E> 
	typename std::enable_if<has_size_member<T>::value, std::vector<E*>>::type gather_one_or_multiple_element_pointers(T& t) {
		std::vector<E*> results;
		for (size_t i = 0; i < t.size(); ++i) {
			results.push_back(&t[i]);
		}
		return results;
	}

	template<typename T> 
	typename std::enable_if<has_no_size_member<T>::value, T*>::type gather_one_or_multiple_element_pointers(T& t) {
		return &t;
	}


	template <typename T>
	binding_data binding(uint32_t pSet, uint32_t pBinding, T& pResource, shader_type pShaderStages = shader_type::all)
	{
		binding_data data{
			pSet,
			vk::DescriptorSetLayoutBinding{}
				.setBinding(pBinding)
				.setDescriptorCount(num_elements(pResource))
				.setDescriptorType(descriptor_type_of<decltype(first_or_only_element(pResource))>())
				.setPImmutableSamplers(nullptr), // The pImmutableSamplers field is only relevant for image sampling related descriptors [3]
			pShaderStages,
			gather_one_or_multiple_element_pointers(pResource)
		};
		return data;
	}

	template <typename T>
	binding_data binding(uint32_t pSet, uint32_t pBinding, shader_type pShaderStages = shader_type::all)
	{
		binding_data data{
			pSet,
			vk::DescriptorSetLayoutBinding{}
				.setBinding(pBinding)
				.setDescriptorCount(1u)
				.setDescriptorType(descriptor_type_of<T>())
				.setPImmutableSamplers(nullptr), // The pImmutableSamplers field is only relevant for image sampling related descriptors [3]
			pShaderStages
		};
		return data;
	}


	template <typename T>
	binding_data binding(uint32_t pBinding, T& pResource, shader_type pShaderStages = shader_type::all)
	{
		return binding(0u, pBinding, pResource, pShaderStages);
	}

	template <typename T>
	binding_data binding(uint32_t pBinding, shader_type pShaderStages = shader_type::all)
	{
		return binding<T>(0u, pBinding, pShaderStages);
	}

	//template <typename T, typename TU, typename TS>
	//binding_data binding(uint32_t pSet, uint32_t pBinding, const std::variant<T, TU, TS>& pResource, shader_type pShaderStages = shader_type::all)
	//{
	//	return binding(pSet, pBinding, cgb::get(pResource), pShaderStages);
	//}

	//template <typename T, typename TU, typename TS>
	//binding_data binding(uint32_t pBinding, const std::variant<T, TU, TS>& pResource, shader_type pShaderStages = shader_type::all)
	//{
	//	return binding(0u, pBinding, cgb::get(pResource), pShaderStages);
	//}
}

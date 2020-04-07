#pragma once
#include <type_traits>

namespace cgb
{
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
				.setDescriptorType(descriptor_type_of<decltype(&first_or_only_element(pResource))>())
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
				.setDescriptorType(descriptor_type_of<T>(nullptr))
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
	binding_data binding(uint32_t aBinding, shader_type aShaderStages = shader_type::all)
	{
		return binding<T>(0u, aBinding, aShaderStages);
	}

}

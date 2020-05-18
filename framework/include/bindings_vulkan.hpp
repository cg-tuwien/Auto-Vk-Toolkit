#pragma once
#include <type_traits>

namespace cgb
{
	template <typename T>
	vk::DescriptorType descriptor_type_of(const T*);

	template<>
	inline vk::DescriptorType descriptor_type_of<uniform_buffer_t>(const uniform_buffer_t*) { return vk::DescriptorType::eUniformBuffer; }
	template<>
	inline vk::DescriptorType descriptor_type_of<uniform_buffer>(const uniform_buffer*) { return vk::DescriptorType::eUniformBuffer; }

	template<>
	inline vk::DescriptorType descriptor_type_of<uniform_texel_buffer_t>(const uniform_texel_buffer_t*) { return vk::DescriptorType::eUniformTexelBuffer; }
	template<>
	inline vk::DescriptorType descriptor_type_of<uniform_texel_buffer>(const uniform_texel_buffer*) { return vk::DescriptorType::eUniformTexelBuffer; }

	template<>
	inline vk::DescriptorType descriptor_type_of<storage_buffer_t>(const storage_buffer_t*) { return vk::DescriptorType::eStorageBuffer; }
	template<>
	inline vk::DescriptorType descriptor_type_of<storage_buffer>(const storage_buffer*) { return vk::DescriptorType::eStorageBuffer; }

	template<>
	inline vk::DescriptorType descriptor_type_of<storage_texel_buffer_t>(const storage_texel_buffer_t*) { return vk::DescriptorType::eStorageTexelBuffer; }
	template<>
	inline vk::DescriptorType descriptor_type_of<storage_texel_buffer>(const storage_texel_buffer*) { return vk::DescriptorType::eStorageTexelBuffer; }

	template<>
	inline vk::DescriptorType descriptor_type_of<image_view_t>(const image_view_t* aImageView) { return vk::DescriptorType::eSampledImage; }

	template<>
	inline vk::DescriptorType descriptor_type_of<image_view>(const image_view* aImageView) { return vk::DescriptorType::eSampledImage; }

	template<>
	inline vk::DescriptorType descriptor_type_of<image_sampler_t>(const image_sampler_t*) { return vk::DescriptorType::eCombinedImageSampler; }
	template<>
	inline vk::DescriptorType descriptor_type_of<image_sampler>(const image_sampler*) { return vk::DescriptorType::eCombinedImageSampler; }

	template<>
	inline vk::DescriptorType descriptor_type_of<top_level_acceleration_structure_t>(const top_level_acceleration_structure_t*) { return vk::DescriptorType::eAccelerationStructureNV; }
	template<>
	inline vk::DescriptorType descriptor_type_of<top_level_acceleration_structure>(const top_level_acceleration_structure*) { return vk::DescriptorType::eAccelerationStructureNV; }

	template<>
	inline vk::DescriptorType descriptor_type_of<buffer_view_t>(const buffer_view_t* _BufferView) { return _BufferView->descriptor_type(); }
	template<>
	inline vk::DescriptorType descriptor_type_of<buffer_view>(const buffer_view* _BufferView) { return (*_BufferView)->descriptor_type(); }

	template<>
	inline vk::DescriptorType descriptor_type_of<sampler_t>(const sampler_t*) { return vk::DescriptorType::eSampler; }
	template<>
	inline vk::DescriptorType descriptor_type_of<sampler>(const sampler*) { return vk::DescriptorType::eSampler; }

	template<>
	inline vk::DescriptorType descriptor_type_of<image_view_as_input_attachment>(const image_view_as_input_attachment*) { return vk::DescriptorType::eInputAttachment; }

	template<>
	inline vk::DescriptorType descriptor_type_of<image_view_as_storage_image>(const image_view_as_storage_image*) { return vk::DescriptorType::eStorageImage; }


	template<typename T> 
	typename std::enable_if<has_size_and_iterators<T>::value, std::vector<typename T::value_type::value_type*>>::type gather_one_or_multiple_element_pointers(T& t) {
		std::vector<typename T::value_type::value_type*> results;
		for (size_t i = 0; i < t.size(); ++i) {
			results.push_back(&(*t[i]));
		}
		return results;
	}

	template<typename T> 
	typename std::enable_if<!has_size_and_iterators<T>::value && is_dereferenceable<T>::value, typename T::value_type*>::type gather_one_or_multiple_element_pointers(T& t) {
		return &*t;
	}

	template<typename T> 
	typename std::enable_if<!has_size_and_iterators<T>::value && !is_dereferenceable<T>::value, T*>::type gather_one_or_multiple_element_pointers(T& t) {
		return &t;
	}

	template <typename T>
	binding_data binding(uint32_t aSet, uint32_t aBinding, const T& aResource, shader_type aShaderStages = shader_type::all)
	{
		binding_data data{
			aSet,
			vk::DescriptorSetLayoutBinding{}
				.setBinding(aBinding)
				.setDescriptorCount(how_many_elements(aResource))
				.setDescriptorType(descriptor_type_of(&first_or_only_element(aResource)))
				.setStageFlags(to_vk_shader_stages(aShaderStages))
				.setPImmutableSamplers(nullptr), // The pImmutableSamplers field is only relevant for image sampling related descriptors [3]
			gather_one_or_multiple_element_pointers(const_cast<T&>(aResource))
		};
		return data;
	}

	template <typename T>
	binding_data binding(uint32_t aSet, uint32_t aBinding, uint32_t aCount, shader_type aShaderStages = shader_type::all)
	{
		assert(aCount > 0u);
		binding_data data{
			aSet,
			vk::DescriptorSetLayoutBinding{}
				.setBinding(aBinding)
				.setDescriptorCount(1u)
				.setDescriptorType(descriptor_type_of<T>(nullptr))
				.setStageFlags(to_vk_shader_stages(aShaderStages))
				.setPImmutableSamplers(nullptr), // The pImmutableSamplers field is only relevant for image sampling related descriptors [3]
		};
		return data;
	}


	template <typename T>
	binding_data binding(uint32_t aBinding, const T& aResource, shader_type aShaderStages = shader_type::all)
	{
		return binding(0u, aBinding, aResource, aShaderStages);
	}

	//template <typename T>
	//binding_data binding(uint32_t aBinding, uint32_t aCount, shader_type aShaderStages = shader_type::all)
	//{
	//	assert(aCount > 0u);
	//	return binding<T>(0u, aBinding, aCount, aShaderStages);
	//}

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

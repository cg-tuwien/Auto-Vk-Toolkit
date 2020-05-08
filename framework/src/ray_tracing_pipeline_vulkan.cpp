#include <cg_base.hpp>

namespace cgb
{
	using namespace cgb::cfg;

	max_recursion_depth max_recursion_depth::set_to_max()
	{
		vk::PhysicalDeviceRayTracingPropertiesKHR rtProps;
		vk::PhysicalDeviceProperties2 props2;
		props2.pNext = &rtProps;
		context().physical_device().getProperties2(&props2);
		return max_recursion_depth { rtProps.maxRecursionDepth };
	}

	owning_resource<ray_tracing_pipeline_t> ray_tracing_pipeline_t::create(ray_tracing_pipeline_config _Config, cgb::context_specific_function<void(ray_tracing_pipeline_t&)> _AlterConfigBeforeCreation)
	{
		ray_tracing_pipeline_t result;

		// 1. Set pipeline flags 
		result.mPipelineCreateFlags = {};
		// TODO: Support all flags (only one of the flags is handled at the moment)
		if ((_Config.mPipelineSettings & pipeline_settings::disable_optimization) == pipeline_settings::disable_optimization) {
			result.mPipelineCreateFlags |= vk::PipelineCreateFlagBits::eDisableOptimization;
		}

		// 2. Gather and build shaders
		// First of all, gather unique shaders and build them
		std::vector<shader_info> orderedUniqueShaderInfos;
		for (auto& tableEntry : _Config.mShaderTableConfig.mShaderTableEntries) {
			if (std::holds_alternative<shader_info>(tableEntry)) {
				add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, std::get<shader_info>(tableEntry));
			}
			else if (std::holds_alternative<triangles_hit_group>(tableEntry)) {
				const auto& hitGroup = std::get<triangles_hit_group>(tableEntry);
				if (hitGroup.mAnyHitShader.has_value()) {
					add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mAnyHitShader.value());
				}
				if (hitGroup.mClosestHitShader.has_value()) {
					add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mClosestHitShader.value());
				}
			}
			else if (std::holds_alternative<procedural_hit_group>(tableEntry)) {
				const auto& hitGroup = std::get<procedural_hit_group>(tableEntry);
				add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mIntersectionShader);
				if (hitGroup.mAnyHitShader.has_value()) {
					add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mAnyHitShader.value());
				}
				if (hitGroup.mClosestHitShader.has_value()) {
					add_to_vector_if_not_already_contained(orderedUniqueShaderInfos, hitGroup.mClosestHitShader.value());
				}
			}
			else {
				throw cgb::runtime_error("tableEntry holds an unknown alternative. That's mysterious.");
			}
		}
		result.mShaders.reserve(orderedUniqueShaderInfos.size());
		result.mShaderStageCreateInfos.reserve(orderedUniqueShaderInfos.size());
		for (auto& shaderInfo : orderedUniqueShaderInfos) {
			// 2.2 Compile the shader
			result.mShaders.push_back(shader::create(shaderInfo));
			assert(result.mShaders.back().has_been_built());
			// 2.3 Create shader info
			result.mShaderStageCreateInfos.push_back(vk::PipelineShaderStageCreateInfo{}
				.setStage(to_vk_shader_stage(result.mShaders.back().info().mShaderType))
				.setModule(result.mShaders.back().handle())
				.setPName(result.mShaders.back().info().mEntryPoint.c_str())
			);
		}
		assert(orderedUniqueShaderInfos.size() == result.mShaders.size());
		assert(result.mShaders.size() == result.mShaderStageCreateInfos.size());
#if defined(_DEBUG)
		// Perform a sanity check:
		for (size_t i = 0; i < orderedUniqueShaderInfos.size(); ++i) {
			assert(orderedUniqueShaderInfos[i] == result.mShaders[i].info());
		}
#endif

		// 3. Create the shader table (with references to the shaders from step 2.)
		// Iterate over the shader table... AGAIN!
		result.mShaderGroupCreateInfos.reserve(_Config.mShaderTableConfig.mShaderTableEntries.size());
		for (auto& tableEntry : _Config.mShaderTableConfig.mShaderTableEntries) {
			// ...But this time, build the shader groups for Vulkan's Ray Tracing Pipeline:
			if (std::holds_alternative<shader_info>(tableEntry)) {
				// The shader indices are actually indices into `result.mShaders` not into
				// `orderedUniqueShaderInfos`. However, both vectors are aligned perfectly,
				// so we are just using `orderedUniqueShaderInfos` for convenience.
				const uint32_t generalShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, std::get<shader_info>(tableEntry)));
				result.mShaderGroupCreateInfos.emplace_back()
					.setType(vk::RayTracingShaderGroupTypeNV::eGeneral)
					.setGeneralShader(generalShaderIndex)
					.setIntersectionShader(VK_SHADER_UNUSED_NV)
					.setAnyHitShader(VK_SHADER_UNUSED_NV)
					.setClosestHitShader(VK_SHADER_UNUSED_NV);
			}
			else if (std::holds_alternative<triangles_hit_group>(tableEntry)) {
				const auto& hitGroup = std::get<triangles_hit_group>(tableEntry);
				uint32_t rahitShaderIndex = VK_SHADER_UNUSED_NV;
				if (hitGroup.mAnyHitShader.has_value()) {
					rahitShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mAnyHitShader.value()));
				}
				uint32_t rchitShaderIndex = VK_SHADER_UNUSED_NV;
				if (hitGroup.mClosestHitShader.has_value()) {
					rchitShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mClosestHitShader.value()));
				}
				result.mShaderGroupCreateInfos.emplace_back()
					.setType(vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup)
					.setGeneralShader(VK_SHADER_UNUSED_NV)
					.setIntersectionShader(VK_SHADER_UNUSED_NV)
					.setAnyHitShader(rahitShaderIndex)
					.setClosestHitShader(rchitShaderIndex);
			}
			else if (std::holds_alternative<procedural_hit_group>(tableEntry)) {
				const auto& hitGroup = std::get<procedural_hit_group>(tableEntry);
				uint32_t rintShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mIntersectionShader));
				uint32_t rahitShaderIndex = VK_SHADER_UNUSED_NV;
				if (hitGroup.mAnyHitShader.has_value()) {
					rahitShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mAnyHitShader.value()));
				}
				uint32_t rchitShaderIndex = VK_SHADER_UNUSED_NV;
				if (hitGroup.mClosestHitShader.has_value()) {
					rchitShaderIndex = static_cast<uint32_t>(index_of(orderedUniqueShaderInfos, hitGroup.mClosestHitShader.value()));
				}
				result.mShaderGroupCreateInfos.emplace_back()
					.setType(vk::RayTracingShaderGroupTypeNV::eProceduralHitGroup)
					.setGeneralShader(VK_SHADER_UNUSED_NV)
					.setIntersectionShader(rintShaderIndex)
					.setAnyHitShader(rahitShaderIndex)
					.setClosestHitShader(rchitShaderIndex);
			}
			else {
				throw cgb::runtime_error("tableEntry holds an unknown alternative. That's mysterious.");
			}
		}

		// 4. Maximum recursion depth:
		result.mMaxRecursionDepth = _Config.mMaxRecursionDepth.mMaxRecursionDepth;

		// 5. Pipeline layout
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(_Config.mResourceBindings));
		result.mAllDescriptorSetLayouts.allocate_all();
		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		// Gather the push constant data
		result.mPushConstantRanges.reserve(_Config.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : _Config.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stages(pcBinding.mShaderStages))
				.setOffset(static_cast<uint32_t>(pcBinding.mOffset))
				.setSize(static_cast<uint32_t>(pcBinding.mSize))
			);
			// TODO: Push Constants need a prettier interface
		}
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 6. Maybe alter the config?
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		// 8. Create the pipeline's layout
		result.mPipelineLayout = context().logical_device().createPipelineLayoutUnique(result.mPipelineLayoutCreateInfo);
		assert(nullptr != result.layout_handle());

		// 9. Build the Ray Tracing Pipeline
		auto pipelineCreateInfo = vk::RayTracingPipelineCreateInfoNV{}
			.setStageCount(static_cast<uint32_t>(result.mShaderStageCreateInfos.size()))
			.setPStages(result.mShaderStageCreateInfos.data())
			.setGroupCount(static_cast<uint32_t>(result.mShaderGroupCreateInfos.size()))
			.setPGroups(result.mShaderGroupCreateInfos.data())
			.setMaxRecursionDepth(result.mMaxRecursionDepth)
			.setLayout(result.layout_handle());
		result.mPipeline = context().logical_device().createRayTracingPipelineNVUnique(
			nullptr,
			pipelineCreateInfo,
			nullptr,
			context().dynamic_dispatch());


		// 10. Build the shader binding table
		{
			vk::PhysicalDeviceRayTracingPropertiesNV rtProps;
			vk::PhysicalDeviceProperties2 props2;
			props2.pNext = &rtProps;
			context().physical_device().getProperties2(&props2);

			result.mShaderGroupHandleSize = static_cast<size_t>(rtProps.shaderGroupHandleSize);
			size_t shaderBindingTableSize = result.mShaderGroupHandleSize * result.mShaderGroupCreateInfos.size();

			// TODO: All of this SBT-stuf probably needs some refactoring
			result.mShaderBindingTable = cgb::create(
				generic_buffer_meta::create_from_size(shaderBindingTableSize),
				memory_usage::host_coherent,
				vk::BufferUsageFlagBits::eRayTracingNV
			); 
			void* mapped = context().logical_device().mapMemory(result.mShaderBindingTable->memory_handle(), 0, result.mShaderBindingTable->meta_data().total_size());
			// Transfer something into the buffer's memory...
			context().logical_device().getRayTracingShaderGroupHandlesNV(
				result.handle(), 
				0, static_cast<uint32_t>(result.mShaderGroupCreateInfos.size()), 
				result.mShaderBindingTable->meta_data().total_size(), 
				mapped, 
				context().dynamic_dispatch());
			context().logical_device().unmapMemory(result.mShaderBindingTable->memory_handle());
		}


		result.mTracker.setTrackee(result);
		return result;
	}
}

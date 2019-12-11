#pragma once

namespace cgb
{
	extern vk::IndexType to_vk_index_type(size_t pSize);

	extern vk::ImageViewType to_image_view_type(const vk::ImageCreateInfo& info);

	extern vk::Bool32 to_vk_bool(bool value);

	/** Converts a cgb::shader_type to the vulkan-specific vk::ShaderStageFlagBits type */
	extern vk::ShaderStageFlagBits to_vk_shader_stage(shader_type pType);

	extern vk::ShaderStageFlags to_vk_shader_stages(shader_type pType);

	extern vk::SampleCountFlagBits to_vk_sample_count(int pSampleCount);

	extern int to_cgb_sample_count(vk::SampleCountFlagBits pSampleCount);

	extern vk::VertexInputRate to_vk_vertex_input_rate(input_binding_general_data::kind _Value);
	
	extern vk::PrimitiveTopology to_vk_primitive_topology(cfg::primitive_topology _Value);

	extern vk::PolygonMode to_vk_polygon_mode(cfg::polygon_drawing_mode _Value);

	extern vk::CullModeFlags to_vk_cull_mode(cfg::culling_mode _Value);

	extern vk::FrontFace to_vk_front_face(cfg::winding_order _Value);

	extern vk::CompareOp to_vk_compare_op(cfg::compare_operation _Value);

	extern vk::ColorComponentFlags to_vk_color_components(cfg::color_channel _Value);

	extern vk::BlendFactor to_vk_blend_factor(cfg::blending_factor _Value);

	extern vk::BlendOp to_vk_blend_operation(cfg::color_blending_operation _Value);

	extern vk::LogicOp to_vk_logic_operation(cfg::blending_logic_operation _Value);

	extern vk::AttachmentLoadOp to_vk_load_op(cfg::attachment_load_operation _Value);

	extern vk::AttachmentStoreOp to_vk_store_op(cfg::attachment_store_operation _Value);
}

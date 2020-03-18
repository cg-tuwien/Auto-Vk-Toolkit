#include <cg_base.hpp>

namespace cgb
{
	using namespace cgb::cfg;

	vk::IndexType to_vk_index_type(size_t aSize)
	{
		if (aSize == sizeof(uint16_t)) {
			return vk::IndexType::eUint16;
		}
		if (aSize == sizeof(uint32_t)) {
			return vk::IndexType::eUint32;
		}
		LOG_ERROR(fmt::format("The given size[{}] does not correspond to a valid vk::IndexType", aSize));
		return vk::IndexType::eNoneNV;
	}

	vk::ImageViewType to_image_view_type(const vk::ImageCreateInfo& info)
	{
		switch (info.imageType)
		{
		case vk::ImageType::e1D:
			if (info.arrayLayers > 1) {
				return vk::ImageViewType::e1DArray;
			}
			else {
				return vk::ImageViewType::e1D;
			}
		case vk::ImageType::e2D:
			if (info.arrayLayers > 1) {
				return vk::ImageViewType::e2DArray;
			}
			else {
				return vk::ImageViewType::e2D;
			}
		case vk::ImageType::e3D:
			return vk::ImageViewType::e3D;
		}
		throw new std::runtime_error("It might be that the implementation of to_image_view_type(const vk::ImageCreateInfo& info) is incomplete. Please complete it!");
	}

	vk::Bool32 to_vk_bool(bool value)
	{
		return value ? VK_TRUE : VK_FALSE;
	}

	vk::ShaderStageFlagBits to_vk_shader_stage(shader_type aType)
	{
		switch (aType) {
		case cgb::shader_type::vertex:
			return vk::ShaderStageFlagBits::eVertex;
		case cgb::shader_type::tessellation_control:
			return vk::ShaderStageFlagBits::eTessellationControl;
		case cgb::shader_type::tessellation_evaluation:
			return vk::ShaderStageFlagBits::eTessellationEvaluation;
		case cgb::shader_type::geometry:
			return vk::ShaderStageFlagBits::eGeometry;
		case cgb::shader_type::fragment:
			return vk::ShaderStageFlagBits::eFragment;
		case cgb::shader_type::compute:
			return vk::ShaderStageFlagBits::eCompute;
		case cgb::shader_type::ray_generation:
			return vk::ShaderStageFlagBits::eRaygenNV;
		case cgb::shader_type::any_hit:
			return vk::ShaderStageFlagBits::eAnyHitNV;
		case cgb::shader_type::closest_hit:
			return vk::ShaderStageFlagBits::eClosestHitNV;
		case cgb::shader_type::miss:
			return vk::ShaderStageFlagBits::eMissNV;
		case cgb::shader_type::intersection:
			return vk::ShaderStageFlagBits::eIntersectionNV;
		case cgb::shader_type::callable:
			return vk::ShaderStageFlagBits::eCallableNV;
		case cgb::shader_type::task:
			return vk::ShaderStageFlagBits::eTaskNV;
		case cgb::shader_type::mesh:
			return vk::ShaderStageFlagBits::eMeshNV;
		default:
			throw std::runtime_error("Invalid shader_type");
		}
	}

	vk::ShaderStageFlags to_vk_shader_stages(shader_type aType)
	{
		vk::ShaderStageFlags result;
		if ((aType & cgb::shader_type::vertex) == cgb::shader_type::vertex) {
			result |= vk::ShaderStageFlagBits::eVertex;
		}
		if ((aType & cgb::shader_type::tessellation_control) == cgb::shader_type::tessellation_control) {
			result |= vk::ShaderStageFlagBits::eTessellationControl;
		}
		if ((aType & cgb::shader_type::tessellation_evaluation) == cgb::shader_type::tessellation_evaluation) {
			result |= vk::ShaderStageFlagBits::eTessellationEvaluation;
		}
		if ((aType & cgb::shader_type::geometry) == cgb::shader_type::geometry) {
			result |= vk::ShaderStageFlagBits::eGeometry;
		}
		if ((aType & cgb::shader_type::fragment) == cgb::shader_type::fragment) {
			result |= vk::ShaderStageFlagBits::eFragment;
		}
		if ((aType & cgb::shader_type::compute) == cgb::shader_type::compute) {
			result |= vk::ShaderStageFlagBits::eCompute;
		}
		if ((aType & cgb::shader_type::ray_generation) == cgb::shader_type::ray_generation) {
			result |= vk::ShaderStageFlagBits::eRaygenNV;
		}
		if ((aType & cgb::shader_type::any_hit) == cgb::shader_type::any_hit) {
			result |= vk::ShaderStageFlagBits::eAnyHitNV;
		}
		if ((aType & cgb::shader_type::closest_hit) == cgb::shader_type::closest_hit) {
			result |= vk::ShaderStageFlagBits::eClosestHitNV;
		}
		if ((aType & cgb::shader_type::miss) == cgb::shader_type::miss) {
			result |= vk::ShaderStageFlagBits::eMissNV;
		}
		if ((aType & cgb::shader_type::intersection) == cgb::shader_type::intersection) {
			result |= vk::ShaderStageFlagBits::eIntersectionNV;
		}
		if ((aType & cgb::shader_type::callable) == cgb::shader_type::callable) {
			result |= vk::ShaderStageFlagBits::eCallableNV;
		}
		if ((aType & cgb::shader_type::task) == cgb::shader_type::task) {
			result |= vk::ShaderStageFlagBits::eTaskNV;
		}
		if ((aType & cgb::shader_type::mesh) == cgb::shader_type::mesh) {
			result |= vk::ShaderStageFlagBits::eMeshNV;
		}
		return result;
	}

	vk::SampleCountFlagBits to_vk_sample_count(int aSampleCount)
	{
		switch (aSampleCount) {
		case 1:
			return vk::SampleCountFlagBits::e1;
		case 2:
			return vk::SampleCountFlagBits::e2;
		case 4:
			return vk::SampleCountFlagBits::e4;
		case 8:
			return vk::SampleCountFlagBits::e8;
		case 16:
			return vk::SampleCountFlagBits::e16;
		case 32:
			return vk::SampleCountFlagBits::e32;
		case 64:
			return vk::SampleCountFlagBits::e64;
		default:
			throw std::invalid_argument("Invalid number of samples");
		}
	}

	int to_cgb_sample_count(vk::SampleCountFlagBits aSampleCount)
	{
		switch (aSampleCount)
		{
		case vk::SampleCountFlagBits::e1: return 1;
		case vk::SampleCountFlagBits::e2: return 2;
		case vk::SampleCountFlagBits::e4: return 4;
		case vk::SampleCountFlagBits::e8: return 8;
		case vk::SampleCountFlagBits::e16: return 16;
		case vk::SampleCountFlagBits::e32: return 32;
		case vk::SampleCountFlagBits::e64: return 64;
		default:
			throw std::invalid_argument("Invalid number of samples");
		}
	}

	vk::VertexInputRate to_vk_vertex_input_rate(input_binding_general_data::kind aValue)
	{
		switch (aValue) {
		case input_binding_general_data::kind::instance:
			return vk::VertexInputRate::eInstance;
		case input_binding_general_data::kind::vertex:
			return vk::VertexInputRate::eVertex;
		default:
			throw std::invalid_argument("Invalid vertex input rate");
		}
	}

	vk::PrimitiveTopology to_vk_primitive_topology(primitive_topology aValue)
	{
		switch (aValue) {
		case primitive_topology::points:
			return vk::PrimitiveTopology::ePointList;
		case primitive_topology::lines: 
			return vk::PrimitiveTopology::eLineList;
		case primitive_topology::line_strip:
			return vk::PrimitiveTopology::eLineStrip;
		case primitive_topology::triangles: 
			return vk::PrimitiveTopology::eTriangleList;
		case primitive_topology::triangle_strip:
			return vk::PrimitiveTopology::eTriangleStrip;
		case primitive_topology::triangle_fan: 
			return vk::PrimitiveTopology::eTriangleFan;
		case primitive_topology::lines_with_adjacency:
			return vk::PrimitiveTopology::eLineListWithAdjacency;
		case primitive_topology::line_strip_with_adjacency: 
			return vk::PrimitiveTopology::eLineStripWithAdjacency;
		case primitive_topology::triangles_with_adjacency: 
			return vk::PrimitiveTopology::eTriangleListWithAdjacency;
		case primitive_topology::triangle_strip_with_adjacency: 
			return vk::PrimitiveTopology::eTriangleStripWithAdjacency;
		case primitive_topology::patches: 
			return vk::PrimitiveTopology::ePatchList;
		default:
			throw std::invalid_argument("Invalid primitive topology");
		}
	}

	vk::PolygonMode to_vk_polygon_mode(polygon_drawing_mode aValue)
	{
		switch (aValue) {
		case polygon_drawing_mode::fill: 
			return vk::PolygonMode::eFill;
		case polygon_drawing_mode::line:
			return vk::PolygonMode::eLine;
		case polygon_drawing_mode::point:
			return vk::PolygonMode::ePoint;
		default:
			throw std::invalid_argument("Invalid polygon drawing mode.");
		}
	}

	vk::CullModeFlags to_vk_cull_mode(culling_mode aValue)
	{
		switch (aValue) {
		case culling_mode::disabled:
			return vk::CullModeFlagBits::eNone;
		case culling_mode::cull_front_faces:
			return vk::CullModeFlagBits::eFront;
		case culling_mode::cull_back_faces:
			return vk::CullModeFlagBits::eBack;
		case culling_mode::cull_front_and_back_faces:
			return vk::CullModeFlagBits::eFrontAndBack;
		default:
			throw std::invalid_argument("Invalid culling mode.");
		}
	}

	vk::FrontFace to_vk_front_face(winding_order aValue)
	{
		switch (aValue) {
		case winding_order::counter_clockwise:
			return vk::FrontFace::eCounterClockwise;
		case winding_order::clockwise:
			return vk::FrontFace::eClockwise;
		default:
			throw std::invalid_argument("Invalid front face winding order.");
		}
	}

	vk::CompareOp to_vk_compare_op(compare_operation aValue)
	{
		switch(aValue) {
		case compare_operation::never:
			return vk::CompareOp::eNever;
		case compare_operation::less: 
			return vk::CompareOp::eLess;
		case compare_operation::equal: 
			return vk::CompareOp::eEqual;
		case compare_operation::less_or_equal: 
			return vk::CompareOp::eLessOrEqual;
		case compare_operation::greater: 
			return vk::CompareOp::eGreater;
		case compare_operation::not_equal: 
			return vk::CompareOp::eNotEqual;
		case compare_operation::greater_or_equal: 
			return vk::CompareOp::eGreaterOrEqual;
		case compare_operation::always: 
			return vk::CompareOp::eAlways;
		default:
			throw std::invalid_argument("Invalid compare operation.");
		}
	}

	vk::ColorComponentFlags to_vk_color_components(color_channel aValue)
	{
		switch (aValue)	{
		case cgb::color_channel::none:
			return vk::ColorComponentFlags{};
		case cgb::color_channel::red:
			return vk::ColorComponentFlagBits::eR;
		case cgb::color_channel::green:
			return vk::ColorComponentFlagBits::eG;
		case cgb::color_channel::blue:
			return vk::ColorComponentFlagBits::eB;
		case cgb::color_channel::alpha:
			return vk::ColorComponentFlagBits::eA;
		case cgb::color_channel::rg:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG;
		case cgb::color_channel::rgb:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB;
		case cgb::color_channel::rgba:
			return vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		default:
			throw std::invalid_argument("Invalid color channel value.");
		}
	}

	vk::BlendFactor to_vk_blend_factor(blending_factor aValue)
	{
		switch (aValue) {
		case blending_factor::zero:
			return vk::BlendFactor::eZero;
		case blending_factor::one: 
			return vk::BlendFactor::eOne;
		case blending_factor::source_color: 
			return vk::BlendFactor::eSrcColor;
		case blending_factor::one_minus_source_color: 
			return vk::BlendFactor::eOneMinusSrcColor;
		case blending_factor::destination_color: 
			return vk::BlendFactor::eDstColor;
		case blending_factor::one_minus_destination_color: 
			return vk::BlendFactor::eOneMinusDstColor;
		case blending_factor::source_alpha: 
			return vk::BlendFactor::eSrcAlpha;
		case blending_factor::one_minus_source_alpha: 
			return vk::BlendFactor::eOneMinusSrcAlpha;
		case blending_factor::destination_alpha: 
			return vk::BlendFactor::eDstAlpha;
		case blending_factor::one_minus_destination_alpha:
			return vk::BlendFactor::eOneMinusDstAlpha;
		case blending_factor::constant_color: 
			return vk::BlendFactor::eConstantColor;
		case blending_factor::one_minus_constant_color: 
			return vk::BlendFactor::eOneMinusConstantColor;
		case blending_factor::constant_alpha: 
			return vk::BlendFactor::eConstantAlpha;
		case blending_factor::one_minus_constant_alpha: 
			return vk::BlendFactor::eOneMinusConstantAlpha;
		case blending_factor::source_alpha_saturate: 
			return vk::BlendFactor::eSrcAlphaSaturate;
		default:
			throw std::invalid_argument("Invalid blend factor value.");
		}
	}

	vk::BlendOp to_vk_blend_operation(color_blending_operation aValue)
	{
		switch (aValue)
		{
		case color_blending_operation::add: 
			return vk::BlendOp::eAdd;
		case color_blending_operation::subtract: 
			return vk::BlendOp::eSubtract;
		case color_blending_operation::reverse_subtract: 
			return vk::BlendOp::eReverseSubtract;
		case color_blending_operation::min: 
			return vk::BlendOp::eMin;
		case color_blending_operation::max: 
			return vk::BlendOp::eMax;
		default:
			throw std::invalid_argument("Invalid color blending operation.");
		}
	}

	vk::LogicOp to_vk_logic_operation(blending_logic_operation aValue)
	{
		switch (aValue)
		{
		case blending_logic_operation::op_clear:
			return vk::LogicOp::eClear;
		case blending_logic_operation::op_and: 
			return vk::LogicOp::eAnd;
		case blending_logic_operation::op_and_reverse: 
			return vk::LogicOp::eAndReverse;
		case blending_logic_operation::op_copy: 
			return vk::LogicOp::eCopy;
		case blending_logic_operation::op_and_inverted: 
			return vk::LogicOp::eAndInverted;
		case blending_logic_operation::no_op: 
			return vk::LogicOp::eNoOp;
		case blending_logic_operation::op_xor: 
			return vk::LogicOp::eXor;
		case blending_logic_operation::op_or: 
			return vk::LogicOp::eOr;
		case blending_logic_operation::op_nor: 
			return vk::LogicOp::eNor;
		case blending_logic_operation::op_equivalent: 
			return vk::LogicOp::eEquivalent;
		case blending_logic_operation::op_invert: 
			return vk::LogicOp::eInvert;
		case blending_logic_operation::op_or_reverse: 
			return vk::LogicOp::eOrReverse;
		case blending_logic_operation::op_copy_inverted: 
			return vk::LogicOp::eCopyInverted;
		case blending_logic_operation::op_or_inverted: 
			return vk::LogicOp::eOrInverted;
		case blending_logic_operation::op_nand: 
			return vk::LogicOp::eNand;
		case blending_logic_operation::op_set: 
			return vk::LogicOp::eSet;
		default: 
			throw std::invalid_argument("Invalid blending logic operation.");
		}
	}

	vk::AttachmentLoadOp to_vk_load_op(cfg::attachment_load_operation aValue)
	{
		switch (aValue) {
		case attachment_load_operation::dont_care:
			return vk::AttachmentLoadOp::eDontCare;
		case attachment_load_operation::clear: 
			return vk::AttachmentLoadOp::eClear;
		case attachment_load_operation::load: 
			return vk::AttachmentLoadOp::eLoad;
		default:
			throw std::invalid_argument("Invalid attachment load operation.");
		}
	}

	vk::AttachmentStoreOp to_vk_store_op(cfg::attachment_store_operation aValue)
	{
		switch (aValue) {
		case attachment_store_operation::dont_care:
			return vk::AttachmentStoreOp::eDontCare;
		case attachment_store_operation::store: 
			return vk::AttachmentStoreOp::eStore;
		default:
			throw std::invalid_argument("Invalid attachment store operation.");
		}
	}

	vk::PipelineStageFlags to_vk_pipeline_stage_flags(cgb::pipeline_stage aValue)
	{
		vk::PipelineStageFlags result;
		// TODO: This might be a bit expensive. Is there a different possible solution to this?
		if (cgb::is_included(aValue, cgb::pipeline_stage::top_of_pipe					)) { result |= vk::PipelineStageFlagBits::eTopOfPipe					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::draw_indirect					)) { result |= vk::PipelineStageFlagBits::eDrawIndirect					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::vertex_input					)) { result |= vk::PipelineStageFlagBits::eVertexInput					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::vertex_shader					)) { result |= vk::PipelineStageFlagBits::eVertexShader					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::tessellation_control_shader	)) { result |= vk::PipelineStageFlagBits::eTessellationControlShader	; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::tessellation_evaluation_shader)) { result |= vk::PipelineStageFlagBits::eTessellationEvaluationShader	; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::geometry_shader				)) { result |= vk::PipelineStageFlagBits::eGeometryShader				; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::fragment_shader				)) { result |= vk::PipelineStageFlagBits::eFragmentShader				; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::early_fragment_tests			)) { result |= vk::PipelineStageFlagBits::eEarlyFragmentTests			; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::late_fragment_tests			)) { result |= vk::PipelineStageFlagBits::eLateFragmentTests			; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::color_attachment_output		)) { result |= vk::PipelineStageFlagBits::eColorAttachmentOutput		; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::compute_shader				)) { result |= vk::PipelineStageFlagBits::eComputeShader				; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::transfer						)) { result |= vk::PipelineStageFlagBits::eTransfer						; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::bottom_of_pipe				)) { result |= vk::PipelineStageFlagBits::eBottomOfPipe					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::host							)) { result |= vk::PipelineStageFlagBits::eHost							; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::all_graphics_stages			)) { result |= vk::PipelineStageFlagBits::eAllGraphics					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::all_commands					)) { result |= vk::PipelineStageFlagBits::eAllCommands					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::transform_feedback			)) { result |= vk::PipelineStageFlagBits::eTransformFeedbackEXT			; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::conditional_rendering			)) { result |= vk::PipelineStageFlagBits::eConditionalRenderingEXT		; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::command_processing			)) { result |= vk::PipelineStageFlagBits::eCommandProcessNVX			; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::shading_rate_image			)) { result |= vk::PipelineStageFlagBits::eShadingRateImageNV			; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::ray_tracing_shaders			)) { result |= vk::PipelineStageFlagBits::eRayTracingShaderNV			; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::acceleration_structure_build	)) { result |= vk::PipelineStageFlagBits::eAccelerationStructureBuildNV	; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::task_shader					)) { result |= vk::PipelineStageFlagBits::eTaskShaderNV					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::mesh_shader					)) { result |= vk::PipelineStageFlagBits::eMeshShaderNV					; }
		if (cgb::is_included(aValue, cgb::pipeline_stage::fragment_density_process		)) { result |= vk::PipelineStageFlagBits::eFragmentDensityProcessEXT	; }
		return result;
	}
	
	vk::PipelineStageFlags to_vk_pipeline_stage_flags(std::optional<cgb::pipeline_stage> aValue)
	{
		if (aValue.has_value()) {
			return to_vk_pipeline_stage_flags(aValue.value());
		}
		return vk::PipelineStageFlags{};
	}

	vk::AccessFlags to_vk_access_flags(cgb::memory_access aValue)
	{
		vk::AccessFlags result;
		// TODO: This might be a bit expensive. Is there a different possible solution to this?
		if (cgb::is_included(aValue, cgb::memory_access::indirect_command_data_read_access			)) { result |= vk::AccessFlagBits::eIndirectCommandRead; }
		if (cgb::is_included(aValue, cgb::memory_access::index_buffer_read_access					)) { result |= vk::AccessFlagBits::eIndexRead; }
		if (cgb::is_included(aValue, cgb::memory_access::vertex_buffer_read_access					)) { result |= vk::AccessFlagBits::eVertexAttributeRead; }
		if (cgb::is_included(aValue, cgb::memory_access::uniform_buffer_read_access					)) { result |= vk::AccessFlagBits::eUniformRead; }
		if (cgb::is_included(aValue, cgb::memory_access::input_attachment_read_access				)) { result |= vk::AccessFlagBits::eInputAttachmentRead; }
		if (cgb::is_included(aValue, cgb::memory_access::shader_buffers_and_images_read_access		)) { result |= vk::AccessFlagBits::eShaderRead; }
		if (cgb::is_included(aValue, cgb::memory_access::shader_buffers_and_images_write_access		)) { result |= vk::AccessFlagBits::eShaderWrite; }
		if (cgb::is_included(aValue, cgb::memory_access::color_attachment_read_access				)) { result |= vk::AccessFlagBits::eColorAttachmentRead; }
		if (cgb::is_included(aValue, cgb::memory_access::color_attachment_write_access				)) { result |= vk::AccessFlagBits::eColorAttachmentWrite; }
		if (cgb::is_included(aValue, cgb::memory_access::depth_stencil_attachment_read_access		)) { result |= vk::AccessFlagBits::eDepthStencilAttachmentRead; }
		if (cgb::is_included(aValue, cgb::memory_access::depth_stencil_attachment_write_access		)) { result |= vk::AccessFlagBits::eDepthStencilAttachmentWrite; }
		if (cgb::is_included(aValue, cgb::memory_access::transfer_read_access						)) { result |= vk::AccessFlagBits::eTransferRead; }
		if (cgb::is_included(aValue, cgb::memory_access::transfer_write_access						)) { result |= vk::AccessFlagBits::eTransferWrite; }
		if (cgb::is_included(aValue, cgb::memory_access::host_read_access							)) { result |= vk::AccessFlagBits::eHostRead; }
		if (cgb::is_included(aValue, cgb::memory_access::host_write_access							)) { result |= vk::AccessFlagBits::eHostWrite; }
		if (cgb::is_included(aValue, cgb::memory_access::any_read_access							)) { result |= vk::AccessFlagBits::eMemoryRead; }
		if (cgb::is_included(aValue, cgb::memory_access::any_write_access					 		)) { result |= vk::AccessFlagBits::eMemoryWrite; }
		if (cgb::is_included(aValue, cgb::memory_access::transform_feedback_write_access			)) { result |= vk::AccessFlagBits::eTransformFeedbackWriteEXT; }
		if (cgb::is_included(aValue, cgb::memory_access::transform_feedback_counter_read_access		)) { result |= vk::AccessFlagBits::eTransformFeedbackCounterReadEXT; }
		if (cgb::is_included(aValue, cgb::memory_access::transform_feedback_counter_write_access	)) { result |= vk::AccessFlagBits::eTransformFeedbackCounterWriteEXT; }
		if (cgb::is_included(aValue, cgb::memory_access::conditional_rendering_predicate_read_access)) { result |= vk::AccessFlagBits::eConditionalRenderingReadEXT; }
		if (cgb::is_included(aValue, cgb::memory_access::command_process_read_access				)) { result |= vk::AccessFlagBits::eCommandProcessReadNVX; }
		if (cgb::is_included(aValue, cgb::memory_access::command_process_write_access				)) { result |= vk::AccessFlagBits::eCommandProcessWriteNVX; }
		if (cgb::is_included(aValue, cgb::memory_access::color_attachment_noncoherent_read_access	)) { result |= vk::AccessFlagBits::eColorAttachmentReadNoncoherentEXT; }
		if (cgb::is_included(aValue, cgb::memory_access::shading_rate_image_read_access				)) { result |= vk::AccessFlagBits::eShadingRateImageReadNV; }
		if (cgb::is_included(aValue, cgb::memory_access::acceleration_structure_read_access			)) { result |= vk::AccessFlagBits::eAccelerationStructureReadNV; }
		if (cgb::is_included(aValue, cgb::memory_access::acceleration_structure_write_access		)) { result |= vk::AccessFlagBits::eAccelerationStructureWriteNV; }
		if (cgb::is_included(aValue, cgb::memory_access::fragment_density_map_attachment_read_access)) { result |= vk::AccessFlagBits::eFragmentDensityMapReadEXT; }

		return result;
	}

	vk::AccessFlags to_vk_access_flags(std::optional<cgb::memory_access> aValue)
	{
		if (aValue.has_value()) {
			return to_vk_access_flags(aValue.value());
		}
		return vk::AccessFlags{};
	}

}

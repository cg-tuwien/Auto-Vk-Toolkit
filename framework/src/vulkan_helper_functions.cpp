namespace cgb
{
	using namespace cgb::cfg;

	vk::IndexType to_vk_index_type(size_t pSize)
	{
		if (pSize == sizeof(uint16_t)) {
			return vk::IndexType::eUint16;
		}
		if (pSize == sizeof(uint32_t)) {
			return vk::IndexType::eUint32;
		}
		LOG_ERROR(fmt::format("The given size[{}] does not correspond to a valid vk::IndexType", pSize));
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

	vk::ShaderStageFlagBits to_vk_shader_stage(shader_type pType)
	{
		switch (pType) {
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

	vk::SampleCountFlagBits to_vk_sample_count(int pSampleCount)
	{
		switch (pSampleCount) {
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

	int to_cgb_sample_count(vk::SampleCountFlagBits pSampleCount)
	{
		switch (pSampleCount)
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

	vk::VertexInputRate to_vk_vertex_input_rate(input_binding_general_data::kind _Value)
	{
		switch (_Value) {
		case input_binding_general_data::kind::instance:
			return vk::VertexInputRate::eInstance;
		case input_binding_general_data::kind::vertex:
			return vk::VertexInputRate::eVertex;
		default:
			throw std::invalid_argument("Invalid vertex input rate");
		}
	}

	vk::PrimitiveTopology to_vk_primitive_topology(primitive_topology _Value)
	{
		switch (_Value) {
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

	vk::PolygonMode to_vk_polygon_mode(polygon_drawing_mode _Value)
	{
		switch (_Value) {
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

	vk::CullModeFlags to_vk_cull_mode(culling_mode _Value)
	{
		switch (_Value) {
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

	vk::FrontFace to_vk_front_face(winding_order _Value)
	{
		switch (_Value) {
		case winding_order::counter_clockwise:
			return vk::FrontFace::eCounterClockwise;
		case winding_order::clockwise:
			return vk::FrontFace::eClockwise;
		default:
			throw std::invalid_argument("Invalid front face winding order.");
		}
	}

	vk::CompareOp to_vk_compare_op(compare_operation _Value)
	{
		switch(_Value) {
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

	vk::ColorComponentFlags to_vk_color_components(color_channel _Value)
	{
		switch (_Value)	{
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

	vk::BlendFactor to_vk_blend_factor(blending_factor _Value)
	{
		switch (_Value) {
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

	vk::BlendOp to_vk_blend_operation(color_blending_operation _Value)
	{
		switch (_Value)
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

	vk::LogicOp to_vk_logic_operation(blending_logic_operation _Value)
	{
		switch (_Value)
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

	vk::AttachmentLoadOp to_vk_load_op(cfg::attachment_load_operation _Value)
	{
		switch (_Value) {
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

	vk::AttachmentStoreOp to_vk_store_op(cfg::attachment_store_operation _Value)
	{
		switch (_Value) {
		case attachment_store_operation::dont_care:
			return vk::AttachmentStoreOp::eDontCare;
		case attachment_store_operation::store: 
			return vk::AttachmentStoreOp::eStore;
		default:
			throw std::invalid_argument("Invalid attachment store operation.");
		}
	}
}

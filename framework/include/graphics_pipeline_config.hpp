#pragma once

namespace cgb
{
	namespace cfg
	{
		/** Pipeline configuration data: General pipeline settings */
		enum struct pipeline_settings
		{
			nothing					= 0x0000,
			force_new_pipe			= 0x0001,
			fail_if_not_reusable	= 0x0002,
		};

		inline pipeline_settings operator| (pipeline_settings a, pipeline_settings b)
		{
			typedef std::underlying_type<pipeline_settings>::type EnumType;
			return static_cast<pipeline_settings>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
		}

		inline pipeline_settings operator& (pipeline_settings a, pipeline_settings b)
		{
			typedef std::underlying_type<pipeline_settings>::type EnumType;
			return static_cast<pipeline_settings>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
		}

		inline pipeline_settings& operator |= (pipeline_settings& a, pipeline_settings b)
		{
			return a = a | b;
		}

		inline pipeline_settings& operator &= (pipeline_settings& a, pipeline_settings b)
		{
			return a = a & b;
		}

		/** An operation how to compare values - used for specifying how depth testing compares depth values */
		enum struct compare_operation
		{
			never,
			less,
			equal,
			less_or_equal,
			greater,
			not_equal,
			greater_or_equal,
			always
		};

		/** Pipeline configuration data: Depth Test settings */
		struct depth_test
		{
			static depth_test enabled() { return depth_test{ true, compare_operation::less }; }
			static depth_test disabled() { return depth_test{ false, compare_operation::less }; }

			depth_test& set_compare_operation(compare_operation& _WhichOne) { mCompareOperation = _WhichOne; return *this; }

			auto is_enabled() const { return mEnabled; }
			auto depth_compare_operation() const { return mCompareOperation; }

			bool mEnabled;
			compare_operation mCompareOperation;
		};

		/** Pipeline configuration data: Depth Write settings */
		struct depth_write
		{
			static depth_write enabled() { return depth_write{ true }; }
			static depth_write disabled() { return depth_write{ false }; }
			bool is_enabled() const { return mEnabled; }
			bool mEnabled;
		};

		/** Viewport position and extent */
		struct viewport_depth_scissors_config
		{
			static viewport_depth_scissors_config from_pos_extend_and_depth(int32_t x, int32_t y, int32_t width, int32_t height, float minDepth, float maxDepth) 
			{ 
				return viewport_depth_scissors_config{ 
					{static_cast<float>(x), static_cast<float>(y)},
					{static_cast<float>(width), static_cast<float>(height)}, 
					minDepth, maxDepth,
					{x, y},
					{width, height},
					false,
					false
				}; 
			}

			static viewport_depth_scissors_config from_pos_and_extent(int32_t x, int32_t y, int32_t width, int32_t height) 
			{ 
				return viewport_depth_scissors_config{ 
					{static_cast<float>(x), static_cast<float>(y)},
					{static_cast<float>(width), static_cast<float>(height)}, 
					0.0f, 1.0f,
					{x, y},
					{width, height},
					false,
					false
				}; 
			}

			static viewport_depth_scissors_config from_extent(int32_t width, int32_t height) 
			{ 
				return viewport_depth_scissors_config{ 
					{0.0f, 0.0f},
					{static_cast<float>(width), static_cast<float>(height)}, 
					0.0f, 1.0f,
					{0, 0},
					{width, height},
					false,
					false
				}; 
			}

			static viewport_depth_scissors_config from_window(const window* window) 
			{ 
				auto dimensions = window->resolution(); // TODO: Is this the right one?
				return viewport_depth_scissors_config{ 
					{0.0f, 0.0f},
					{static_cast<float>(dimensions.x), static_cast<float>(dimensions.y)}, 
					0.0f, 1.0f,
					{0, 0},
					{static_cast<int32_t>(dimensions.x), static_cast<int32_t>(dimensions.y)},
					false,
					false
				}; 
			}

			viewport_depth_scissors_config& enable_dynamic_viewport() { mDynamicViewportEnabled = true; return *this; }
			viewport_depth_scissors_config& disable_dynamic_viewport() { mDynamicViewportEnabled = false; return *this; }
			viewport_depth_scissors_config& enable_dynamic_scissor() { mDynamicScissorEnabled = true; return *this; }
			viewport_depth_scissors_config& disable_dynamic_scissor() { mDynamicScissorEnabled = false; return *this; }

			const auto& position() const { return mPosition; }
			auto x() const { return mPosition.x; }
			auto y() const { return mPosition.y; }
			const auto& dimensions() const { return mDimensions; }
			auto width() const { return mDimensions.x; }
			auto height() const { return mDimensions.y; }
			auto min_depth() const { return mMinDepth; }
			auto max_depth() const { return mMaxDepth; }
			const auto& scissor_offset() const { return mScissorOffset; }
			auto scissor_x() const { return mScissorOffset.x; }
			auto scissor_y() const { return mScissorOffset.y; }
			const auto& scissor_extent() const { return mScissorExtent; }
			auto scissor_width() const { return mScissorExtent.x; }
			auto scissor_height() const { return mScissorExtent.y; }
			auto is_dynamic_viewport_enabled() const { return mDynamicViewportEnabled; }
			auto is_dynamic_scissor_enabled() const { return mDynamicScissorEnabled; }

			glm::vec2 mPosition;
			glm::vec2 mDimensions;
			float mMinDepth;
			float mMaxDepth;
			glm::ivec2 mScissorOffset;
			glm::ivec2 mScissorExtent;
			bool mDynamicViewportEnabled;
			bool mDynamicScissorEnabled;
		};

		/** Pipeline configuration data: Culling Mode */
		enum struct culling_mode
		{
			 disabled,
			 cull_front_faces,
			 cull_back_faces,
			 cull_front_and_back_faces
		};

		/** Winding order of polygons */
		enum struct winding_order
		{
			counter_clockwise,
			clockwise
		};

		/** Pipeline configuration data: Culling Mode */
		struct front_face
		{
			static front_face define_front_faces_to_be_counter_clockwise() { return front_face{ winding_order::counter_clockwise }; }
			static front_face define_front_faces_to_be_clockwise() { return front_face{ winding_order::clockwise }; }

			winding_order winding_order_of_front_faces() const { return mFrontFaces; }
			winding_order winding_order_of_back_faces() const { return mFrontFaces == winding_order::counter_clockwise ? winding_order::clockwise : winding_order::counter_clockwise; }

			winding_order mFrontFaces;
		};

		/** How to draw polygons */
		enum struct polygon_drawing_mode
		{
			fill,
			line,
			point
		};

		/** Pipeline configuration data: Polygon Drawing Mode (and additional settings) */
		struct polygon_drawing
		{
			static polygon_drawing config_for_filling() 
			{ 
				return { polygon_drawing_mode::fill, 1.0f, false, 1.0f }; 
			}
			
			static polygon_drawing config_for_lines(float pLineWidth = 1.0f, bool pDynamic = false) 
			{ 
				return { polygon_drawing_mode::line, pLineWidth, pDynamic, 1.0f }; 
			}
			
			static polygon_drawing config_for_points(float pPointSize = 1.0f) 
			{ 
				return { polygon_drawing_mode::point, 1.0f, false, pPointSize }; 
			}

			auto drawing_mode() const { return mDrawingMode; }
			auto line_width() const { return mLineWidth; }
			auto dynamic_line_width() const { return mDynamicLineWidth; }

			polygon_drawing_mode mDrawingMode;
			float mLineWidth;
			bool mDynamicLineWidth;

			float mPointSize;
		};

		/** How the rasterizer processes geometry */
		enum struct rasterizer_geometry_mode
		{
			rasterize_geometry,
			discard_geometry,
		};

		/** Additional depth-related parameters for the rasterizer */
		struct depth_clamp_bias
		{
			static depth_clamp_bias config_nothing_special() { return { false, false, 0.0f, 0.0f, 0.0f, false }; }
			static depth_clamp_bias config_enable_depth_bias(float pConstantFactor, float pBiasClamp, float pSlopeFactor) { return { false, true, pConstantFactor, pBiasClamp, pSlopeFactor, false }; }
			static depth_clamp_bias config_enable_clamp_and_depth_bias(float pConstantFactor, float pBiasClamp, float pSlopeFactor) { return { true, true, pConstantFactor, pBiasClamp, pSlopeFactor, false }; }

			depth_clamp_bias& enable_dynamic_depth_bias() { mEnableDynamicDepthBias = true; return *this; }
			depth_clamp_bias& disable_dynamic_depth_bias() { mEnableDynamicDepthBias = false; return *this; }

			auto is_clamp_to_frustum_enabled() const { return mClampDepthToFrustum; }
			auto is_depth_bias_enabled() const { return mEnableDepthBias; }
			auto bias_constant_factor() const { return mDepthBiasConstantFactor; }
			auto bias_clamp_value() const { return mDepthBiasClamp; }
			auto bias_slope_factor() const { return mDepthBiasSlopeFactor; }
			auto is_dynamic_depth_bias_enabled() const { return mEnableDynamicDepthBias; }

			bool mClampDepthToFrustum;
			bool mEnableDepthBias;
			float mDepthBiasConstantFactor;
			float mDepthBiasClamp;
			float mDepthBiasSlopeFactor;
			bool mEnableDynamicDepthBias;
		};

		/** Settings to enable/disable depth bounds and for its range */
		struct depth_bounds
		{
			static depth_bounds disable() { return {false, 0.0f, 1.0f}; }
			static depth_bounds enable(float _RangeFrom, float _RangeTo) { return {true, _RangeFrom, _RangeTo}; }

			auto is_enabled() const { return mEnabled; }
			auto is_dynamic_depth_bounds_enabled() const { return mEnabled; }
			auto min_bounds() const { return mMinDeptBounds; }
			auto max_bounds() const { return mMaxDepthBounds; }

			bool mEnabled;
			float mMinDeptBounds;
			float mMaxDepthBounds;
		};

		/** Reference the separate color channels */
		enum struct color_channel
		{
			none		= 0x0000,
			red			= 0x0001,
			green		= 0x0002,
			blue		= 0x0004,
			alpha		= 0x0008,
			rg			= red | green,
			rgb			= red | green | blue,
			rgba		= red | green | blue | alpha
		};

		inline color_channel operator| (color_channel a, color_channel b)
		{
			typedef std::underlying_type<color_channel>::type EnumType;
			return static_cast<color_channel>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
		}

		inline color_channel operator& (color_channel a, color_channel b)
		{
			typedef std::underlying_type<color_channel>::type EnumType;
			return static_cast<color_channel>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
		}

		inline color_channel& operator |= (color_channel& a, color_channel b)
		{
			return a = a | b;
		}

		inline color_channel& operator &= (color_channel& a, color_channel b)
		{
			return a = a & b;
		}

		/** Different operation types for color blending */
		enum struct color_blending_operation
		{
			add,
			subtract,
			reverse_subtract,
			min,
			max
		};

		/** Different factors for color blending operations */
		enum struct blending_factor
		{
			zero,
			one,
			source_color,
			one_minus_source_color,
			destination_color,
			one_minus_destination_color,
			source_alpha,
			one_minus_source_alpha,
			destination_alpha,
			one_minus_destination_alpha,
			constant_color,
			one_minus_constant_color,
			constant_alpha,
			one_minus_constant_alpha,
			source_alpha_saturate
		};

		/** Different types operation types for `color_blending_mode::logic_operation` mode */
		enum struct blending_logic_operation
		{
			op_clear,
			op_and,
			op_and_reverse,
			op_copy,
			op_and_inverted,
			no_op,
			op_xor,
			op_or,
			op_nor,
			op_equivalent,
			op_invert,
			op_or_reverse,
			op_copy_inverted,
			op_or_inverted,
			op_nand,
			op_set
		};

		/** Some color blending settings */
		struct color_blending_settings
		{
			static color_blending_settings disable_logic_operation() { return color_blending_settings{ {}, {} }; }
			static color_blending_settings config_blend_constants(glm::vec4 pValues) { return color_blending_settings{ {}, pValues }; }
			static color_blending_settings enable_logic_operation(blending_logic_operation pLogicOp) { return color_blending_settings{ pLogicOp, {} }; }

			bool is_logic_operation_enabled() const { return mLogicOpEnabled.has_value(); }
			blending_logic_operation logic_operation() const { return mLogicOpEnabled.value_or(blending_logic_operation::no_op); }
			const auto& blend_constants() const { return mBlendConstants; }

			std::optional<blending_logic_operation> mLogicOpEnabled;
			glm::vec4 mBlendConstants;
		};

		/** A specific color blending config for an attachment (or for all attachments) */
		struct color_blending_config
		{
			static color_blending_config disable()
			{
				return color_blending_config{ 
					{},
					false,
					color_channel::rgba,
					blending_factor::one, blending_factor::zero, color_blending_operation::add,
					blending_factor::one, blending_factor::zero, color_blending_operation::add
				};
			}

			static color_blending_config disable_blending_for_attachment(uint32_t pAttachment, color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{ 
					pAttachment,
					false,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::zero, color_blending_operation::add,
					blending_factor::one, blending_factor::zero, color_blending_operation::add
				};
			}

			static color_blending_config enable_alpha_blending_for_all_attachments(color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{ 
					{},
					true,
					pAffectedColorChannels,
					blending_factor::source_alpha, blending_factor::one_minus_source_alpha, color_blending_operation::add,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add
				};
			}

			static color_blending_config enable_alpha_blending_for_attachment(uint32_t pAttachment, color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{
					pAttachment,
					true,
					pAffectedColorChannels,
					blending_factor::source_alpha, blending_factor::one_minus_source_alpha, color_blending_operation::add,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add
				};
			}

			static color_blending_config enable_premultiplied_alpha_blending_for_all_attachments(color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{ 
					{},
					true,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add
				};
			}

			static color_blending_config enable_premultiplied_alpha_blending_for_attachment(uint32_t pAttachment, color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{
					pAttachment,
					true,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add,
					blending_factor::one, blending_factor::one_minus_source_alpha, color_blending_operation::add
				};
			}

			static color_blending_config enable_additive_for_all_attachments(color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{ 
					{},
					true,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::one, color_blending_operation::add,
					blending_factor::one, blending_factor::one, color_blending_operation::add
				};
			}

			static color_blending_config enable_additive_for_attachment(uint32_t pAttachment, color_channel pAffectedColorChannels = color_channel::rgba)
			{
				return color_blending_config{
					pAttachment,
					true,
					pAffectedColorChannels,
					blending_factor::one, blending_factor::one, color_blending_operation::add,
					blending_factor::one, blending_factor::one, color_blending_operation::add
				};
			}

			bool has_specific_target_attachment()  const { return mTargetAttachment.has_value(); }
			auto target_attachment() const { return mTargetAttachment; }
			auto is_blending_enabled() const { return mEnabled; }
			auto affected_color_channels() const { return mAffectedColorChannels; }
			auto color_source_factor() const { return mIncomingColorFactor; }
			auto color_destination_factor() const { return mExistingColorFactor; }
			auto color_operation() const { return mColorOperation; }
			auto alpha_source_factor() const { return mIncomingAlphaFactor; }
			auto alpha_destination_factor() const { return mExistingAlphaFactor; }
			auto alpha_operation() const { return mAlphaOperation; }

			// affected color attachment
			// This value must equal the colorAttachmentCount for the subpass in which this pipeline is used.
			std::optional<uint32_t> mTargetAttachment;
			// blending enabled
			bool mEnabled;
			// affected color channels a.k.a. write mask
			color_channel mAffectedColorChannels;
			// incoming a.k.a. source color
			blending_factor mIncomingColorFactor;
			// existing a.k.a. destination color
			blending_factor mExistingColorFactor;
			// incoming*factor -operation- existing*factor
			color_blending_operation mColorOperation;
			// incoming a.k.a. source alpha
			blending_factor mIncomingAlphaFactor;
			// existing a.k.a. destination alpha
			blending_factor mExistingAlphaFactor;
			// incoming*factor -operation- existing*factor
			color_blending_operation mAlphaOperation;
		};

		/** How the vertex input is to be interpreted topology-wise */
		enum struct primitive_topology
		{
			points,
			lines,
			line_strip,
			triangles,
			triangle_strip,
			triangle_fan,
			lines_with_adjacency,
			line_strip_with_adjacency,
			triangles_with_adjacency,
			triangle_strip_with_adjacency,
			patches
		};
	}

	// Forward declare that the graphics_pipeline_t class for the context_specific_function
	class graphics_pipeline_t;
	
	/** Pipeline configuration data: BIG GRAPHICS PIPELINE CONFIG STRUCT */
	struct graphics_pipeline_config
	{
		graphics_pipeline_config();
		graphics_pipeline_config(graphics_pipeline_config&&) = default;
		graphics_pipeline_config(const graphics_pipeline_config&) = delete;
		graphics_pipeline_config& operator=(graphics_pipeline_config&&) = default;
		graphics_pipeline_config& operator=(const graphics_pipeline_config&) = delete;
		~graphics_pipeline_config() = default;

		cfg::pipeline_settings mPipelineSettings; // ?
		std::optional<std::tuple<renderpass, uint32_t>> mRenderPassSubpass;
		std::vector<input_binding_location_data> mInputBindingLocations;
		cfg::primitive_topology mPrimitiveTopology;
		std::vector<shader_info> mShaderInfos;
		std::vector<cfg::viewport_depth_scissors_config> mViewportDepthConfig;
		cfg::rasterizer_geometry_mode mRasterizerGeometryMode;
		cfg::polygon_drawing mPolygonDrawingModeAndConfig;
		cfg::culling_mode mCullingMode;
		cfg::front_face mFrontFaceWindingOrder;
		cfg::depth_clamp_bias mDepthClampBiasConfig;
		cfg::depth_test mDepthTestConfig;
		cfg::depth_write mDepthWriteConfig;
		cfg::depth_bounds mDepthBoundsConfig;
		std::vector<cfg::color_blending_config> mColorBlendingPerAttachment;
		cfg::color_blending_settings mColorBlendingSettings;
		std::vector<binding_data> mResourceBindings;
		std::vector<push_constant_binding_data> mPushConstantsBindings;
	};



}
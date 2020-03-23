#include <cg_base.hpp>

namespace cgb
{
	using namespace cgb::cfg;

	// Set sensible defaults:
	graphics_pipeline_config::graphics_pipeline_config()
		: mPipelineSettings{ pipeline_settings::nothing }
		, mRenderPassSubpass {} // not set by default
		, mPrimitiveTopology{ primitive_topology::triangles } // triangles after one another
		, mRasterizerGeometryMode{ rasterizer_geometry_mode::rasterize_geometry } // don't discard, but rasterize!
		, mPolygonDrawingModeAndConfig{ polygon_drawing::config_for_filling() } // Fill triangles
		, mCullingMode{ culling_mode::cull_back_faces } // Cull back faces
		, mFrontFaceWindingOrder{ front_face::define_front_faces_to_be_counter_clockwise() } // CCW == front face
		, mDepthClampBiasConfig{ depth_clamp_bias::config_nothing_special() } // no clamp, no bias, no factors
		, mDepthTestConfig{ depth_test::enabled() } // enable depth testing
		, mDepthWriteConfig{ depth_write::enabled() } // enable depth writing
		, mDepthBoundsConfig{ depth_bounds::disable() }
		, mColorBlendingSettings{ color_blending_settings::disable_logic_operation() }
	{
	}

	namespace cfg
	{
		viewport_depth_scissors_config viewport_depth_scissors_config::from_window(window* aWindow)
		{
			if (nullptr == aWindow) {
				aWindow = cgb::context().main_window();
			}
			
			auto dimensions = aWindow->resolution(); // TODO: Is this the right one?
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
	}
}

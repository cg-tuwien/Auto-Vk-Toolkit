#include <cg_base.hpp>

namespace cgb
{
	ray_tracing_pipeline_config::ray_tracing_pipeline_config()
		: mPipelineSettings{ cfg::pipeline_settings::nothing }
		, mShaderTableConfig{ }
		, mMaxRecursionDepth{ 2u }
	{

	}
}

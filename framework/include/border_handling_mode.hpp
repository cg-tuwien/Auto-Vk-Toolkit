#pragma once

namespace cgb
{
	/** Configuration struct to specify the border handling strategy for texture sampling. */
	enum struct border_handling_mode
	{
		clamp_to_edge,
		mirror_clamp_to_edge,
		clamp_to_border,
		repeat,
		mirrored_repeat,
	};
}

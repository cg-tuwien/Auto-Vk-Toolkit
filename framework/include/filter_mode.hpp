#pragma once

namespace cgb
{
	/** Configuration struct to specify the filtering strategy for texture sampling. */
	enum struct filter_mode
	{
		nearest_neighbor,
		bilinear,
		trilinear,
		cubic,
		anisotropic_2x,
		anisotropic_4x,
		anisotropic_8x,
		anisotropic_16x,
	};
}

#pragma once

namespace cgb
{
	/** Allows to configure whether to go for many or for a single queue. */
	enum struct device_queue_selection_strategy
	{
		prefer_separate_queues,
		prefer_everything_on_single_queue,
	};
}

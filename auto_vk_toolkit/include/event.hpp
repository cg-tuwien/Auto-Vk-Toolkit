#pragma once

#include "event_data.hpp"

namespace avk
{
	/** Base class for updater-events */
	class event
	{
	public:
		virtual ~event() = default;

		/**	Called frequently by the `avk::updater`.
		 *	An event must evaluate whatever it has to evaluate and must return:
		 *	 -> true if the event has occured.
		 *	 -> false if the event has not occured.
		 *
		 *	@param	aData		Data storage where every event instance may gather
		 *						some data which might be used by updatees.
		 */
		virtual bool update(event_data& aData) = 0;
	};
}

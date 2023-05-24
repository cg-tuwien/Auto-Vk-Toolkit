#pragma once

#include "invokee.hpp"

namespace avk
{
	/**	@brief Handle all @ref invokee sequentially!
	 */
	class sequential_invoker
	{
	public:
		/** Invoke all the update() methods in a sequential fashion, 
		 *  if the respective instance is enabled.
		 */
		void invoke_updates(const std::vector<invokee*>& elements)
		{
			for (auto& e : elements) {
				if (e->is_enabled()) {
					e->update();
				}
			}
		}

		/** Invoke all the render() methods in a sequential fashion,
		 *  if the respective instance is enabled.
		 *	Also pay attention to any pending updater actions.
		 */
		void invoke_renders(const std::vector<invokee*>& elements)
		{
			updater::prepare_for_current_frame();
			for (auto& e : elements) {
				if (e->is_enabled()) {
					// First, apply potential changes required by the updater of the invokee,
					// if one really exist. It is important that those changes are applied
					// before the next render call.
					e->apply_recreation_updates();
				}
				if (e->is_render_enabled()) {
					e->render();
				}
			}
		}
	};
}

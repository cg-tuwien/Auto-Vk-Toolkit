#pragma once
#include <gvk.hpp>

namespace gvk
{
	/**	@brief Handle all @ref invokee sequentially!
	 *
	 *	An invoker compatible with the @ref composition class.
	 *	All @ref invokee instances which are managed by one @ref composition
	 *	are handled through an invoker. The @ref sequential_invoker updates,
	 *	renders, etc. all of them one after the other.
	 */
	class sequential_invoker : public invoker_interface
	{
	public:
		void execute_handle_enablings(const std::vector<invokee*>& elements) override
		{
			for (auto& e : elements)
			{
				e->handle_enabling();
			}
		}

		void execute_fixed_updates(const std::vector<invokee*>& elements) override
		{
			for (auto& e : elements)
			{
				if (e->is_enabled()) {
					e->fixed_update();
				}
			}
		}

		void execute_updates(const std::vector<invokee*>& elements) override
		{
			for (auto& e : elements)
			{
				if (e->is_enabled()) {
					e->update();										
				}
			}
		}

		void execute_renders(const std::vector<invokee*>& elements) override
		{
			for (auto& e : elements)
			{
				if (e->is_enabled())
				{
					// first apply potential changes required by the updater of the invokee, 
					// should one really exist. It is important that those changes are applied
					// before the next render call.
					e->apply_recreation_updates();
				}
				if (e->is_render_enabled()) {
					
					e->render();
				}
			}
		}

		void execute_render_gizmos(const std::vector<invokee*>& elements) override
		{
			for (auto& e : elements)
			{
				if (e->is_render_gizmos_enabled()) {
					e->render_gizmos();
				}
			}
		}

		void execute_handle_disablings(const std::vector<invokee*>& elements) override
		{
			for (auto& e : elements)
			{
				e->handle_disabling();
			}
		}
	};
}

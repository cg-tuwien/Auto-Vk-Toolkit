#pragma once

namespace cgb
{
	/**	@brief Handle all @ref cg_element sequentially!
	 *
	 *	An executor compatible with the @ref composition class.
	 *	All @ref cg_element instances which are managed by one @ref composition
	 *	are handled through an executor. The @ref sequential_executor updates,
	 *	renders, etc. all of them one after the other.
	 */
	class sequential_executor
	{
	public:
		sequential_executor(composition_interface* pComposition)
			: mParentComposition(pComposition) 
		{}

		void execute_handle_enablings(const std::vector<cg_element*>& elements)
		{
			for (auto& e : elements)
			{
				e->handle_enabling();
			}
		}

		void execute_fixed_updates(const std::vector<cg_element*>& elements)
		{
			for (auto& e : elements)
			{
				if (e->is_enabled()) {
					e->fixed_update();
				}
			}
		}

		void execute_updates(const std::vector<cg_element*>& elements)
		{
			for (auto& e : elements)
			{
				if (e->is_enabled()) {
					e->update();
				}
			}
		}

		void execute_renders(const std::vector<cg_element*>& elements)
		{
			for (auto& e : elements)
			{
				if (e->is_render_enabled()) {
					e->render();
				}
			}
		}

		void execute_render_gizmos(const std::vector<cg_element*>& elements)
		{
			for (auto& e : elements)
			{
				if (e->is_render_gizmos_enabled()) {
					e->render_gizmos();
				}
			}
		}

		void execute_render_guis(const std::vector<cg_element*>& elements)
		{
			for (auto& e : elements)
			{
				if (e->is_render_gui_enabled()) {
					e->render_gui();
				}
			}
		}

		void execute_handle_disablings(const std::vector<cg_element*>& elements)
		{
			for (auto& e : elements)
			{
				e->handle_disabling();
			}
		}

	private:
		composition_interface* mParentComposition;
	};
}

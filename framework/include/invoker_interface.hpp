#pragma once
#include <gvk.hpp>

namespace gvk
{
	class invoker_interface
	{
	public:
		virtual ~invoker_interface() {};
		virtual void execute_handle_enablings(const std::vector<invokee*>&) = 0;
		virtual void execute_updates(const std::vector<invokee*>&) = 0;
		virtual void execute_renders(const std::vector<invokee*>&) = 0;
		virtual void execute_handle_disablings(const std::vector<invokee*>&) = 0;
	};
}

#pragma once

namespace xk
{
	template <typename TTimer, typename TExecutor>
	static composition<TTimer, TExecutor> setup()
	{
		return xk::composition<TTimer, TExecutor>();
	}

	template <typename TTimer, typename TExecutor, typename... Args>
	static composition<TTimer, TExecutor> setup(Args&&... args)
	{
		static_assert((std::is_lvalue_reference<Args>::value && ...), "Can not handle the lifetimes of cg_elements; they must be handled by the user. Only pass l-value references to cg_element instances!");
		return xk::composition<TTimer, TExecutor>({static_cast<cg_element*>(&args)...});
	}

	static auto setup()
	{
		return composition<varying_update_timer, sequential_executor>();
	}

	template <typename... Args>
	static auto setup(Args&&... args)
	{
		static_assert((std::is_lvalue_reference<Args>::value && ...), "Can not handle the lifetimes of cg_elements; they must be handled by the user. Only pass l-value references to cg_element instances!");
		return composition<varying_update_timer, sequential_executor>({static_cast<cg_element*>(&args)...});
	}
	
	
}

#pragma once

#include "timer_frame_type.hpp"

namespace avk
{
	/** Base class (actually an "interface") which all concrete timer_interface 
	 *	implementations have to implement in order to be usable with @ref run.
	 *	
	 *	Please note that, in addition to implementing all pure virtual
	 *	functions, subclasses must implement a timer_frame_type tick();
	 *	method in order to be usable with the framework. (Please investigate
	 *	the implementation of @ref composition for details
	 *	on how timers and the tick-method in particular are used.
	 */
	class timer_interface
	{
	public:
		virtual ~timer_interface() {}

		/** @brief To be invoked once per frame... tick... tick... tick...
		 */
		virtual timer_frame_type tick() = 0;

		/**	@brief The absolute system time.
		 */
		virtual float absolute_time() const = 0;

		/**	@brief The time at the beginning of the current frame.
		 *
		 *	Please note that "frame" in this case also refers to 
		 *	simulation-only frames, i.e. those frames where only 
		 *	update is invoked, but not rendering.
		 *	I.e., a simulation-only update frame will have a different
		 *	frame-time than the following update-and-render frame.
		 */
		virtual float time_since_start() const = 0;

		/**	@brief The duration of the fixed simulation timestep 
		 *
		 *	This will return the fixed delta time IF the used timer_interface
		 *	supports fixed timesteps. If it doesn't, it will return the
		 *	same as @ref delta_time, i.e. a varying delta time.
		 */
		virtual float fixed_delta_time() const = 0;

		/** @brief The time it took to complete the last frame
		 *	This value usually varies, i.e. it is a varying delta time.
		 */
		virtual float delta_time() const = 0;

		/** @brief The scale at which the time is passing 
		 */
		virtual float time_scale() const = 0;

		/**	@brief The absolute system time in double precision
		*/
		virtual double absolute_time_dp() const = 0;

		/**	@brief The time at the beginning of the current frame in double precision.
		*
		*	Please note that "frame" in this case also refers to
		*	simulation-only frames, i.e. those frames where only
		*	update is invoked, but not rendering.
		*	I.e., a simulation-only update frame will have a different
		*	frame-time than the following update-and-render frame.
		*/
		virtual double time_since_start_dp() const = 0;
		
		/**	@brief The duration of the fixed simulation timestep in double precision
		*
		*	This will return the fixed delta time IF the used timer_interface
		*	supports fixed timesteps. If it doesn't, it will return the
		*	same as @ref delta_time, i.e. a varying delta time.
		*/
		virtual double fixed_delta_time_dp() const = 0;
		
		/** @brief The time it took to complete the last frame in double precision
		*
		*	This value usually varies. Generally speaking, use delta_time
		 *	This value usually varies, i.e. it is a varying delta time.
		*/
		virtual double delta_time_dp() const = 0;
		
		/** @brief The scale at which the time is passing in double precision
		*/
		virtual double time_scale_dp() const = 0;
	};	
}

namespace avk
{
#pragma region global data representing the currently active composition
	/**	@brief Get the current timer, which represents the current game-/render-time
	 *	This can be nullptr! Use time() for a version which will not return nullptr.
	 */
	 /**	@brief Get the current timer, which represents the current game-/render-time
  *	This can be nullptr! Use time() for a version which will not return nullptr.
  */
	extern timer_interface*& timer_reference();

	/**	@brief Sets a new timer.
	 *	ATTENTION: This timer must live until the end of the application!
	 */
	extern void set_timer(timer_interface* const aPointerToNewTimer);

	/**	@brief Get the current timer, which represents the current game-/render-time
	 */
	extern timer_interface& time();

	extern void set_default_timer();
}

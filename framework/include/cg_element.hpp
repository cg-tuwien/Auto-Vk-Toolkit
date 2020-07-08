#pragma once

namespace xk
{
	/**	@brief Override this base class for objects to update and/or render
	 * 
	 *	Base class for any object which should be updated or rendered
	 *	during a @ref run. A specific object should override all the 
	 *	virtual methods which it would like to hook-into with custom
	 *	functionality, just leave the other virtual methods in their
	 *	empty default implementation.
	 *
	 *	Invocation order of cg_element's methods:
	 *	  1. initialize
	 *	  loop:
	 *	    2. check and possibly issue on_enable event handlers
	 *	    3. fixed_update 
	 *	       possibly continue; // depending on the timer_interface used
	 *	    4. update
	 *	    5. render
	 *	    6. render_gizmos
	 *	    7. check and possibly issue on_disable event handlers
	 *	  loop-end
	 *	  8. finalize
	 */
	class cg_element
	{
	public:
		/**
		 * @brief Constructor which automatically generates a name for this object
		 */
		cg_element()
			: mName{ "cg_element #" + std::to_string(sGeneratedNameId++) }
			, mWasEnabledLastFrame{ false }
			, mEnabled{ true }
			, mRenderEnabled{ true }
			, mRenderGizmosEnabled{ true }
		{ }

		/**	@brief Constructor
		 *	@param pName Name by which this object can be identified
		 *	@param pIsEnabled Enabled means that this object will receive the method invocations during the loop
		 */
		cg_element(std::string pName, bool pIsEnabled = true) 
			: mName{ pName }
			, mWasEnabledLastFrame{ false }
			, mEnabled{ pIsEnabled }
			, mRenderEnabled{ true }
			, mRenderGizmosEnabled{ true }
		{ }

		virtual ~cg_element()
		{
			// Make sure, this element gets removed from the composition
			xk::current_composition().remove_element_immediately(*this, true);
		}

		/** Returns the name of this cg_element */
		const std::string& name() const { return mName; }

		/** Returns the desired execution order of this cg_element w.r.t. the default time 0.
		 *	cg_elements with negative execution orders will get their initialize-, update-,
		 *	render-, etc. methods invoked earlier; cg_elements with positive execution orders
		 *	will be invoked later.
		 */
		virtual int execution_order() const { return 0; }

		/**	@brief Initialize this cg_element
		 *
		 *	This is the first method in the lifecycle of a cg_element,
		 *	that is invoked by the framework during a @ref run. 
		 *	It is called only once.
		 */
		virtual void initialize() {}

		/**	@brief Update this cg_element at fixed intervals
		 *
		 *	This method is called at fixed intervals with a fixed 
		 *	delta time. Please note that this only applies to timers
		 *	which support fixed update intervals. For all other timers,
		 *	this method will simply be called each frame before the 
		 *	regular @ref update is called.
		 */
		virtual void fixed_update() {}

		/**	@brief Update this cg_element before rendering with varying delta time
		 *
		 *	This method is called at varying intervals. Query the
		 *	timer_interface's delta time to get the time which has passed since
		 *	the last frame. This method will always be called after
		 *	@ref fixed_update and before @ref render.
		 */
		virtual void update() {}

		/**	@brief Render this cg_element 
		 *
		 *	This method is called whenever this cg_element should
		 *	perform its rendering tasks. It is called right after
		 *	all the @ref update methods have been invoked.
		 */
		virtual void render() {}

		/**	@brief Render gizmos for this cg_element
		 *
		 *	This method can be used to render additional information
		 *	about this cg_element like, for instance, debug information.
		 *	This method will always be called after all @ref render
		 *	methods of the current @ref run have been invoked.
		 */
		virtual void render_gizmos() {}

		/**	@brief Cleanup this cg_element
		 *
		 *	This is the last method in the lifecycle of a cg_element,
		 *	that is invoked by the framework during a @ref run.
		 *	It is called only once.
		 */
		virtual void finalize() {}

		/**	@brief Enable this element
		 *
		 *	@param pAlsoEnableRendering Set to true to also enable
		 *  rending of this element. If set to false, the flags which 
		 *  indicate whether or not to render, and render gizmos
		 *  will remain unchanged.
		 *
		 *	Call this method to enable this cg_element by the end of 
		 *	the current frame! I.e. this method expresses your intent
		 *	to enable. Perform neccessary actions in the on_enable() 
		 *	event handler method (not here!).
		 *
		 *	It is strongly recommended not to override this method!
		 *	However, if you do, make sure to set the flag, i.e. call 
		 *	base class' implementation (i.e. this one).
		 */
		virtual void enable(bool pAlsoEnableRendering = true) 
		{ 
			mEnabled = true; 
			if (pAlsoEnableRendering) {
				mRenderEnabled = true;
				mRenderGizmosEnabled = true;
			}
		}

		/**	@brief Handle the event of this cg_element having been enabled
		 *
		 *	Perform all your "This element has been enabled" actions within
		 *	this event handler method! 
		 */
		virtual void on_enable() {}

		/**	@brief Disable this element
		 *
		 *	@param pAlsoDisableRendering Set to true to also disable
		 *  rending of this element. If set to false, the flags which
		 *  indicate whether or not to render, render gizmos, and render
		 *  gui of this element, will remain unchanged.
		 *
		 *	Call this method to disable this cg_element by the end of
		 *	the current frame! I.e. this method expresses your intent
		 *	to disable. Perform neccessary actions in the on_disable()
		 *	event handler method (not here!).
		 *
		 *	It is strongly recommended not to override this method!
		 *	However, if you do, make sure to set the flag, i.e. call
		 *	base class' implementation (i.e. this one).
		 */
		virtual void disable(bool pAlsoDisableRendering = true)
		{ 
			mEnabled = false; 
			if (pAlsoDisableRendering) {
				mRenderEnabled = false;
				mRenderGizmosEnabled = false;
			}
		}

		/**	@brief Handle the event of this cg_element having been disabled
		 *
		 *	Perform all your "This element has been disabled" actions within
		 *	this event handler method! It will be called regardless of 
		 *	whether the disable request has come from within this cg_element
		 *	or from outside.
		 */
		virtual void on_disable() {}

		/**	@brief Checks if the element has been enabled and if so, issues 
		 *	the on_enable() event method. Also updates the internal state.
		 *
		 *	This method will be called by the composition. Do not 
		 *	call it directly!! (Unless, of course, you really know
		 *	what you are doing)
		 */
		void handle_enabling() 
		{
			if (mEnabled && !mWasEnabledLastFrame) {
				on_enable();
				update_enabled_state();
			}
		}

		/**	@brief Checks if the element has been enabled and if so, issues
		 *	the on_enable() event method. Also updates the internal state.
		 *
		 *	This method will be called by the composition. Do not
		 *	call it directly!! (Unless, of course, you really know
		 *	what you are doing)
		 */
		void handle_disabling()
		{
			if (!mEnabled && mWasEnabledLastFrame) {
				on_disable();
				update_enabled_state();
			}
		}

		/**	@brief Also updates the internal enabled/disabled state.
		 *
		 *	This method will be called internally. Do not call it 
		 *  directly!! (Unless, of course, you really know what you 
		 *	are doing)
		 */
		void update_enabled_state()
		{
			mWasEnabledLastFrame = mEnabled;
		}

		/**	@brief Returns whether or not this element is currently enabled. */
		bool is_enabled() const { return mEnabled; }

		/** @brief Enable or disable rendering of this element
		 *	@param pValue true to enable, false to disable
		 */
		void set_render_enabled(bool pValue) { mRenderEnabled = pValue; }

		/** @brief Enable or disable rendering of this element's gizmos
		 *	@param pValue true to enable, false to disable
		 */
		void set_render_gizmos_enabled(bool pValue) { mRenderGizmosEnabled = pValue; }

		/** @brief Returns whether rendering of this element is enabled or not. */
		bool is_render_enabled() const { return mRenderEnabled; }

		/** @brief Returns whether rendering this element's gizmos is enabled or not. */
		bool is_render_gizmos_enabled() const { return mRenderGizmosEnabled; }

	private:
		inline static int32_t sGeneratedNameId = 0;
		std::string mName;
		bool mWasEnabledLastFrame;
		bool mEnabled;
		bool mRenderEnabled;
		bool mRenderGizmosEnabled;
	};
}

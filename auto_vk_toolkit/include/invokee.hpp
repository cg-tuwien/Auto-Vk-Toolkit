#pragma once


#include "updater.hpp"
#include "composition_interface.hpp"

namespace avk
{
	/**	@brief Override this base class for objects to update and/or render
	 * 
	 *	Base class for any object which should be updated or rendered
	 *	during a @ref run. A specific object should override all the 
	 *	virtual methods which it would like to hook-into with custom
	 *	functionality, just leave the other virtual methods in their
	 *	empty default implementation.
	 *
	 *	Invocation order of invokee's methods:
	 *	  1. initialize
	 *	  loop:
	 *	    2. check and possibly issue on_enable event handlers
	 *	    3. update
	 *	    4. render
	 *	    5. check and possibly issue on_disable event handlers
	 *	  loop-end
	 *	  6. finalize
	 */
	class invokee
	{
	public:
		/**
		 * @brief Constructor which automatically generates a name for this object
		 * @param aExecutionOrder sets the desired execution order of this invokee (default = 0)
		 */
		invokee()
			: mName{ "invokee #" + std::to_string(sGeneratedNameId++) }
			, mWasEnabledLastFrame{ false }
			, mEnabled{ true }
			, mRenderEnabled{ true }
		{ }

		/**	@brief Constructor
		 *	@param aName Name by which this object can be identified
		 *	@param aIsEnabled Enabled means that this object will receive the method invocations during the loop
		 *  @param aExecutionOrder sets the desired execution order of this invokee (default = 0)
		 */
		invokee(std::string aName, bool aIsEnabled = true) 
			: mName{ std::move(aName) }
			, mWasEnabledLastFrame{ false }
			, mEnabled{ aIsEnabled }
			, mRenderEnabled{ true }
		{ }

		virtual ~invokee()
		{
			// Make sure, this element gets removed from the composition
			auto* cc = composition_interface::current();
			if (nullptr != cc) {
				cc->remove_element_immediately(*this, true);
			}
		}

		/** Returns the name of this invokee */
		const std::string& name() const { return mName; }

		/** Sets the name of this invokee */
		void set_name(std::string aName) { mName = std::move(aName); }

		/** Returns the desired execution order of this invokee w.r.t. the default time 0.
		 *	invokees with negative execution orders will get their initialize-, update-,
		 *	render-, etc. methods invoked earlier; invokees with positive execution orders
		 *	will be invoked later.
		 */
		virtual int execution_order() const { return 0; }

		/**	@brief Initialize this invokee
		 *
		 *	This is the first method in the lifecycle of a invokee,
		 *	that is invoked by the framework during a @ref run. 
		 *	It is called only once.
		 */
		virtual void initialize() {}

		/**	@brief Update this invokee before rendering with fixed or varying delta time
		 *
		 *	This method is called either at fixed or at varying intervals, depending on the timer used. 
		 */
		virtual void update() {}

		/**	@brief Render this invokee 
		 *
		 *	This method is called whenever this invokee should
		 *	perform its rendering tasks. It is called right after
		 *	all the @ref update methods have been invoked.
		 */
		virtual void render() {}

		/**	@brief Cleanup this invokee
		 *
		 *	This is the last method in the lifecycle of a invokee,
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
		 *	Call this method to enable this invokee by the end of 
		 *	the current frame! I.e. this method expresses your intent
		 *	to enable. Perform necessary actions in the on_enable() 
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
			}
		}

		/**	@brief Handle the event of this invokee having been enabled
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
		 *	Call this method to disable this invokee by the end of
		 *	the current frame! I.e. this method expresses your intent
		 *	to disable. Perform necessary actions in the on_disable()
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
			}
		}

		/**	@brief Handle the event of this invokee having been disabled
		 *
		 *	Perform all your "This element has been disabled" actions within
		 *	this event handler method! It will be called regardless of 
		 *	whether the disable request has come from within this invokee
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

		/** @brief Returns whether rendering of this element is enabled or not. */
		bool is_render_enabled() const { return mRenderEnabled; }

		/** @brief applies the changes required by the updater, if one is created for this invokee.	*/
		void apply_recreation_updates()
		{
			if (mUpdater.has_value()) {
				mUpdater->apply();
			}
		}

		/** @brief Returns true if this invokee has an updater */
		bool has_updater()
		{
			return mUpdater.has_value();
		}

		/** @brief Emplaces an updater in mUpdater if there is none, and returns a reference to the newly
		 *         created updater or the already existing one. */
		updater& get_or_create_upadater()
		{
			if (!has_updater()) {
				mUpdater.emplace();
			}
			return mUpdater.value();
		}

	protected:
		/** In case that an updater is required by this invokee, one should be constructed here.
		* The updater - if needed - can be changed over time. However this is the place where
		* the active updater is expected to be.
		*/
		std::optional<updater> mUpdater;

	private:
		inline static int32_t sGeneratedNameId = 0;
		std::string mName;
		bool mWasEnabledLastFrame;
		bool mEnabled;
		bool mRenderEnabled;
	};
}

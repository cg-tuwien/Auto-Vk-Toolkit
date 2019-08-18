#pragma once

namespace cgb
{
	// Forward-declare cg_element
	class cg_element;

	/**	Interface to access the current composition from e.g. inside
	 *	a @ref cg_element.
	 */
	class composition_interface
	{
		friend class generic_glfw;
	public:
		/** @brief Get the currently active composition_interface
		 *
		 *	By design, there can only be one active composition_interface at at time.
		 *	You can think of composition_interface being something like scenes, of 
		 *	which typically one can be active at any given point in time.
		 */
		static composition_interface* current() { return sCurrentComposition; }

		/** Interface to the current game time */
		virtual timer_interface& time() = 0;

		/** Access to the current frame's input */
		virtual input_buffer& input() = 0;

		/** @brief Get the @ref cg_element at the given index
		 *
		 *	Get the @ref cg_element in this composition_interface's objects-container at 
		 *	the given index. If the index is out of bounds, nullptr will be
		 *	returned.
		 */
		virtual cg_element* element_at_index(size_t) = 0;

		/** @brief Find a @ref cg_element by name
		 *
		 *	Find a @ref cg_element assigned to this composition_interface by name.
		 *	If no object with the given name could be found, nullptr is returned.
		 */
		virtual cg_element* element_by_name(std::string_view) = 0;

		/** @brief Find an object by type 
		 *	@param pType type-identifier of the @ref cg_element to be searched for
		 *	@param pIndex Get the n-th object of the specified type
		 */
		virtual cg_element* element_by_type(const std::type_info& pType, uint32_t pIndex = 0) = 0;

		/** @brief	Add an element to this composition which becomes active in the next frame
		 *	This element will be added to the collection of elements at the end of the current frame.
		 *  I.e. the first repeating method call on the element will be a call to @ref cg_element::fixed_update()
		 *  Before the @ref cg_element::fixed_update() call, however, @ref cg_element::initialize() will be 
		 *  called at the beginning of the next frame.
		 *  @param	pElement	Reference to the element to add to the composition
		 */
		virtual void add_element(cg_element& pElement) = 0;

		/** @brief	Adds an element immediately to the composition
		 *	This method performs the same as @ref add_element() but does not wait until the end of 
		 *  the current frame. Also @ref cg_element::intialize() will be called immediately on the
		 *  element.
		 *  @param	pElement	Reference to the element to add to the composition
		 */
		virtual void add_element_immediately(cg_element& pElement) = 0;

		/**	@brief	Removes an element from the composition at the end of the current frame
		 *	Removes the given element from the composition at the end of the current frame.
		 *	This means that all current frame's repeading method calls up until @ref cg_element::render_gui()
		 *  will be called on the element. After that, @ref cg_element::finalize() will be called and the
		 *  element will be removed from the collection.
		 *  @param	pElement	Reference to the element to be removed from the composition
		 */
		virtual void remove_element(cg_element& pElement) = 0;

		/**	@brief	Removes an element immediately from the composition
		 *	This method performs the same as @ref remove_element() but does not wait until the end of
		 *  the current frame. Also @ref cg_element::finalize() will be called immediately on the element
		 *  (except if the pIsBeingDestructed parameter is set to true).
		 *  @param	pElement			Reference to the element to be removed from the composition
		 *	@param	pIsBeingDestructed	Set to true to indicate this call is being issued from the 
		 *								@ref cg_element's destructor; in that case, @ref cg_element::finalize()
		 *								will not be invoked on the given element.
		 */
		virtual void remove_element_immediately(cg_element& pElement, bool pIsBeingDestructed = false) = 0;

		/** @brief Start a game/rendering-loop for this composition_interface
		 *
		 *	Attention: In subclasses of @ref composition_interface, do not forget to call
		 *	@ref set_current no later than at the very beginning of this methods's
		 *	implementation!
		 */
		virtual void start() = 0;

		/** Stop a currently running game/rendering-loop for this composition_interface */
		virtual void stop() = 0;

		/** True if this composition_interface has been started but not yet stopped or finished. */
		virtual bool is_running() = 0;

	protected:
		/** @brief Set a new current composition_interface 
		 *
		 *	Set a new composition_interface. Remember: There can only be one
		 *	at any given point in time. This call will fail if a different
		 *	composition_interface is currently set with @ref is_running() evaluating
		 *	to true.
		 */
		static void set_current(composition_interface* pNewComposition)
		{
			if (nullptr != sCurrentComposition && sCurrentComposition->is_running())
			{
				throw std::runtime_error("There is already an active composition_interface which is still running.");
			}
			// It's okay.
			sCurrentComposition = pNewComposition;
		}

		/** Hidden access to the background input buffer, accessible to friends (namely @ref generic_glfw) */
		virtual input_buffer& background_input_buffer() = 0;

	private:
		/** The (single) currently active composition_interface */
		static composition_interface* sCurrentComposition;
	};
}

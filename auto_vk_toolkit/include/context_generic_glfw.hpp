#pragma once

#include "context_state.hpp"
#include "context_generic_glfw_types.hpp"

class cursor;

namespace avk
{
	// ============================= forward declarations ===========================
	class input_buffer;
	enum struct key_code;

	// =============================== type aliases =================================
	using dispatcher_action = void(void);

	/** Context event handler types are of type bool()
	*	The return value of an event handler means "handled?" or "done?".
	*	I.e.: return true if the event handler is done with its work and can be removed.
	*        Return false to indicate that the event handler is not done and shall remain
	*        in the list of event handlers. I.e. it will be invoked again.
	*  Example: For a one-time handler, just return true and have it removed after its first invocation.
	*	Example: For a recurring handler, return false as long as it shall remain in the list of event handlers.
	*/
	using event_handler_func = std::function<bool()>;

	// =========================== GLFW (PARTIAL) CONTEXT ===========================
	/** @brief Provides generic GLFW-specific functionality
	 */
	class context_generic_glfw
	{
	public:
		/** Initializes GLFW */
		context_generic_glfw();
		
		/** Cleans up GLFW stuff */
		virtual ~context_generic_glfw();
		
		/** Evaluates to true if GLFW initialization succeeded  */
		operator bool() const;
		
		/** Prepares a new window */
		window* prepare_window();

		/** Close the given window, cleanup the resources */
		void close_window(window_base& wnd);

		/** Gets the current system time */
		double get_time();

		/** @brief starts receiving mouse and keyboard input from specified window.
		 *
		 *	@param[in] _Window The window to receive input from
		 *	@param[ref] _InputBuffer The input buffer to be filled with user input
		 */
		void start_receiving_input_from_window(const window& _Window, input_buffer& _InputBuffer);

		/**	@brief stops receiving mouse and keyboard input from specified window.
		 *
		 *	@param[in] _Window The window to stop receiving input from
		 */
		void stop_receiving_input_from_window(const window& _Window);

		/** Sets the given window as the new main window.
		 */
		void set_main_window(window* aMainWindowToBe);

		/** Returns the first window which has been created and is still alive or
		 *	the one which has been made the main window via set_main_window()
		 */
		window* main_window() const;

		/** Returns the window which matches the given name, if it is present in the composition.
		 *	@param	aTitle	Title of the window
		 *  @return	Pointer to the window with the given name or nullptr if no window matches
		 */
		window* window_by_title(std::string_view aTitle) const;

		/** Returns the window which matches the given id, if it is present in the composition.
		 *	@param	pId		Id of the window
		 *  @return	Pointer to the window with the given name or nullptr if no window matches
		 */
		window* window_by_id(uint32_t pId) const;

		/** Select multiple windows and return a vector of pointers to them.
		 *  Example: To select any window, pass the lambda [](avk::window* w){ return true; }
		 */
		template <typename T>
		window* find_window(T selector)
		{
			for (auto& wnd : mWindows) {
				if (selector(&wnd)) {
					return &wnd;
				}
			}
			return nullptr;
		}

		/** Select multiple windows and return a vector of pointers to them.
		 *  Example: To select all windows, pass the lambda [](avk::window* w){ return true; }
		 */
		template <typename T>
		std::vector<window*> find_windows(T selector)
		{
			std::vector<window*> results;
			for (auto& wnd : mWindows) {
				if (selector(&wnd)) {
					results.push_back(&wnd);
				}
			}
			return results;
		}

		/** Finds the window which is associated to the given handle.
		 *	Throws an exception if the handle does not exist in the list of windows!
		 */
		window* window_for_handle(GLFWwindow* handle) const;

		/** Call a function for each window
		 *	@tparam	F	void(window*)
		 */
		template <typename F>
		void execute_for_each_window(F&& fun)
		{
			for (auto& wnd : mWindows) {
				fun(&wnd);
			}
		}

		/** Returns the window which is currently in focus, i.e. this is also
		 *	the window which is affected by all mouse cursor input interaction.
		 */
		window* window_in_focus() const { return sWindowInFocus; }

		/** With this context, all windows share the same graphics-context, this 
		 *	method can be used to get a window to share the context with.
		 */
		GLFWwindow* get_window_for_shared_context();

		/** Returns true if the calling thread is the main thread, false otherwise. */
		static bool are_we_on_the_main_thread();

		/**	Dispatch an action to the main thread and have it executed there.
		 *	@param	pAction	The action to execute on the main thread.
		 */
		void dispatch_to_main_thread(std::function<dispatcher_action> pAction);

		/** Works off all elements in the mDispatchQueue
		 */
		void work_off_all_pending_main_thread_actions();

		/** Add a context event handler function
		*	@param	pHandler		The event handler function which does whatever it does.
		*							Pay attention, however, to its return type: 
		*							 * If the handler returns true, the handler will be removed from the list of event handlers
		*							 * If the handler returns false, in will remain in the event handlers list
		*	@param	pStage			If the handler shall be invoked at a certain context stage only,
		*							specify that stage with this parameter. If it shall be invoked 
		*							regardless of the context's stage, set it to context_state::anytime.
		*/
		void add_event_handler(context_state pStage, event_handler_func pHandler);

		/** Invoke all the event handlers which are assigned to the current context state or to unknown state
		 *	
		 *	@return The number of event handlers that have been removed from the list of event handlers
		 */
		void work_off_event_handlers();

		// Polls GLFW input events if there are any
		void poll_input_events() const { glfwPollEvents(); }
		
		// Waits for GLFW input events and pauses the current (=main) thread
		void wait_for_input_events() const { glfwWaitEvents(); }

		// Posts an event so that the waiting on the other thread is ended and the other thread continues
		void signal_waiting_main_thread() const { glfwPostEmptyEvent(); }

		// Activates a specific type of cursor
		void activate_cursor(window_base* aWindow, cursor aCursorType);

		// Returns the current context state
		auto state() const { return mContextState; }

		template <typename F>
		void add_event_handler(F action, avk::context_state when)
		{
			mEventHandlers.emplace_back(std::move(action), when);
		}
		
	protected:
		static void glfw_error_callback(int error, const char* description);
		static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
		static void glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
		static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
		static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		static void glfw_char_callback(GLFWwindow* window, unsigned int character);
		static void glfw_window_focus_callback(GLFWwindow* window, int focused);
		static void glfw_window_size_callback(GLFWwindow* window, int width, int height);

		std::deque<window> mWindows;
		size_t mMainWindowIndex = 0;
		static window* sWindowInFocus;
		bool mInitialized = false;

		static std::mutex sInputMutex;
		static std::array<key_code, GLFW_KEY_LAST + 1> sGlfwToKeyMapping;

		static std::thread::id sMainThreadId;
		// Protects the mDispatchQueue
		static std::mutex sDispatchMutex;

		std::vector<std::function<dispatcher_action>> mDispatchQueue;

		/** Context event handlers, possible assigned to a certain context_state at 
		*	which they shall be executed. If the avk::context_state is set to unknown,
		*	the associated event handler could possible be executed at any state.
		*
		*	Event handlers are always executed on the main thread.
		*/
		std::deque<std::tuple<event_handler_func, avk::context_state>> mEventHandlers;

		// Which state the context is currently in
		avk::context_state mContextState = context_state::uninitialized;

		GLFWcursor* mArrowCursor = nullptr;
		GLFWcursor* mIbeamCursor = nullptr;
		GLFWcursor* mCrosshairCursor = nullptr;
		GLFWcursor* mHandCursor = nullptr;
		GLFWcursor* mHorizResizeCursor = nullptr;
		GLFWcursor* mVertResizeCursor = nullptr;
	};
}

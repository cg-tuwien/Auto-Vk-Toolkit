#pragma once
#include <exekutor.hpp>

namespace xk
{
	/**	A composition brings together all of the separate components, which there are
	 *	 - A timer
	 *	 - One or more windows
	 *	 - One or more @ref cg_element(-derived objects)
	 *	 
	 *	Upon @ref start, a composition spins up the game-/rendering-loop in which
	 *	all of the @ref cg_element's methods are called.
	 *	
	 *	A composition will internally call @ref set_global_composition_data in order
	 *	to make itself the currently active composition. By design, there can only 
	 *	be one active composition_interface at at time. You can think of a composition
	 *	being something like a scene, of which typically one can be active at any 
	 *	given point in time.
	 *	
	 *	\remark You don't HAVE to use this composition-class, if you are developing 
	 *	an alternative composition class or a different approach and still want to use
	 *	a similar structure as proposed by this composition-class, please make sure 
	 *	to call @ref set_global_composition_data 
	 */
	class composition : public composition_interface
	{
	public:
		composition(timer_interface* aTimer, invoker_interface* aInvoker, std::vector<window*> aWindows, std::vector<cg_element*> aElements)
			: mTimer{ aTimer }
			, mInvoker{ aInvoker }
			, mWindows{ aWindows }
			, mInputBuffers()
			, mInputBufferForegroundIndex(0)
			, mInputBufferBackgroundIndex(1)
			, mShouldStop(false)
			, mInputBufferSwapPending(false)
			, mIsRunning(false)
		{
			for (auto* el : aElements) {
				auto it = std::lower_bound(std::begin(mElements), std::end(mElements), el, [](const cg_element* left, const cg_element* right) { return left->execution_order() < right->execution_order(); });
				mElements.insert(it, el);
			}
		}

		/** Provides access to the timer which is used by this composition */
		timer_interface& time() override
		{
			return *mTimer;
		}

		/** Provides to the currently active input buffer, which contains the
		 *	current user input data */
		input_buffer& input() override
		{
			return mInputBuffers[mInputBufferForegroundIndex];
		}

		input_buffer& background_input_buffer() override
		{
			return mInputBuffers[mInputBufferBackgroundIndex];
		}

		/** Returns the @ref cg_element at the given index */
		cg_element* element_at_index(size_t pIndex) override
		{
			if (pIndex < mElements.size())
				return mElements[pIndex];

			return nullptr;
		}

		/** Finds a @ref cg_element by its name 
		 *	\returns The element found or nullptr
		 */
		cg_element* element_by_name(std::string_view pName) override
		{
			auto found = std::find_if(
				std::begin(mElements), 
				std::end(mElements), 
				[&pName](const cg_element* element)
				{
					return element->name() == pName;
				});

			if (found != mElements.end())
				return *found;

			return nullptr;
		}

		/** Finds the @ref cg_element(s) with matching type.
		 *	@param pType	The type to look for
		 *	@param pIndex	Use this parameter to get the n-th element of the given type
		 *	\returns An element of the given type or nullptr
		 */
		cg_element* element_by_type(const std::type_info& pType, uint32_t pIndex) override
		{
			uint32_t nth = 0;
			for (auto* element : mElements)
			{
				if (typeid(*element) == pType)
				{
					if (pIndex == nth++)
					{
						return element;
					}
				}
			}
			return nullptr;
		}

	private:
		/** Add all elements which are about to be added to the composition */
		void add_pending_elements()
		{
			// Make a copy of all the elements to be added to not interfere with erase-operations:
			std::unique_lock<std::mutex> guard(sCompMutex); // For parallel executors, this is neccessary!
			auto toBeAdded = mElementsToBeAdded;
			guard.unlock();
			for (auto el : toBeAdded) {
				add_element_immediately(*el);
			}
		}

		/** Remove all elements which are about to be removed */
		void remove_pending_elements()
		{
			// Make a copy of all the elements to be added to not interfere with erase-operations:
			std::unique_lock<std::mutex> guard(sCompMutex); // For parallel executors, this is neccessary!
			auto toBeRemoved = mElementsToBeRemoved;
			guard.unlock();
			for (auto el : toBeRemoved) {
				remove_element_immediately(*el);
			}
			assert(mElementsToBeRemoved.size() == 0);
		}

		/** Signal the main thread to start swapping input buffers */
		static void please_swap_input_buffers(composition* thiz)
		{
			{
				std::scoped_lock<std::mutex> guard(sCompMutex);
				assert(false == thiz->mInputBufferSwapPending);
				thiz->mInputBufferSwapPending = true;
			}
			context().signal_waiting_main_thread();
		}

		/** Wait on the rendering thread until the main thread has swapped the input buffers */
		static void wait_for_input_buffers_swapped(composition* thiz)
		{
			using namespace std::chrono_literals;
			
			std::unique_lock<std::mutex> lk(sCompMutex);
			if (thiz->mInputBufferSwapPending) {
#if defined(_DEBUG)
				auto totalWaitTime = 0ms;
				auto timeout = 1ms;
				int cnt = 0;
				while (!sInputBufferCondVar.wait_for(lk, timeout, [thiz]{ return !thiz->mInputBufferSwapPending; })) {
					cnt++;
					totalWaitTime += timeout;
					LOG_DEBUG(fmt::format("Condition variable waits for timed out. Total wait time in current frame is {}", totalWaitTime));	
				}
#else
				sInputBufferCondVar.wait(lk, [thiz]{ return !thiz->mInputBufferSwapPending; });
#endif
			}
			assert(thiz->mInputBufferSwapPending == false);
//#if _DEBUG
//				if ((i+1) % 10000 == 0)
//				{
//					LOG_DEBUG(fmt::format("Warning: more than {} iterations in spin-lock", i+1));
//				}
//#endif
		}

		/** Rendering thread's main function */
		static void render_thread(composition* thiz)
		{
			// Used to distinguish between "simulation" and "render"-frames
			auto frameType = timer_frame_type::none;

			while (!thiz->mShouldStop)
			{
				thiz->add_pending_elements();

				// signal context
				context().begin_frame();
				context().signal_waiting_main_thread(); // Let the main thread do some work in the meantime

				frameType = thiz->mTimer->tick();

				wait_for_input_buffers_swapped(thiz);

				// 2. check and possibly issue on_enable event handlers
				thiz->mInvoker->execute_handle_enablings(thiz->mElements);

				// 3. fixed_update
				if ((frameType & timer_frame_type::fixed) == timer_frame_type::fixed)
				{
					thiz->mInvoker->execute_fixed_updates(thiz->mElements);
				}

				if ((frameType & timer_frame_type::varying) == timer_frame_type::varying)
				{
					// 4. update
					thiz->mInvoker->execute_updates(thiz->mElements);

					// signal context
					context().update_stage_done();
					context().signal_waiting_main_thread(); // Let the main thread work concurrently

					// Tell the main thread that we'd like to have the new input buffers from A) here:
					please_swap_input_buffers(thiz);

					// Sync (wait for fences and so) per window BEFORE executing render callbacks
					xk::context().execute_for_each_window([](window* wnd){
						wnd->sync_before_render();
					});

					// 5. render
					thiz->mInvoker->execute_renders(thiz->mElements);

					// 6. render_gizmos
					thiz->mInvoker->execute_render_gizmos(thiz->mElements);
					
					// Render per window
					xk::context().execute_for_each_window([](window* wnd){
						wnd->render_frame();
					});
				}
				else
				{
					// signal context
					context().update_stage_done();
					context().signal_waiting_main_thread(); // Let the main thread work concurrently

					// If not performed from inside the positive if-branch, tell the main thread of our 
					// input buffer update desire here:
					please_swap_input_buffers(thiz);
				}

				// 8. check and possibly issue on_disable event handlers
				thiz->mInvoker->execute_handle_disablings(thiz->mElements);

				// signal context
				context().end_frame();
				context().signal_waiting_main_thread(); // Let the main thread work concurrently

				thiz->remove_pending_elements();
			}

		}

	public:
		void add_element(cg_element& pElement) override
		{
			std::scoped_lock<std::mutex> guard(sCompMutex); // For parallel executors, this is neccessary!
			mElementsToBeAdded.push_back(&pElement);
		}

		void add_element_immediately(cg_element& pElement) override
		{
			std::scoped_lock<std::mutex> guard(sCompMutex); // For parallel executors, this is neccessary!
			// Find right place to insert:
			auto it = std::lower_bound(std::begin(mElements), std::end(mElements), &pElement, [](const cg_element* left, const cg_element* right) { return left->execution_order() < right->execution_order(); });
			mElements.insert(it, &pElement);
			// 1. initialize
			pElement.initialize();
			// Remove from mElementsToBeAdded container (if it was contained in it)
			mElementsToBeAdded.erase(std::remove(std::begin(mElementsToBeAdded), std::end(mElementsToBeAdded), &pElement), std::end(mElementsToBeAdded));
		}

		void remove_element(cg_element& pElement) override
		{
			std::scoped_lock<std::mutex> guard(sCompMutex); // For parallel executors, this is neccessary!
			mElementsToBeRemoved.push_back(&pElement);
		}

		void remove_element_immediately(cg_element& pElement, bool pIsBeingDestructed = false) override
		{
			std::scoped_lock<std::mutex> guard(sCompMutex); // For parallel executors, this is neccessary!
			if (!pIsBeingDestructed) {
				assert(std::find(std::begin(mElements), std::end(mElements), &pElement) != mElements.end());
				// 9. finalize
				pElement.finalize();
				// Remove from the actual elements-container
				mElements.erase(std::remove(std::begin(mElements), std::end(mElements), &pElement), std::end(mElements));
				// ...and from mElementsToBeRemoved
				mElementsToBeRemoved.erase(std::remove(std::begin(mElementsToBeRemoved), std::end(mElementsToBeRemoved), &pElement), std::end(mElementsToBeRemoved));
			}
			else {
				LOG_DEBUG(fmt::format("Removing element with name[{}] and address[{}] issued from cg_element's destructor",
										 pElement.name(),
										 fmt::ptr(&pElement)));
			}
		}

		void start() override
		{
			context().work_off_event_handlers();

			// Make myself the current composition_interface
			composition_interface::set_current(this);

			// 1. initialize
			for (auto& o : mElements)
			{
				o->initialize();
			}

			// Signal context after initialization
			context().begin_composition();
			context().work_off_event_handlers();

			// Enable receiving input
			auto windows_for_input = context().find_windows([](auto * w) { return w->is_input_enabled(); });
			for (auto* w : windows_for_input)
			{
				w->set_is_in_use(true);
				// Write into the buffer at mInputBufferUpdateIndex,
				// let client-objects read from the buffer at mInputBufferConsumerIndex
				context().start_receiving_input_from_window(*w, mInputBuffers[mInputBufferForegroundIndex]);
				mWindows.push_back(w);
			}

			// game-/render-loop:
			mIsRunning = true;

			// off it goes
			std::thread renderThread(render_thread, this);
			
			while (!mShouldStop)
			{
				context().work_off_all_pending_main_thread_actions();
				context().work_off_event_handlers();

				std::unique_lock<std::mutex> lk(sCompMutex);
				if (mInputBufferSwapPending) {
					auto* windowForCursorActions = context().window_in_focus();
					// The buffer which has been updated becomes the buffer which will be consumed in the next frame
					//  update-buffer = previous frame, i.e. done
					//  consumer-index = updated in the next frame, i.e. to be done
					input_buffer::prepare_for_next_frame(
						mInputBuffers[mInputBufferBackgroundIndex], 
						mInputBuffers[mInputBufferForegroundIndex], 
						windowForCursorActions);
					std::swap(mInputBufferForegroundIndex, mInputBufferBackgroundIndex);

					// reset flag:
					mInputBufferSwapPending = false;

					int width = 0, height = 0;
				    glfwGetFramebufferSize(xk::context().main_window()->handle()->mHandle, &width, &height);
				    while (width == 0 || height == 0) {
				        glfwGetFramebufferSize(xk::context().main_window()->handle()->mHandle, &width, &height);
				        glfwWaitEvents();
				    }
					
					// resume render_thread:
					lk.unlock();
					sInputBufferCondVar.notify_one();
				}
				else {
					lk.unlock();
				}

				context().wait_for_input_events();
			}

			renderThread.join();


			mIsRunning = false;

			// Stop the input
			for (auto* w : mWindows)
			{
				context().stop_receiving_input_from_window(*w);
				w->set_is_in_use(false);
			}
			mWindows.clear();

			// Signal context before finalization
			context().end_composition();

			// 9. finalize
			for (auto& o : mElements)
			{
				o->finalize();
			}
			
			// Make myself the current composition_interface
			composition_interface::set_current(nullptr);
		}

		/** Stop a currently running game/rendering-loop for this composition_interface */
		void stop() override
		{
			mShouldStop = true;
		}

		/** True if this composition_interface has been started but not yet stopped or finished. */
		bool is_running() override
		{
			return mIsRunning;
		}

	private:
		static std::mutex sCompMutex;
		std::atomic_bool mShouldStop;
		bool mInputBufferSwapPending;
		static std::condition_variable sInputBufferCondVar;

		bool mIsRunning;

		timer_interface* mTimer;
		invoker_interface* mInvoker;
		std::vector<window*> mWindows;
		std::vector<cg_element*> mElements;
		std::vector<cg_element*> mElementsToBeAdded;
		std::vector<cg_element*> mElementsToBeRemoved;

		std::array<input_buffer, 2> mInputBuffers;
		int32_t mInputBufferForegroundIndex;
		int32_t mInputBufferBackgroundIndex;
	};

	std::mutex composition::sCompMutex{};

	std::condition_variable composition::sInputBufferCondVar{};

}


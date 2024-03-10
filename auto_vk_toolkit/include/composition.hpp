#pragma once

#include "invokee.hpp"
#include "composition_interface.hpp"
#include "context_vulkan.hpp"
#include "timer_interface.hpp"

#define SINGLE_THREADED 1

namespace avk
{
	/**	A composition brings together all of the separate components, which there are
	 *	 - A timer
	 *	 - One or more windows
	 *	 - One or more @ref invokee(-derived objects)
	 *	 
	 *	Upon @ref start, a composition spins up the game-/rendering-loop in which
	 *	all of the @ref invokee's methods are called.
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
		composition(std::vector<window*> aWindows, std::vector<invokee*> aElements)
			: mWindows{ aWindows }
			, mInputBuffers()
			, mInputBufferForegroundIndex(0)
			, mInputBufferBackgroundIndex(1)
			, mShouldStop(false)
			, mInputBufferSwapPending(false)
			, mIsRunning(false)
		{
			for (auto* el : aElements) {
				auto it = std::lower_bound(std::begin(mElements), std::end(mElements), el, [](const invokee* left, const invokee* right) { return left->execution_order() < right->execution_order(); });
				mElements.insert(it, el);
			}
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

		/** Returns the @ref invokee at the given index */
		invokee* element_at_index(size_t pIndex) override
		{
			if (pIndex < mElements.size())
				return mElements[pIndex];

			return nullptr;
		}

		/** Finds a @ref invokee by its name 
		 *	\returns The element found or nullptr
		 */
		invokee* element_by_name(std::string_view pName) override
		{
			auto found = std::find_if(
				std::begin(mElements), 
				std::end(mElements), 
				[&pName](const invokee* element)
				{
					return element->name() == pName;
				});

			if (found != mElements.end())
				return *found;

			return nullptr;
		}

		/** Finds the @ref invokee(s) with matching type.
		 *	@param pType	The type to look for
		 *	@param pIndex	Use this parameter to get the n-th element of the given type
		 *	\returns An element of the given type or nullptr
		 */
		invokee* element_by_type(const std::type_info& pType, uint32_t pIndex) override
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
			std::unique_lock<std::mutex> guard(sCompMutex); // For parallel invokers, this is neccessary!
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
			std::unique_lock<std::mutex> guard(sCompMutex); // For parallel invokers, this is neccessary!
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
#if !SINGLE_THREADED
			{
				std::scoped_lock<std::mutex> guard(sCompMutex);
				assert(false == thiz->mInputBufferSwapPending);
				thiz->mInputBufferSwapPending = true;
			}
			context().signal_waiting_main_thread();
#else
			thiz->mInputBufferSwapPending = true;
#endif
		}

		static void signal_input_buffers_have_been_swapped()
		{
#if !SINGLE_THREADED
			sInputBufferCondVar.notify_one();
#endif
		}

		/** Wait on the rendering thread until the main thread has swapped the input buffers */
		static void wait_for_input_buffers_swapped(composition* thiz)
		{
#if !SINGLE_THREADED
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
					LOG_DEBUG(std::format("Condition variable waits for timed out. Total wait time in current frame is {}", totalWaitTime));	
				}
#else
				sInputBufferCondVar.wait(lk, [thiz]{ return !thiz->mInputBufferSwapPending; });
#endif
			}
			assert(thiz->mInputBufferSwapPending == false);
//#ifdef _DEBUG
//				if ((i+1) % 10000 == 0)
//				{
//					LOG_DEBUG(std::format("Warning: more than {} iterations in spin-lock", i+1));
//				}
//#endif
#endif
		}

		static void awake_main_thread()
		{
#if !SINGLE_THREADED
			context().signal_waiting_main_thread(); // Wake up the main thread which is possibly waiting for events and let it do some work
#endif
		}

		/** Rendering thread's main function */
		template <typename UC, typename RC>
		static void render_thread(composition* thiz, UC aUpdateCallback, RC aRenderCallback)
		{
			// Used to distinguish between "simulation" and "render"-frames
			auto frameType = timer_frame_type::none;

#if !SINGLE_THREADED
			while (!thiz->mShouldStop)
			{
#endif
				thiz->add_pending_elements();

				// signal context
				context().begin_frame();
				awake_main_thread(); // Let the main thread do some work in the meantime

				frameType = time().tick();

				wait_for_input_buffers_swapped(thiz);

				// 2. check and possibly issue on_enable event handlers
				for (auto& e : thiz->mElements) {
					e->handle_enabling();
				}

				// 3. update
				if ((frameType & timer_frame_type::update) == timer_frame_type::update)
				{
					aUpdateCallback(static_cast<const std::vector<invokee*>&>(thiz->mElements));

					// signal context:
					context().update_stage_done();
					awake_main_thread(); // Let the main thread work concurrently
				}

				if ((frameType & timer_frame_type::render) == timer_frame_type::render)
				{
					// Tell the main thread that we'd like to have the new input buffers from A) here:
					please_swap_input_buffers(thiz);

					// 4. render:
					aRenderCallback(static_cast<const std::vector<invokee*>&>(thiz->mElements));
				}
				else
				{
					// If not performed from inside the positive if-branch, tell the main thread of our 
					// input buffer update desire here:
					please_swap_input_buffers(thiz);
				}

				// 5. check and possibly issue on_disable event handlers
				for (auto& e : thiz->mElements) {
					e->handle_disabling();
				}

				// signal context
				context().end_frame();
				awake_main_thread(); // Let the main thread work concurrently

				thiz->remove_pending_elements();
#if !SINGLE_THREADED
			}
#endif
		}

	public:
		void add_element(invokee& pElement) override
		{
			std::scoped_lock<std::mutex> guard(sCompMutex); // For parallel invokers, this is neccessary!
			mElementsToBeAdded.push_back(&pElement);
		}

		void add_element_immediately(invokee& pElement) override
		{
			std::scoped_lock<std::mutex> guard(sCompMutex); // For parallel invokers, this is neccessary!
			// Find right place to insert:
			auto it = std::lower_bound(std::begin(mElements), std::end(mElements), &pElement, [](const invokee* left, const invokee* right) { return left->execution_order() < right->execution_order(); });
			mElements.insert(it, &pElement);
			// 1. initialize
			pElement.initialize();
			// Remove from mElementsToBeAdded container (if it was contained in it)
			mElementsToBeAdded.erase(std::remove(std::begin(mElementsToBeAdded), std::end(mElementsToBeAdded), &pElement), std::end(mElementsToBeAdded));
		}

		void remove_element(invokee& pElement) override
		{
			std::scoped_lock<std::mutex> guard(sCompMutex); // For parallel invokers, this is neccessary!
			mElementsToBeRemoved.push_back(&pElement);
		}

		void remove_element_immediately(invokee& pElement, bool pIsBeingDestructed = false) override
		{
			std::scoped_lock<std::mutex> guard(sCompMutex); // For parallel invokers, this is neccessary!
			if (!pIsBeingDestructed) {
				assert(std::find(std::begin(mElements), std::end(mElements), &pElement) != mElements.end());
				// 6. finalize
				pElement.finalize();
				// Remove from the actual elements-container
				mElements.erase(std::remove(std::begin(mElements), std::end(mElements), &pElement), std::end(mElements));
				// ...and from mElementsToBeRemoved
				mElementsToBeRemoved.erase(std::remove(std::begin(mElementsToBeRemoved), std::end(mElementsToBeRemoved), &pElement), std::end(mElementsToBeRemoved));
			}
			else {
				LOG_DEBUG(std::format("Removing element with name[{}] and address[{}] issued from invokee's destructor",
										 pElement.name(),
										 reinterpret_cast<intptr_t>(&pElement)));
			}
		}

		/**	Start the render loop for the (previously configured) composition. 
		 *	The loop will run as long as its internal mShouldStop is false, which can be changed via composition_interface::stop.
		 *	You must pass two callback functions as parameters:
		 *	@param	aUpdateCallback		A function which must invoke all the passed invokee's update() methods if they are enabled.
		 *                              Hint: You could use avk::sequential_invoker for this task.
		 *                              Required signature of the callback: void(const std::vector<invokee*>&)
		 *	@param	aRenderCallback		A function which must invoke all the passed invokee's render() methods if they are enabled.
		 *                              Hint: You could use avk::sequential_invoker for this task.
		 *                              Furthermore, if windows are used, window::sync_before_render must be invoked for every active
		 *                              window before the render() invokations, and window::render_frame must be invoked afterwards.
		 *                              Required signature of the callback: void(const std::vector<invokee*>&)
		 */
		template <typename UC, typename RC>
		void start_render_loop(UC aUpdateCallback, RC aRenderCallback)
		{
			context().work_off_event_handlers();

			// Make myself the current composition_interface
			composition_interface::set_current(this);

			// 1. initialize
			for (auto& o : mElements) {
				o->initialize();
			}

			// Signal context after initialization
			context().begin_composition();
			context().work_off_event_handlers();

			// Enable receiving input
			auto windows_for_input = context().find_windows([](auto * w) { return w->is_input_enabled(); });
			for (auto* w : windows_for_input) {
				w->set_is_in_use(true);
				// Write into the buffer at mInputBufferUpdateIndex,
				// let client-objects read from the buffer at mInputBufferConsumerIndex
				context().start_receiving_input_from_window(*w, mInputBuffers[mInputBufferForegroundIndex]);
				mWindows.push_back(w);
			}

			// game-/render-loop:
			mIsRunning = true;

#if !SINGLE_THREADED
			// off it goes
			std::thread renderThread(
				render_thread<decltype(aUpdateCallback), decltype(aRenderCallback)>,
				this, std::move(aUpdateCallback), std::move(aRenderCallback));
#endif
			
			while (!mShouldStop)
			{
				context().work_off_all_pending_main_thread_actions();
				context().work_off_event_handlers();

#if !SINGLE_THREADED
				std::unique_lock<std::mutex> lk(sCompMutex);
#else
				render_thread(this, std::move(aUpdateCallback), std::move(aRenderCallback));
#endif
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

					// TODO: This is a bit (read: a lot) stupid, that this is tied to the main_window => support multiple windows here gracefully:
					int width = 0, height = 0;
				    glfwGetFramebufferSize(context().main_window()->handle()->mHandle, &width, &height);
				    while (width == 0 || height == 0) {
				        glfwGetFramebufferSize(context().main_window()->handle()->mHandle, &width, &height);
				        glfwWaitEvents();
				    }
					context().main_window()->update_resolution();
					
#if !SINGLE_THREADED
					// resume render_thread:
					lk.unlock();
#endif
					signal_input_buffers_have_been_swapped();
				}
				else {
#if !SINGLE_THREADED
					lk.unlock();
#endif
				}

#if !SINGLE_THREADED
				context().wait_for_input_events();
#else
				context().poll_input_events();
#endif
			}

#if !SINGLE_THREADED
			renderThread.join();
#endif

			mIsRunning = false;

			// Stop the input
			for (auto* w : mWindows) {
				context().stop_receiving_input_from_window(*w);
				w->set_is_in_use(false);
			}

			// Signal context before finalization
			context().end_composition(); // Performs a waitIdle

			for (auto* w : mWindows) {
				w->clean_up_resources_for_frame(std::numeric_limits<window::frame_id_t>().max());
				w->remove_all_present_semaphore_dependencies_for_frame(std::numeric_limits<window::frame_id_t>().max());
			}
			mWindows.clear();
			
			// 9. finalize in reverse order
			const auto elementsReverseView = std::ranges::reverse_view{ mElements };
			for (const auto& o : elementsReverseView) {
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

		std::vector<window*> mWindows;
		std::vector<invokee*> mElements;
		std::vector<invokee*> mElementsToBeAdded;
		std::vector<invokee*> mElementsToBeRemoved;

		std::array<input_buffer, 2> mInputBuffers;
		int32_t mInputBufferForegroundIndex;
		int32_t mInputBufferBackgroundIndex;
	};

}


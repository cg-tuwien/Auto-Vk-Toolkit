#pragma once

namespace cgb // ========================== TODO/WIP =================================
{
	class renderpass_t;
	class command_pool;
	class window;
	class image_t;
	class graphics_pipeline_t;
	class compute_pipeline_t;
	class ray_tracing_pipeline_t;
	class set_of_descriptor_set_layouts;
	struct binding_data;

	enum struct command_buffer_state
	{
		none,
		recording,
		finished_recording,
		submitted
	};

	/** A command buffer which has been created for a certain queue family */
	class command_buffer_t
	{
		friend class device_queue;
		
	public:
		command_buffer_t() = default;
		command_buffer_t(command_buffer_t&&) noexcept = default;
		command_buffer_t(const command_buffer_t&) = delete;
		command_buffer_t& operator=(command_buffer_t&&) noexcept = default;
		command_buffer_t& operator=(const command_buffer_t&) = delete;
		~command_buffer_t();

		/** Set a custom deleter function.
		 *	This is often used for resource cleanup, e.g. a buffer which can be deleted when this command buffer is destroyed.
		 */
		template <typename F>
		command_buffer_t& set_custom_deleter(F&& aDeleter) noexcept
		{
			if (mCustomDeleter.has_value()) {
				// There is already a custom deleter! Make sure that this stays alive as well.
				mCustomDeleter = [
					existingDeleter = std::move(mCustomDeleter.value()),
					additionalDeleter = std::forward<F>(aDeleter)
				]() {
					additionalDeleter();
					existingDeleter();
				};
			}
			else {
				mCustomDeleter = std::forward<F>(aDeleter);
			}
			return *this;
		}

		void begin_recording();
		void end_recording();

		/**	Begins a new render pass for the given window, i.e. calls Vulkan's vkBeginRenderPass.
		 *	Also clears all the attachments.
		 *	Pay attention to the parameter `_ConcurrentFrameIndex` as it will refer to one of the (concurrent) back buffers!
		 *	
		 *	@param	aWindow					The window which to begin the render pass for.
		 *	@param	aInFlightIndex			The "in flight index" referring to a specific index of the back buffers.
		 *									If left unset, it will be set to the current frame's "in flight index",
		 *									which is basically `cf % iff`, where `cf` is the current frame's id (or 
		 *									`_Window->current_frame()`), and `iff` is the number of concurrent frames,
		 *									in flight (or `_Window->number_of_in_flight_frames()`).
		 */
		void begin_render_pass_for_framebuffer(window* aWindow, std::optional<int64_t> aInFlightIndex = {});
		void begin_render_pass_for_framebuffer(const cgb::renderpass_t& aRenderPass, window* aWindow, std::optional<int64_t> aInFlightIndex = {});
		void begin_render_pass_for_framebuffer(const vk::RenderPass& aRenderPass, const vk::Framebuffer& aFramebuffer, const vk::Offset2D& aOffset, const vk::Extent2D& aExtent, std::vector<vk::ClearValue> aClearValues);
		void establish_execution_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage);
		void establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible);
		void establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible);
		void establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible);
		void establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible);
		void copy_image(const image_t& aSource, const vk::Image& aDestination);
		void end_render_pass();

		auto& begin_info() const { return mBeginInfo; }
		auto& handle() const { return mCommandBuffer.get(); }
		auto* handle_addr() const { return &mCommandBuffer.get(); }
		auto state() const { return mState; }

		// Template specializations are implemented in the respective pipeline's header files
		template <typename T> // Expected to be just the pipeline's type
		void bind_pipeline(const T& aPipeline)
		{
			assert(false);
			throw std::logic_error("No suitable bind_pipeline overload found for the given argument.");
		}

		void bind_descriptors(vk::PipelineBindPoint aBindingPoint, vk::PipelineLayout aLayoutHandle, std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aDescriptorCache = nullptr);

		// Template specializations are implemented in the respective pipeline's header files
		template <typename T> // Expected to be a tuple of type: std::tuple<const PIPE*, const set_of_descriptor_set_layouts*>
		void bind_descriptors(T aPipelineLayoutTuple, std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aDescriptorCache = nullptr)
		{
			// TODO: In the current state, we're relying on COMPATIBLE layouts. Think about reusing the pipeline's allocated and internally stored layouts!
			assert(false);
			throw std::logic_error("No suitable bind_descriptors overload found for the given pipeline/layout.");
		}
		
		static std::vector<owning_resource<command_buffer_t>> create_many(uint32_t aCount, command_pool& aPool, vk::CommandBufferUsageFlags aUsageFlags);
		static owning_resource<command_buffer_t> create(command_pool& aPool, vk::CommandBufferUsageFlags aUsageFlags);
		
	private:
		command_buffer_state mState;
		vk::CommandBufferBeginInfo mBeginInfo;
		vk::UniqueCommandBuffer mCommandBuffer;
		
		/** A custom deleter function called upon destruction of this command buffer */
		std::optional<cgb::unique_function<void()>> mCustomDeleter;
	};

	// Typedef for a variable representing an owner of a command_buffer
	using command_buffer = owning_resource<command_buffer_t>;
	
}

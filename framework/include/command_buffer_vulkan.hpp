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
	class framebuffer_t;
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
		void begin_render_pass_for_framebuffer(const renderpass_t& aRenderpass, const framebuffer_t& aFramebuffer, glm::ivec2 aRenderAreaOffset = {0, 0}, std::optional<glm::uvec2> aRenderAreaExtent = {}, bool aSubpassesInline = true);
		void establish_execution_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage);
		void establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible);
		void establish_global_memory_barrier(pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible);
		void establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<memory_access> aSrcAccessToBeMadeAvailable, std::optional<memory_access> aDstAccessToBeMadeVisible);
		void establish_image_memory_barrier(image_t& aImage, pipeline_stage aSrcStage, pipeline_stage aDstStage, std::optional<write_memory_access> aSrcAccessToBeMadeAvailable, std::optional<read_memory_access> aDstAccessToBeMadeVisible);
		void copy_image(const image_t& aSource, const vk::Image& aDestination);
		void end_render_pass();

		template <typename Bfr, typename... Bfrs>
		void draw_vertices(const Bfr aVertexBuffer, const Bfrs&... aFurtherBuffers, uint32_t aNumberOfInstances = 1u, uint32_t aFirstVertex = 0u, uint32_t aFirstInstance = 0u)
		{
			handle().bindVertexBuffers(0u, { aVertexBuffer.buffer_handle(), aFurtherBuffers.buffer_handle() ... }, { vk::DeviceSize{0}, ((void)aFurtherBuffers, vk::DeviceSize{0}) ... });
			//																									Make use of the discarding behavior of the comma operator ^, see: https://stackoverflow.com/a/61098748/387023
			handle().draw(aVertexBuffer.mVertexCount, aNumberOfInstances, aFirstVertex, aFirstInstance);                      
		}

		template <typename IdxBfr, typename... Bfrs>
		void draw_indexed(const IdxBfr& aIndexBuffer, uint32_t aNumberOfInstances, uint32_t aFirstIndex, uint32_t aVertexOffset, uint32_t aFirstInstance, const Bfrs&... aVertexBuffers)
		{
			handle().bindVertexBuffers(0u, { aVertexBuffers.buffer_handle() ... }, { ((void)aVertexBuffers, vk::DeviceSize{0}) ... });
			//											Make use of the discarding behavior of the comma operator ^, see: https://stackoverflow.com/a/61098748/387023

			// TODO: I have no idea why cgb::to_vk_index_type can not be found during compilation time
			vk::IndexType indexType = vk::IndexType::eNoneNV;
			switch (aIndexBuffer.meta_data().sizeof_one_element()) {
				case sizeof(uint16_t): indexType = vk::IndexType::eUint16; break;
				case sizeof(uint32_t): indexType = vk::IndexType::eUint32; break;
				default: LOG_ERROR(fmt::format("The given size[{}] does not correspond to a valid vk::IndexType", aIndexBuffer.meta_data().sizeof_one_element())); break;
			}
			
			handle().bindIndexBuffer(aIndexBuffer.buffer_handle(), 0u, indexType);
			handle().drawIndexed(aIndexBuffer.meta_data().num_elements(), aNumberOfInstances, aFirstIndex, aVertexOffset, aFirstInstance);
		}
		
		template <typename IdxBfr, typename... Bfrs>
		void draw_indexed(const IdxBfr& aIndexBuffer, const Bfrs&... aVertexBuffers)
		{
			draw_indexed(aIndexBuffer, 1u, 0u, 0u, 0u, aVertexBuffers ...);
		}
		
		auto& begin_info() const { return mBeginInfo; }
		auto& handle() const { return mCommandBuffer.get(); }
		auto* handle_addr() const { return &mCommandBuffer.get(); }
		auto state() const { return mState; }

		// Template specializations are implemented in the respective pipeline's header files
		template <typename T> // Expected to be just the pipeline's type
		void bind_pipeline(const T& aPipeline)
		{
			assert(false);
			throw cgb::logic_error("No suitable bind_pipeline overload found for the given argument.");
		}

		void bind_descriptors(vk::PipelineBindPoint aBindingPoint, vk::PipelineLayout aLayoutHandle, std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aDescriptorCache = nullptr);

		// Template specializations are implemented in the respective pipeline's header files
		template <typename T> // Expected to be a tuple of type: std::tuple<const PIPE*, const set_of_descriptor_set_layouts*>
		void bind_descriptors(T aPipelineLayoutTuple, std::initializer_list<binding_data> aBindings, descriptor_cache_interface* aDescriptorCache = nullptr)
		{
			// TODO: In the current state, we're relying on COMPATIBLE layouts. Think about reusing the pipeline's allocated and internally stored layouts!
			assert(false);
			throw cgb::logic_error("No suitable bind_descriptors overload found for the given pipeline/layout.");
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

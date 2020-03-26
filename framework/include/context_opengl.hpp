#pragma once

namespace cgb
{
	/**	@brief Context for OpenGL 4.6 core profile
	 *	
	 *	This context uses OpenGL 4.6 functionality and a core profile.
	 *	Some data and functionality is shared from the generic_opengl 
	 *  context, environment-related matters like window creation is
	 *	performed via GLFW, most of which is implemented in @ref generic_glfw
	 *  context.
	 */
	class opengl : public generic_glfw
	{
	public:
		opengl();
		opengl(const opengl&) = delete;
		opengl(opengl&&) = delete;
		opengl& operator=(const opengl&) = delete;
		opengl& operator=(opengl&&) = delete;
		~opengl() = default;

		template <typename T>
		void track_creation(T*)		{ LOG_WARNING(fmt::format("Encountered unsupported type '{}' in opengl::track_creation", typeid(T).name())); }
		template <typename T>
		void track_copy(T*, T*)		{ LOG_WARNING(fmt::format("Encountered unsupported type '{}' in opengl::track_copy", typeid(T).name())); }
		template <typename T>
		void track_move(T*, T*)		{ LOG_WARNING(fmt::format("Encountered unsupported type '{}' in opengl::track_move", typeid(T).name())); }
		template <typename T>
		void track_destruction(T*)	{ LOG_WARNING(fmt::format("Encountered unsupported type '{}' in opengl::track_destruction", typeid(T).name())); }

		/**	Creates a new window, but doesn't open it. Set the window's parameters
		 *	according to your requirements before opening it!
		 *
		 *  @thread_safety This function must only be called from the main thread.
		 */
		window* create_window(const std::string& pTitle);

		/** Completes all pending work on the device, blocks the current thread until then. */
		void finish_pending_work();

		/** Used to signal the context about the beginning of a composition */
		void begin_composition();

		/** Used to signal the context about the end of a composition */
		void end_composition();

		/** Used to signal the context about the beginning of a new frame */
		void begin_frame();

		/** Used to signal the context about the end of a frame */
		void end_frame();

		/**	Checks whether there is a GL-Error and logs it to the console
				 *	@return true if there was an error, false if not
				 */
		bool check_error(const char* file, int line);

	};

	template <>
	inline void opengl::track_creation<generic_buffer_t>(generic_buffer_t* thing) { LOG_WARNING("TODO: implement generic_buffer_t handling in context::track_creation."); }
	template <>
	inline void opengl::track_creation<uniform_buffer_t>(uniform_buffer_t* thing) { LOG_WARNING("TODO: implement uniform_buffer_t handling in context::track_creation."); }
	template <>
	inline void opengl::track_creation<uniform_texel_buffer_t>(uniform_texel_buffer_t* thing) { LOG_WARNING("TODO: implement uniform_texel_buffer_t handling in context::track_creation."); }
	template <>
	inline void opengl::track_creation<storage_buffer_t>(storage_buffer_t* thing) { LOG_WARNING("TODO: implement storage_buffer_t handling in context::track_creation."); }
	template <>
	inline void opengl::track_creation<storage_texel_buffer_t>(storage_texel_buffer_t* thing) { LOG_WARNING("TODO: implement storage_texel_buffer_t handling in context::track_creation."); }
	template <>
	inline void opengl::track_creation<vertex_buffer_t>(vertex_buffer_t* thing) { LOG_WARNING("TODO: implement vertex_buffer_t handling in context::track_creation."); }
	template <>
	inline void opengl::track_creation<index_buffer_t>(index_buffer_t* thing) { LOG_WARNING("TODO: implement index_buffer_t handling in context::track_creation."); }
	template <>
	inline void opengl::track_creation<instance_buffer_t>(instance_buffer_t* thing) { LOG_WARNING("TODO: implement instance_buffer_t handling in context::track_creation."); }

	template <>
	inline void opengl::track_move<generic_buffer_t>(generic_buffer_t* thing, generic_buffer_t* other) { LOG_WARNING("TODO: implement generic_buffer_t handling in context::track_move."); }
	template <>
	inline void opengl::track_move<uniform_buffer_t>(uniform_buffer_t* thing, uniform_buffer_t* other) { LOG_WARNING("TODO: implement uniform_buffer_t handling in context::track_move."); }
	template <>
	inline void opengl::track_move<uniform_texel_buffer_t>(uniform_texel_buffer_t* thing, uniform_texel_buffer_t* other) { LOG_WARNING("TODO: implement uniform_texel_buffer_t handling in context::track_move."); }
	template <>
	inline void opengl::track_move<storage_buffer_t>(storage_buffer_t* thing, storage_buffer_t* other) { LOG_WARNING("TODO: implement storage_buffer_t handling in context::track_move."); }
	template <>
	inline void opengl::track_move<storage_texel_buffer_t>(storage_texel_buffer_t* thing, storage_texel_buffer_t* other) { LOG_WARNING("TODO: implement storage_texel_buffer_t handling in context::track_move."); }
	template <>
	inline void opengl::track_move<vertex_buffer_t>(vertex_buffer_t* thing, vertex_buffer_t* other) { LOG_WARNING("TODO: implement vertex_buffer_t handling in context::track_move."); }
	template <>
	inline void opengl::track_move<index_buffer_t>(index_buffer_t* thing, index_buffer_t* other) { LOG_WARNING("TODO: implement index_buffer_t handling in context::track_move."); }
	template <>
	inline void opengl::track_move<instance_buffer_t>(instance_buffer_t* thing, instance_buffer_t* other) { LOG_WARNING("TODO: implement instance_buffer_t handling in context::track_move."); }

	template <>
	inline void opengl::track_destruction<generic_buffer_t>(generic_buffer_t* thing) {
		LOG_WARNING("TODO: implement generic_buffer_t handling in context::track_destruction."); 
	}
	template <>
	inline void opengl::track_destruction<uniform_buffer_t>(uniform_buffer_t* thing) { LOG_WARNING("TODO: implement uniform_buffer_t handling in context::track_destruction."); }
	template <>
	inline void opengl::track_destruction<uniform_texel_buffer_t>(uniform_texel_buffer_t* thing) { LOG_WARNING("TODO: implement uniform_texel_buffer_t handling in context::track_destruction."); }
	template <>
	inline void opengl::track_destruction<storage_buffer_t>(storage_buffer_t* thing) { LOG_WARNING("TODO: implement storage_buffer_t handling in context::track_destruction."); }
	template <>
	inline void opengl::track_destruction<storage_texel_buffer_t>(storage_texel_buffer_t* thing) { LOG_WARNING("TODO: implement storage_texel_buffer_t handling in context::track_destruction."); }
	template <>
	inline void opengl::track_destruction<vertex_buffer_t>(vertex_buffer_t* thing) { LOG_WARNING("TODO: implement vertex_buffer_t handling in context::track_destruction."); }
	template <>
	inline void opengl::track_destruction<index_buffer_t>(index_buffer_t* thing) { LOG_WARNING("TODO: implement buffer_t<index_buffer_data> handling in context::track_destruction."); }
	template <>
	inline void opengl::track_destruction<instance_buffer_t>(instance_buffer_t* thing) { LOG_WARNING("TODO: implement buffer_t<instance_buffer_t> handling in context::track_destruction."); }

	template <>
	inline void opengl::track_creation<shader>(shader* thing)				{ LOG_WARNING("TODO: implement 'shader' handling in context::track_creation."); }
	template <>
	inline void opengl::track_move<shader>(shader* thing, shader* other)	{ LOG_WARNING("TODO: implement 'shader' handling in context::track_move."); }
	template <>
	inline void opengl::track_destruction<shader>(shader* thing)			{ LOG_WARNING("TODO: implement 'shader' handling in context::track_destruction."); }

	template <>
	inline void opengl::track_creation<sampler_t>(sampler_t* thing)		{ LOG_WARNING("TODO: implement 'sampler_t' handling in context::track_creation."); }
	template <>
	inline void opengl::track_move<sampler_t>(sampler_t* thing, sampler_t* other)			{ LOG_WARNING("TODO: implement 'sampler_t' handling in context::track_move."); }
	template <>
	inline void opengl::track_destruction<sampler_t>(sampler_t* thing)	{ LOG_WARNING("TODO: implement 'sampler_t' handling in context::track_destruction."); }

	template <>
	inline void opengl::track_creation<image_view_t>(image_view_t* thing)		{ LOG_WARNING("TODO: implement 'image_view_t' handling in context::track_creation."); }
	template <>
	inline void opengl::track_move<image_view_t>(image_view_t* thing, image_view_t* other)			{ LOG_WARNING("TODO: implement 'image_view_t' handling in context::track_move."); }
	template <>
	inline void opengl::track_destruction<image_view_t>(image_view_t* thing)	{ LOG_WARNING("TODO: implement 'image_view_t' handling in context::track_destruction."); }

	template <>
	inline void opengl::track_creation<renderpass_t>(renderpass_t* thing)		{ LOG_WARNING("TODO: implement 'renderpass_t' handling in context::track_creation."); }
	template <>
	inline void opengl::track_move<renderpass_t>(renderpass_t* thing, renderpass_t* other)			{ LOG_WARNING("TODO: implement 'renderpass_t' handling in context::track_move."); }
	template <>
	inline void opengl::track_destruction<renderpass_t>(renderpass_t* thing)	{ LOG_WARNING("TODO: implement 'renderpass_t' handling in context::track_destruction."); }

	template <>
	inline void opengl::track_creation<graphics_pipeline_t>(graphics_pipeline_t* thing)		{ LOG_WARNING("TODO: implement 'graphics_pipeline_t' handling in context::track_creation."); }
	template <>
	inline void opengl::track_move<graphics_pipeline_t>(graphics_pipeline_t* thing, graphics_pipeline_t* other)			{ LOG_WARNING("TODO: implement 'graphics_pipeline_t' handling in context::track_move."); }
	template <>
	inline void opengl::track_destruction<graphics_pipeline_t>(graphics_pipeline_t* thing)	{ LOG_WARNING("TODO: implement 'graphics_pipeline_t' handling in context::track_destruction."); }

	template <>
	inline void opengl::track_creation<framebuffer_t>(framebuffer_t* thing)		{ LOG_WARNING("TODO: implement 'framebuffer_t' handling in context::track_creation."); }
	template <>
	inline void opengl::track_move<framebuffer_t>(framebuffer_t* thing, framebuffer_t* other)			{ LOG_WARNING("TODO: implement 'framebuffer_t' handling in context::track_move."); }
	template <>
	inline void opengl::track_destruction<framebuffer_t>(framebuffer_t* thing)	{ LOG_WARNING("TODO: implement 'framebuffer_t' handling in context::track_destruction."); }

}

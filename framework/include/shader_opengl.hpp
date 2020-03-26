#pragma once

namespace cgb
{
	/** Represents a shader program handle for the Vulkan context */
	class shader
	{
	public:
		shader() = default;
		shader(shader&&) noexcept = default;
		shader(const shader&) = delete;
		shader& operator=(shader&&) noexcept = default;
		shader& operator=(const shader&) = delete;
		~shader() = default;

		static shader prepare(shader_info pInfo);
		static shader create(shader_info pInfo);

	private:
		shader_info mInfo;
		std::string mActualShaderLoadPath;
		context_tracker<shader> mTracker;
	};

}

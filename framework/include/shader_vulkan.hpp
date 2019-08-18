#pragma once

namespace cgb
{
	/** Represents a shader program handle for the Vulkan context */
	class shader
	{
	public:
		shader() = default;
		shader(const shader&) = delete;
		shader(shader&&) = default;
		shader& operator=(const shader&) = delete;
		shader& operator=(shader&&) = default;
		~shader() = default;

		const auto& handle() const { return mShaderModule.get(); }
		const auto* handle_addr() const { return &mShaderModule.get(); }
		const auto& info() const { return mInfo; }

		static vk::UniqueShaderModule build_from_file(const std::string& pPath);
		static vk::UniqueShaderModule build_from_binary_code(const std::vector<char>& pCode);
		bool has_been_built() const;

		static shader prepare(shader_info pInfo);
		static shader create(shader_info pInfo);

	private:
		shader_info mInfo;
		vk::UniqueShaderModule mShaderModule;
		std::string mActualShaderLoadPath;
		context_tracker<shader> mTracker;
	};

}

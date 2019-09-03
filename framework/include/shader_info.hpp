#pragma once

namespace cgb
{
	struct shader_info
	{
		static shader_info create(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false, std::optional<cgb::shader_type> pShaderType = {});

		std::string mPath;
		cgb::shader_type mShaderType;
		std::string mEntryPoint;
		bool mDontMonitorFile;
	};

	static bool operator ==(const shader_info& left, const shader_info& right)
	{
		return are_paths_equal(left.mPath, right.mPath)
			&& left.mShaderType == right.mShaderType 
			&& trim_spaces(left.mEntryPoint) == trim_spaces(right.mEntryPoint);
	}

	static bool operator !=(const shader_info& left, const shader_info& right)
	{
		return !(left == right);
	}

	/** @brief Shader source information and shader loading options
	 *
	 *	This information is important especially for shader hot reloading.
	 */
	enum struct shader_source_info : uint32_t
	{
		nothing			= 0x0000,
		/** Shader source is loaded from a file */
		from_file		= 0x0001,
		/** Shader source is loaded from memory (a string most likely) */
		from_memory		= 0x0002,
		/** Load the shader and append a new-line to the source */
		append_newline	= 0x0004,
	};

	inline shader_source_info operator| (shader_source_info a, shader_source_info b)
	{
		typedef std::underlying_type<shader_source_info>::type EnumType;
		return static_cast<shader_source_info>(static_cast<EnumType>(a) | static_cast<EnumType>(b));
	}

	inline shader_source_info operator& (shader_source_info a, shader_source_info b)
	{
		typedef std::underlying_type<shader_source_info>::type EnumType;
		return static_cast<shader_source_info>(static_cast<EnumType>(a) & static_cast<EnumType>(b));
	}

	inline shader_source_info& operator |= (shader_source_info& a, shader_source_info b)
	{
		return a = a | b;
	}

	inline shader_source_info& operator &= (shader_source_info& a, shader_source_info b)
	{
		return a = a & b;
	}

	inline shader_info vertex_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::vertex); }
	inline shader_info tessellation_control_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::tessellation_control); }
	inline shader_info tessellation_evaluation_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::tessellation_evaluation); }
	inline shader_info geometry_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::geometry); }
	inline shader_info fragment_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::fragment); }
	inline shader_info compute_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::compute); }
	inline shader_info ray_generation_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::ray_generation); }
	inline shader_info any_hit_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::any_hit); }
	inline shader_info closest_hit_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::closest_hit); }
	inline shader_info miss_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::miss); }
	inline shader_info intersection_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::intersection); }
	inline shader_info callable_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::callable); }
	inline shader_info task_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::task); }
	inline shader_info mesh_shader(std::string pPath, std::string pEntryPoint = "main", bool pDontMonitorFile = false) { return shader_info::create(std::move(pPath), std::move(pEntryPoint), pDontMonitorFile, cgb::shader_type::mesh); }

}

namespace std // Inject hash for `cgb::shader_info` into std::
{
	template<> struct hash<cgb::shader_info>
	{
		std::size_t operator()(cgb::shader_info const& o) const noexcept
		{
			std::size_t h = 0;
			cgb::hash_combine(h,
				cgb::transform_path_for_comparison(o.mPath),
				static_cast<std::underlying_type<cgb::shader_type>::type>(o.mShaderType),
				cgb::trim_spaces(o.mEntryPoint)
			);
			return h;
		}
	};
}

namespace cgb
{
	shader_info shader_info::create(std::string pPath, std::string pEntryPoint, bool pDontMonitorFile, std::optional<cgb::shader_type> pShaderType)
	{
		pPath = trim_spaces(pPath);
		if (!pShaderType.has_value()) {
			// "classical" shaders
			if (pPath.ends_with(".vert"))	{ pShaderType = cgb::shader_type::vertex; }
			else if (pPath.ends_with(".tesc"))	{ pShaderType = cgb::shader_type::tessellation_control; }
			else if (pPath.ends_with(".tese"))	{ pShaderType = cgb::shader_type::tessellation_evaluation; }
			else if (pPath.ends_with(".geom"))	{ pShaderType = cgb::shader_type::geometry; }
			else if (pPath.ends_with(".frag"))	{ pShaderType = cgb::shader_type::fragment; }
			else if (pPath.ends_with(".comp"))	{ pShaderType = cgb::shader_type::compute; }
			// ray tracing shaders
			else if (pPath.ends_with(".rgen"))	{ pShaderType = cgb::shader_type::ray_generation; }
			else if (pPath.ends_with(".rahit"))	{ pShaderType = cgb::shader_type::any_hit; }
			else if (pPath.ends_with(".rchit"))	{ pShaderType = cgb::shader_type::closest_hit; }
			else if (pPath.ends_with(".rmiss"))	{ pShaderType = cgb::shader_type::miss; }
			else if (pPath.ends_with(".rint"))	{ pShaderType = cgb::shader_type::intersection; }
			// callable shader
			else if (pPath.ends_with(".call"))	{ pShaderType = cgb::shader_type::callable; }
			// mesh shaders
			else if (pPath.ends_with(".task"))	{ pShaderType = cgb::shader_type::task; }
			else if (pPath.ends_with(".mesh"))	{ pShaderType = cgb::shader_type::mesh; }
		}

		if (!pShaderType.has_value()) {
			throw std::runtime_error("No shader type set and could not infer it from the file ending.");
		}

		return shader_info
		{
			std::move(pPath),
			pShaderType.value(),
			std::move(pEntryPoint),
			pDontMonitorFile
		};
	}
}

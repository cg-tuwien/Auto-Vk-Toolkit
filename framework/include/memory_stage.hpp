#pragma once

namespace cgb
{
	enum struct memory_stage
	{
        indirect_command,
		index,
		vertex_attribute,
		uniform,
		input_attachment,
		color_attachment,
		depth_stencil_attachment,
		transfer,
		conditional_rendering,
		transform_feedback,
		command_process,
		shading_rate_image,
		acceleration_structure,
		shader_access,
		host_access,
		any_access
	};
}

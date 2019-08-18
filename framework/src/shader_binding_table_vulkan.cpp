namespace cgb
{
	shader_binding_table::shader_binding_table() noexcept
	{ }

	shader_binding_table::shader_binding_table(size_t pSize, const vk::BufferUsageFlags& pBufferFlags, const vk::Buffer& pBuffer, const vk::MemoryPropertyFlags& pMemoryProperties, const vk::DeviceMemory& pMemory) noexcept
	{ }

	shader_binding_table::shader_binding_table(shader_binding_table&& other) noexcept
	{ }

	shader_binding_table& shader_binding_table::operator=(shader_binding_table&& other) noexcept
	{ 
		return *this;
	}

	shader_binding_table::~shader_binding_table()
	{ }

	shader_binding_table shader_binding_table::create(const graphics_pipeline_t& pRtPipeline)
	{
		auto numGroups = 5u; // TODO: store groups in `pipeline` (or rather in `ray_tracing_pipeline : pipeline`) and then, read from pRtPipeline
		auto rtProps = context().get_ray_tracing_properties();
		auto shaderBindingTableSize = rtProps.shaderGroupHandleSize * numGroups;

		// TODO: Use *new* buffer_t
		//auto b = buffer::create(shaderBindingTableSize,
		//						vk::BufferUsageFlagBits::eTransferSrc,
		//						vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

		//void* mapped = context().logical_device().mapMemory(b.mMemory, 0, b.mSize);
		//// Transfer something into the buffer's memory...
		//context().logical_device().getRayTracingShaderGroupHandlesNV(pRtPipeline.mPipeline, 0, numGroups, b.mSize, mapped, context().dynamic_dispatch());
		//context().logical_device().unmapMemory(b.mMemory);

		auto sbt = shader_binding_table();
		//static_cast<buffer&>(sbt) = std::move(b);
		return sbt;
	}
}

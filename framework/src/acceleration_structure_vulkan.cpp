namespace cgb
{
	acceleration_structure::acceleration_structure() noexcept
		: mAccStructureInfo()
		, mAccStructure(nullptr)
		, mHandle()
		, mMemoryProperties()
		, mMemory(nullptr)
	{ }

	acceleration_structure::acceleration_structure(acceleration_structure&& other) noexcept
		: mAccStructureInfo(std::move(other.mAccStructureInfo))
		, mAccStructure(std::move(other.mAccStructure))
		, mHandle(std::move(other.mHandle))
		, mMemoryProperties(std::move(other.mMemoryProperties))
		, mMemory(std::move(other.mMemory))
	{ 
		other.mAccStructureInfo = vk::AccelerationStructureInfoNV();
		other.mAccStructure = nullptr;
		other.mHandle = acceleration_structure_handle();
		other.mMemoryProperties = vk::MemoryPropertyFlags();
		other.mMemory = nullptr;
	}

	acceleration_structure& acceleration_structure::operator=(acceleration_structure&& other) noexcept
	{ 
		mAccStructureInfo = std::move(other.mAccStructureInfo);
		mAccStructure = std::move(other.mAccStructure);
		mHandle = std::move(other.mHandle);
		mMemoryProperties = std::move(other.mMemoryProperties);
		mMemory = std::move(other.mMemory);
		other.mAccStructureInfo = vk::AccelerationStructureInfoNV();
		other.mAccStructure = nullptr;
		other.mHandle = acceleration_structure_handle();
		other.mMemoryProperties = vk::MemoryPropertyFlags();
		other.mMemory = nullptr;
		return *this;
	}

	acceleration_structure::~acceleration_structure()
	{
		if (mAccStructure) {
			context().logical_device().destroyAccelerationStructureNV(mAccStructure, nullptr, cgb::context().dynamic_dispatch());
			mAccStructure = nullptr;
		}
		if (mMemory) {
			context().logical_device().freeMemory(mMemory);
			mMemory = nullptr;
		}
	}

	acceleration_structure acceleration_structure::create_top_level(uint32_t pInstanceCount)
	{
		return acceleration_structure::create(vk::AccelerationStructureTypeNV::eTopLevel, {}, pInstanceCount);
	}

	acceleration_structure acceleration_structure::create_bottom_level(const std::vector<vk::GeometryNV>& pGeometries)
	{
		return acceleration_structure::create(vk::AccelerationStructureTypeNV::eBottomLevel, pGeometries, 0);
	}

	acceleration_structure acceleration_structure::create(vk::AccelerationStructureTypeNV pType, const std::vector<vk::GeometryNV>& pGeometries, uint32_t pInstanceCount)
	{
		assert(pType == vk::AccelerationStructureTypeNV::eBottomLevel && pGeometries.size() > 0 || pInstanceCount > 0);
		// If type is VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV then geometryCount must be 0
		// If type is VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV then instanceCount must be 0

		auto accInfo = vk::AccelerationStructureInfoNV()
			.setType(pType)
			.setFlags(vk::BuildAccelerationStructureFlagsNV())
			.setInstanceCount(pType == vk::AccelerationStructureTypeNV::eBottomLevel ? 0 : pInstanceCount)
			.setGeometryCount(pType == vk::AccelerationStructureTypeNV::eTopLevel ? 0 : static_cast<uint32_t>(pGeometries.size()))
			.setPGeometries(pType == vk::AccelerationStructureTypeNV::eTopLevel ? nullptr : pGeometries.data());

		auto createInfo = vk::AccelerationStructureCreateInfoNV()
			.setCompactedSize(0)
			.setInfo(accInfo);
		auto accStructure = context().logical_device().createAccelerationStructureNV(createInfo, nullptr, cgb::context().dynamic_dispatch());

		auto accStructMemInfo = vk::AccelerationStructureMemoryRequirementsInfoNV()
			.setAccelerationStructure(accStructure)
			.setType(vk::AccelerationStructureMemoryRequirementsTypeNV::eObject);
		auto memRequirements = context().logical_device().getAccelerationStructureMemoryRequirementsNV(accStructMemInfo, cgb::context().dynamic_dispatch());

		auto memPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;

		auto allocInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memRequirements.memoryRequirements.size)
			.setMemoryTypeIndex(context().find_memory_type_index(
				memRequirements.memoryRequirements.memoryTypeBits,
				memPropertyFlags));
		auto deviceMemory = context().logical_device().allocateMemory(allocInfo);

		// bind memory to acceleration structure
		auto bindInfo = vk::BindAccelerationStructureMemoryInfoNV()
			.setAccelerationStructure(accStructure)
			.setMemory(deviceMemory)
			.setMemoryOffset(0)
			.setDeviceIndexCount(0)
			.setPDeviceIndices(nullptr);
		context().logical_device().bindAccelerationStructureMemoryNV({ bindInfo }, cgb::context().dynamic_dispatch());

		acceleration_structure_handle handle;
		context().logical_device().getAccelerationStructureHandleNV(accStructure, sizeof(handle.mHandle), &handle.mHandle, cgb::context().dynamic_dispatch());

		//return acceleration_structure(accInfo, accStructure, handle, memPropertyFlags, deviceMemory);
		return acceleration_structure{};
	}

	size_t acceleration_structure::get_scratch_buffer_size()
	{
		auto memReqInfo = vk::AccelerationStructureMemoryRequirementsInfoNV()
			.setAccelerationStructure(mAccStructure)
			.setType(vk::AccelerationStructureMemoryRequirementsTypeNV::eBuildScratch);

		auto memRequirements = context().logical_device().getAccelerationStructureMemoryRequirementsNV(memReqInfo, cgb::context().dynamic_dispatch());
		return static_cast<size_t>(memRequirements.memoryRequirements.size);
	}
}

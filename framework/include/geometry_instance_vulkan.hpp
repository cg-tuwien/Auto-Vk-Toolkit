#pragma once

namespace cgb
{
	class bottom_level_acceleration_structure_t;

	struct geometry_instance
	{
		/** Create a geometry instance for a specific geometry, which is represented by a bottom level acceleration structure.
		 *	@param	_Blas	The bottom level acceleration structure which represents the underlying geometry for this instance
		 */
		static geometry_instance create(const bottom_level_acceleration_structure_t& _Blas);

		geometry_instance& set_transform(glm::mat4 _TransformationMatrix);
		geometry_instance& set_custom_index(uint32_t _CustomIndex);
		geometry_instance& set_mask(uint32_t _Mask);
		geometry_instance& set_instance_offset(size_t _Offset);
		geometry_instance& disable_culling();
		geometry_instance& define_front_faces_to_be_counter_clockwise();
		geometry_instance& force_opaque();
		geometry_instance& force_non_opaque();

		glm::mat4 mTransform;
		uint32_t mInstanceCustomIndex;
		uint32_t mMask;
		size_t mInstanceOffset;
		vk::GeometryInstanceFlagsNV mFlags;
		uint64_t mAccelerationStructureDeviceHandle;
	};

	struct VkGeometryInstanceNV
	{
		float		transform[12];
		uint32_t	instanceCustomIndex : 24;
		uint32_t	mask : 8;
		uint32_t	instanceOffset : 24;
		uint32_t	flags : 8;
		uint64_t	accelerationStructureHandle;
	};

	extern VkGeometryInstanceNV convert_for_gpu_usage(const geometry_instance& _GeomInst);
	extern std::vector<VkGeometryInstanceNV> convert_for_gpu_usage(const std::vector<geometry_instance>& _GeomInstances);

}
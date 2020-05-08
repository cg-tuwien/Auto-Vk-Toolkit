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

		/** Set the transformation matrix of this geometry instance. */
		geometry_instance& set_transform(glm::mat4 _TransformationMatrix);
		/** Set the custom index assigned to this geometry instance. */
		geometry_instance& set_custom_index(uint32_t _CustomIndex);
		/** Set the mask for this geometry instance. */
		geometry_instance& set_mask(uint32_t _Mask);
		/** Set the instance offset parameter assigned to this geometry instance. */
		geometry_instance& set_instance_offset(size_t _Offset);
		/** Add the flag to disable triangle culling for this geometry instance. */
		geometry_instance& disable_culling();
		/** Add the flag which indicates that triangle front faces are given in counter-clockwise order. */
		geometry_instance& define_front_faces_to_be_counter_clockwise();
		/** Add a flag to force this geometry instance to be opaque. */
		geometry_instance& force_opaque();
		/** Add a flag to force this geometry instance to be treated as being non-opaque. */
		geometry_instance& force_non_opaque();
		/** Reset all the flag values. After calling this method, this geometry instance is in a state without any flags set. */
		geometry_instance& reset_flags();

		glm::mat4 mTransform;
		uint32_t mInstanceCustomIndex;
		uint32_t mMask;
		size_t mInstanceOffset;
		vk::GeometryInstanceFlagsKHR mFlags;
		vk::DeviceAddress mAccelerationStructureDeviceHandle;
	};

	extern VkAccelerationStructureInstanceKHR convert_for_gpu_usage(const geometry_instance& _GeomInst);
	extern std::vector<VkAccelerationStructureInstanceKHR> convert_for_gpu_usage(const std::vector<geometry_instance>& _GeomInstances);

}
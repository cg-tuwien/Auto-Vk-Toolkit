#include <cg_base.hpp>

namespace cgb
{
	owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(std::vector<std::tuple<vertex_buffer, index_buffer>> _GeometryDescriptions, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeMemoryAlloc)
	{
		bottom_level_acceleration_structure_t result;
		result.mVertexBuffers.reserve(_GeometryDescriptions.size());
		result.mIndexBuffers.reserve(_GeometryDescriptions.size());
		result.mGeometries.reserve(_GeometryDescriptions.size());

		// 1. Gather all geometry descriptions and create vk::GeometryTypeNV entries:
		for (auto& tpl : _GeometryDescriptions) {
			// Perform two sanity checks, because we really need the member descriptions to know where to find the positions.
			// 1st check:
			if (std::get<vertex_buffer>(tpl)->meta_data().member_descriptions().size() == 0) {
				throw std::runtime_error("cgb::vertex_buffers passed to bottom_level_acceleration_structure_t::create must have a member_description for their positions element in their meta data.");
			}

			// Store the data and assemble vk::GeometryNV info:
			result.mVertexBuffers.push_back(std::move(std::get<vertex_buffer>(tpl)));
			// Find member representing the positions, and...
			auto posMember = std::find_if(
				std::begin(result.mVertexBuffers.back()->meta_data().member_descriptions()), 
				std::end(result.mVertexBuffers.back()->meta_data().member_descriptions()), 
				[](const buffer_element_member_meta& md) {
					return md.mContent == content_description::position;
				});
			// ... perform 2nd check:
			if (posMember == std::end(result.mVertexBuffers.back()->meta_data().member_descriptions())) {
				throw std::runtime_error("cgb::vertex_buffers passed to bottom_level_acceleration_structure_t::create has no member which represents positions.");
			}
			result.mIndexBuffers.push_back(std::move(std::get<index_buffer>(tpl)));


			// Temporary check:
			// TODO: Remove the following line:
			assert(vk::Format::eR32G32B32Sfloat == posMember->mFormat.mFormat);


			result.mGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeNV::eTriangles)
				.setGeometry(vk::GeometryDataNV{} // TODO: Support AABBs (not only triangles)
					.setTriangles(vk::GeometryTrianglesNV()
						.setVertexData(result.mVertexBuffers.back()->buffer_handle())
						.setVertexOffset(posMember->mOffset)
						.setVertexCount(static_cast<uint32_t>(result.mVertexBuffers.back()->meta_data().num_elements()))
						.setVertexStride(result.mVertexBuffers.back()->meta_data().sizeof_one_element())
						.setVertexFormat(posMember->mFormat.mFormat)
						.setIndexData(result.mIndexBuffers.back()->buffer_handle())
						.setIndexOffset(0) // TODO: In which cases might this not be zero?
						.setIndexCount(static_cast<uint32_t>(result.mIndexBuffers.back()->meta_data().num_elements()))
						.setIndexType(to_vk_index_type(result.mIndexBuffers.back()->meta_data().sizeof_one_element()))
						// transformData is a buffer containing optional reference to an array of 32-bit floats 
						// representing a 3x4 row major affine transformation matrix for this geometry.
						.setTransformData(nullptr)
						// transformOffset is the offset in bytes in transformData of the transform 
						// information described above.
						.setTransformOffset(0))
						// TODO: Support transformData and transformOffset
				)
				.setFlags(vk::GeometryFlagBitsNV::eOpaque); // TODO: Support the other flag as well!
		} // for each geometry description

		// 2. Assemble info about the BOTTOM LEVEL acceleration structure and the set its geometry
		result.mAccStructureInfo = vk::AccelerationStructureInfoNV{}
			.setType(vk::AccelerationStructureTypeNV::eBottomLevel)
			.setFlags(vk::BuildAccelerationStructureFlagBitsNV::ePreferFastBuild) // TODO: Support flags
			.setInstanceCount(0u)
			.setGeometryCount(static_cast<uint32_t>(result.mGeometries.size()))
			.setPGeometries(result.mGeometries.data());

		// 3. Maybe alter the config?
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		// 4. Create the create info
		auto createInfo = vk::AccelerationStructureCreateInfoNV{}
			.setCompactedSize(0)
			.setInfo(result.mAccStructureInfo);
		// 5. Create it
		result.mAccStructure = context().logical_device().createAccelerationStructureNVUnique(createInfo, nullptr, context().dynamic_dispatch());

		// Steps 6. to 11. in here:
		finish_acceleration_structure_creation(result, std::move(_AlterConfigBeforeMemoryAlloc));
		
		result.mTracker.setTrackee(result);
		return result;
	}
}

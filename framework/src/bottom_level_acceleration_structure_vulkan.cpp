#include <cg_base.hpp>

namespace cgb
{
	owning_resource<bottom_level_acceleration_structure_t> bottom_level_acceleration_structure_t::create(std::vector<std::tuple<vertex_buffer, index_buffer>> _GeometryDescriptions, cgb::context_specific_function<void(bottom_level_acceleration_structure_t&)> _AlterConfigBeforeCreation)
	{
		bottom_level_acceleration_structure_t result;
		result.mVertexBuffers.reserve(_GeometryDescriptions.size());
		result.mIndexBuffers.reserve(_GeometryDescriptions.size());
		result.mGeometries.reserve(_GeometryDescriptions.size());

		for (auto& tpl : _GeometryDescriptions) {
			// First of all, a sanity check, because we really need the member descriptions
			if (std::get<vertex_buffer>(tpl)->meta_data().member_descriptions().size() == 0	|| std::get<vertex_buffer>(tpl)->meta_data().member_descriptions()[0].mLocation != 0) {
				throw std::runtime_error("cgb::vertex_buffers passed to bottom_level_acceleration_structure_t::create must have a member_description for their positions element in their meta data AND have it assigned to location 0.");
			}

			// Store the data and assemble vk::GeometryNV info:
			result.mVertexBuffers.push_back(std::move(std::get<vertex_buffer>(tpl)));
			result.mIndexBuffers.push_back(std::move(std::get<index_buffer>(tpl)));
			auto& dataDescr = std::get<input_binding_location_data>(tpl);


			// Temporary check:
			// TODO: Remove the following line:
			assert(vk::Format::eR32G32B32Sfloat == result.mVertexBuffers.back()->meta_data().member_descriptions()[0].mFormat.mFormat);


			result.mGeometries.emplace_back()
				.setGeometryType(vk::GeometryTypeNV::eTriangles)
				.setGeometry(vk::GeometryDataNV{}			// TODO: Support AABBs
					.setTriangles(vk::GeometryTrianglesNV()
						.setVertexData(result.mVertexBuffers.back()->buffer_handle())
						.setVertexOffset(0) // TODO: In which cases might this not be zero?
						.setVertexCount(result.mVertexBuffers.back()->meta_data().num_elements())
						.setVertexStride(result.mVertexBuffers.back()->meta_data().sizeof_one_element())
						.setVertexFormat(result.mVertexBuffers.back()->meta_data().member_descriptions()[0].mFormat.mFormat)
						.setIndexData(result.mIndexBuffers.back()->buffer_handle())
						.setIndexOffset(0) // TODO: In which cases might this not be zero?
						.setIndexCount(result.mIndexBuffers.back()->meta_data().num_elements())
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
		}

		// TODO: Proceed here!
	}
}

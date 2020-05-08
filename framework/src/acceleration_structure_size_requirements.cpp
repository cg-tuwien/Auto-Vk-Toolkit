#include <cg_base.hpp>

namespace cgb
{
	acceleration_structure_size_requirements acceleration_structure_size_requirements::from_buffers(const cgb::vertex_buffer_t& aVertexBuffer, const cgb::index_buffer_t& aIndexBuffer)
	{
		// Perform two sanity checks, because we really need the member descriptions to know where to find the positions.
		// 1st check:
		if (aVertexBuffer.meta_data().member_descriptions().size() == 0) {
			throw cgb::runtime_error("cgb::vertex_buffers passed to acceleration_structure_size_requirements::from_buffers must have a member_description for their positions element in their meta data.");
		}

		// Find member representing the positions, and...
		auto posMember = std::find_if(
			std::begin(aVertexBuffer.meta_data().member_descriptions()), 
			std::end(aVertexBuffer.meta_data().member_descriptions()), 
			[](const buffer_element_member_meta& md) {
				return md.mContent == content_description::position;
			});
		// ... perform 2nd check:
		if (posMember == std::end(aVertexBuffer.meta_data().member_descriptions())) {
			throw cgb::runtime_error("cgb::vertex_buffers passed to acceleration_structure_size_requirements::from_buffers has no member which represents positions.");
		}
		
		return acceleration_structure_size_requirements{
			static_cast<uint32_t>(aIndexBuffer.meta_data().num_elements()) / 3u,
			aIndexBuffer.meta_data().sizeof_one_element(),
			static_cast<uint32_t>(aVertexBuffer.meta_data().num_elements()),
			posMember->mFormat
		};
	}

	acceleration_structure_size_requirements acceleration_structure_size_requirements::from_buffers(const cgb::index_buffer_t& aIndexBuffer, const cgb::vertex_buffer_t& aVertexBuffer)
	{
		return from_buffers(aVertexBuffer, aIndexBuffer);
	}
}

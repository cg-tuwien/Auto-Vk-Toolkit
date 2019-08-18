#pragma once

namespace cgb
{
	struct input_binding_general_data
	{
		enum struct kind { vertex, instance };

		uint32_t mBinding;
		size_t mStride;
		kind mKind;
	};

	inline bool operator ==(const input_binding_general_data& left, const input_binding_general_data& right)
	{
		return left.mBinding == right.mBinding && left.mStride == right.mStride && left.mKind == right.mKind;
	}

	inline bool operator <(const input_binding_general_data& left, const input_binding_general_data& right)
	{
		return left.mBinding < right.mBinding 
			|| (left.mBinding == right.mBinding && (left.mStride < right.mStride 
												|| (left.mStride == right.mStride && left.mKind == input_binding_general_data::kind::vertex && right.mKind == input_binding_general_data::kind::instance)
				));
	}


	/** One specific input location description to a graphics pipeline,
	 *	used to gather data for the creation of a `input_description` instance.
	 */
	struct input_binding_location_data
	{
		inline input_binding_location_data& from_buffer_at_binding(uint32_t _BindingIndex)
		{
			mGeneralData.mBinding = _BindingIndex;
			return *this;
		}

		input_binding_general_data mGeneralData;
		buffer_element_member_meta mMemberMetaData;
	};

	/**	Describes the input to a graphics pipeline
	 */
	class input_description
	{
		// The buffers at the binding locations 0, 1, ...
		// Each one of those buffer-metas contains the separate locations as childs.
		using input_buffer_t = std::variant<std::monostate, vertex_buffer_meta, instance_buffer_meta>;

	public:
		/** Create a complete input description record based on multiple `input_binding_location_data` records. */
		static input_description create(std::initializer_list<input_binding_location_data> pBindings);

	private:
		// Contains all the data, ordered by the binding ids
		// (and internally ordered by locations)
		std::map<uint32_t, input_buffer_t> mInputBuffers;
	};

	/** Describe an input location for a pipeline's vertex input.
	 *	The binding point is set to 0 in this case (opposed to `cgb::vertex_input_bindign` where you have to specify it),
	 *	but you'll have to set it to some other value if you are going to use multiple buffers. 
	 *	Suggested usage/example: `cgb::vertex_input_location(0, &Vertex::position).from_buffer_at_binding(0);`
	 *	The binding point represents a specific buffer which provides the data for the location specified.
	 */
	inline input_binding_location_data vertex_input_location(uint32_t pLocation, size_t pOffset, buffer_member_format pFormat, size_t pStride)
	{
		return input_binding_location_data{ 
			{ 0u, pStride, input_binding_general_data::kind::vertex},
			{ pLocation, pOffset, pFormat } 
		};
	}

	// TODO: Figure out how to use this best
	template <class M>
	inline input_binding_location_data vertex_input_location(uint32_t pLocation, const M&)
	{
		return vertex_input_location(pLocation, 0, format_for<M>(), sizeof(M));
	}

#if defined(_MSC_VER) && defined(__cplusplus)
	/** Describe an input location for a pipeline's vertex input.
	 *	The binding point is set to 0 in this case (opposed to `cgb::vertex_input_bindign` where you have to specify it),
	 *	but you'll have to set it to some other value if you are going to use multiple buffers. 
	 *	Suggested usage/example: `cgb::vertex_input_location(0, &Vertex::position).from_buffer_at_binding(0);`
	 *	The binding point represents a specific buffer which provides the data for the location specified.
	 *	
	 *	Usage example:
	 *	```
	 *  {
	 *		vertex_input_location(0, &Vertex::pos);
	 *		vertex_input_location(1, &Vertex::color);
	 *		vertex_input_location(2, &Vertex::texCoord);
	 *  }
	 *	```
	 *
	 *	Or if all the data comes from different buffers:
	 *	```
	 *  {
	 *		vertex_input_location(0, &Vertex::pos).from_buffer_at_binding(0);
	 *		vertex_input_location(1, &Vertex::color).from_buffer_at_binding(1);
	 *		vertex_input_location(2, &Vertex::texCoord).from_buffer_at_binding(2);
	 *  }
	 *	```
	 */
	template <class T, class M> 
	input_binding_location_data vertex_input_location(uint32_t pLocation, M T::* pMember)
	{
		return vertex_input_location(
			pLocation, 
			// ReSharper disable CppCStyleCast
			((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*pMember))),
			// ReSharper restore CppCStyleCast
			format_for<M>(),
			sizeof(T));
	}
#endif


	/** Describe an input location for a pipeline's instance input.
	*	The binding point is set to 0 in this case (opposed to `cgb::vertex_input_bindign` where you have to specify it),
	 *	but you'll have to set it to some other value if you are going to use multiple buffers. 
	 *	Suggested usage/example: `cgb::vertex_input_location(0, &Vertex::position).from_buffer_at_binding(0);`
	 *	The binding point represents a specific buffer which provides the data for the location specified.
	*/
	inline input_binding_location_data instance_input_location(uint32_t pLocation, size_t pOffset, buffer_member_format pFormat, size_t pStride)
	{
		return input_binding_location_data{ 
			{ 0u, pStride, input_binding_general_data::kind::instance}, 
			{ pLocation, pOffset, pFormat } 
		};
	}

#if defined(_MSC_VER) && defined(__cplusplus)

	/** Describe an input location for a pipeline's instance input.
	*	Also, assign the input location to a specific binding point (first parameter `pBinding`).
	*	The binding point represents a specific buffer which provides the data for the location specified.
	*	
	*	Usage example:
	*	```
	*  {
	*		instance_input_location(0, &SomeInstanceData::color);
	*  }
	*	```
	*/
	template <class T, class M> 
	input_binding_location_data instance_input_location(uint32_t pLocation, M T::* pMember)
	{
		return instance_input_location(
			pLocation, 
			// ReSharper disable CppCStyleCast
			((::size_t)&reinterpret_cast<char const volatile&>((((T*)0)->*pMember))),
			// ReSharper restore CppCStyleCast
			format_for<M>(),
			sizeof(T));
	}
#endif
}

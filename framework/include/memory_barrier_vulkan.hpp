#pragma once

namespace cgb
{
	enum struct memory_barrier_type { global, buffer, image };

	/**	Defines a memory barrier. 
	 */
	class memory_barrier
	{
		/** Create a very coarse global memory barrier which makes the writes of all source stages
		 *	available the reads of all destination stages.
		 *	In order to establish a tighter memory barrier, consider using one of the parameterized
		 *	`memory_barrier::create_read_after_write()` methods!
		 */
		static constexpr memory_barrier create_read_after_write();
		
		/** Create a very coarse global memory barrier which makes the writes of all source stages
		 *	available before writes in all destination stages happen.
		 *	In order to establish a tighter memory barrier, consider using one of the parameterized
		 *	`memory_barrier::create_write_after_write()` methods!
		 */
		static constexpr memory_barrier create_write_after_write();
		
		/**	Create a global read-after-write memory barrier between two given memory stages.
		 *	@param		aMemoryToMakeVisible	This is the stage, which writes memory a.k.a. the SOURCE-stage.
		 *										The memory of this stage is to be made visible (i.e. transferred into L2)
		 *	@param		aMemoryToMakeAvailable	This is the stage, which reads memory a.k.a. the DESTINATION-stage.
		 *										There, the memory of the source stage shall be available (i.e. transferred into L1)
		 */
		static constexpr memory_barrier create_read_after_write(memory_access aMemoryToMakeVisible, memory_access aMemoryToMakeAvailable);

		/**	Create a global write-after-write memory barrier between two given memory stages.
		 *	@param		aMemoryToMakeVisible	This is the stage, which writes memory a.k.a. the SOURCE-stage.
		 *										The memory of this stage is to be made visible (i.e. transferred into L2)
		 *	@param		aMemoryToMakeAvailable	This is the stage, which reads memory a.k.a. the DESTINATION-stage.
		 *										There, the memory of the source stage shall be available (i.e. transferred into L1)
		 */
		static constexpr memory_barrier create_write_after_write(memory_access aMemoryToMakeVisible, memory_access aMemoryToMakeAvailable);

		/** Create a very coarse image memory barrier which makes the writes of all source stages
		 *	available the reads of all destination stages.
		 *	In order to establish a tighter memory barrier, consider using one of the parameterized
		 *	`memory_barrier::create_read_after_write()` methods!
		 */
		static constexpr memory_barrier create_read_after_write(const image_t& aImage);
		
		/** Create a very coarse image memory barrier which makes the writes of all source stages
		 *	available before writes in all destination stages happen.
		 *	In order to establish a tighter memory barrier, consider using one of the parameterized
		 *	`memory_barrier::create_write_after_write()` methods!
		 */
		static constexpr memory_barrier create_write_after_write(const image_t& aImage);
		
		/**	Create a read-after-write image memory barrier between two given memory stages.
		 *	@param		aMemoryToMakeVisible	This is the stage, which writes memory a.k.a. the SOURCE-stage.
		 *										The memory of this stage is to be made visible (i.e. transferred into L2)
		 *	@param		aMemoryToMakeAvailable	This is the stage, which reads memory a.k.a. the DESTINATION-stage.
		 *										There, the memory of the source stage shall be available (i.e. transferred into L1)
		 */
		static constexpr memory_barrier create_read_after_write(memory_access aMemoryToMakeVisible, memory_access aMemoryToMakeAvailable, const image_t& aImage);

		/**	Create a write-after-write image memory barrier between two given memory stages.
		 *	@param		aMemoryToMakeVisible	This is the stage, which writes memory a.k.a. the SOURCE-stage.
		 *										The memory of this stage is to be made visible (i.e. transferred into L2)
		 *	@param		aMemoryToMakeAvailable	This is the stage, which reads memory a.k.a. the DESTINATION-stage.
		 *										There, the memory of the source stage shall be available (i.e. transferred into L1)
		 */
		static constexpr memory_barrier create_write_after_write(memory_access aMemoryToMakeVisible, memory_access aMemoryToMakeAvailable, const image_t& aImage);

		// TODO: Buffer barriers after buffer data types have been unified
		
	private:
		static constexpr vk::AccessFlags get_write_flag_for_memory_stage(memory_access aMemoryStage);
		static constexpr vk::AccessFlags get_read_flag_for_memory_stage(memory_access aMemoryStage);
		
		std::variant<vk::MemoryBarrier, vk::BufferMemoryBarrier, vk::ImageMemoryBarrier> mBarrier;
	};
}

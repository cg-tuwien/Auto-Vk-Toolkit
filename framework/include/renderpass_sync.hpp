#pragma once

namespace cgb
{
	/**	Config struct for specifying sync parameters during renderpass creation.
	 *
	 *	When you receive such a sync-struct in a callback, it will be pre-filled
	 *	with some pipeline stages and access flags. Change them according to the
	 *	requirements of your particular application!
	 */
	struct renderpass_sync
	{
		/** Value representing an external dependency to/from a render pass */
		static const int sExternal = -1;

		renderpass_sync(int aSrcPass, int aDstPass)
			: mSrcPass{aSrcPass}, mDstPass{aDstPass}, mSourceStage{pipeline_stage::top_of_pipe}, mSourceMemoryDependency{}, mDestinationStage{pipeline_stage::bottom_of_pipe}, mDestinationMemoryDependency{} {}
		renderpass_sync(int aSrcPass, int aDstPass, cgb::pipeline_stage aSrcStage, std::optional<cgb::write_memory_access> aSrcAccess, cgb::pipeline_stage aDstStage, std::optional<cgb::memory_access> aDstAccess)
			: mSrcPass{aSrcPass}, mDstPass{aDstPass}, mSourceStage{aSrcStage}, mSourceMemoryDependency{aSrcAccess}, mDestinationStage{aDstStage}, mDestinationMemoryDependency{aDstAccess} {}
		bool is_external_pre_sync() const { return sExternal == mSrcPass; }
		bool is_external_post_sync() const { return sExternal == mDstPass; }
		bool is_intra_subpass_sync() const { return !is_external_pre_sync() && !is_external_post_sync(); }
		int source_subpass_id() const { return mSrcPass; }
		int destination_subpass_id() const { return mDstPass; }

		/** The (previous) stages that must have completed execution. */
		cgb::pipeline_stage mSourceStage;

		/** The memory to be made available from the source stages. */
		std::optional<cgb::write_memory_access> mSourceMemoryDependency;

		/** The (subsequent) stages that have to wait upon completion of source stages. */
		cgb::pipeline_stage mDestinationStage;
		
		/** The memory/caches to be made visible before continuing execution.
		 *	This can be both, read and write access for "read after write" sync,
		 *	or "write after write" sync, respectively.
		 */
		std::optional<cgb::memory_access> mDestinationMemoryDependency;

	private:
		int mSrcPass;
		int mDstPass;
	};
}

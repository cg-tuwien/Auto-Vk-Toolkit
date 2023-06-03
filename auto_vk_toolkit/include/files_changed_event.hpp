#pragma once

#include <FileWatcher/FileWatcher.h>

#include "event.hpp"

namespace avk
{
	/** This event occurs when any of the files watched by this instance
	 *	has been modified on the file system.
	 */
	class files_changed_event : public event
	{
		class file_events_handler : public FW::FileWatchListener
		{
		public:
			~file_events_handler();
			
			void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action) override;
			void add_directory_watch(std::string aDirectory);
			bool is_directory_already_watched(std::string aDirectory) const;
			void prepare_for_new_update();
			size_t number_of_watch_ids() const { return mWatchIds.size(); }
			bool was_any_file_modified_during_last_update(const std::unordered_map<std::string, std::unordered_set<std::string>>& aDirAndFiles) const;
			
		private:
			std::unordered_set<std::string> mWatchedDirectories;
			std::vector<FW::WatchID> mWatchIds;
			std::unordered_map<std::string, std::unordered_set<std::string>> mDirectoriesAndFilesModifiedInLastUpdate;
		};
		
	public:
		files_changed_event(std::vector<std::string> aPathsToWatch);
		
		files_changed_event(files_changed_event&&) noexcept = default;
		files_changed_event(const files_changed_event&) = default;
		files_changed_event& operator=(files_changed_event&&) noexcept = default;
		files_changed_event& operator=(const files_changed_event&) = default;
		~files_changed_event() = default;

		bool update(event_data& aData) override;
		static void update();

		const auto& watched_directories_and_files() const { return mUniqueDirectoriesToFiles; }

	private:
		// This behaves like a singleton -.- Meaning that an update()-invocation causes all handlers
		// to run IMMEDIATELY if there are events. This is a bit of a pain in the backside.
		// It will work fine if no other FileWatchers are used in Auto-Vk-Toolkit.
		// If there are, the handlers could be invoked at unexpected times. So, beware!
		static FW::FileWatcher sFileWatcher;

		static file_events_handler sFileEventsHandler;
		
		std::unordered_map<std::string, std::unordered_set<std::string>> mUniqueDirectoriesToFiles;
	};

	extern bool operator==(const files_changed_event& left, const files_changed_event& right);

	extern bool operator!=(const files_changed_event& left, const files_changed_event& right);

	extern files_changed_event shader_files_changed_event(const avk::graphics_pipeline_t& aPipeline);

	extern files_changed_event shader_files_changed_event(const avk::compute_pipeline_t& aPipeline);

	extern files_changed_event shader_files_changed_event(const avk::ray_tracing_pipeline_t& aPipeline);
}

namespace std // Inject hash for `files_changed_event` into std::
{
	template<> struct hash<avk::files_changed_event>
	{
		std::size_t operator()(avk::files_changed_event const& o) const noexcept
		{
			std::vector<std::string> allDirectories;
			for (const auto& dir : o.watched_directories_and_files()) {
				allDirectories.push_back(dir.first);
			}
			std::sort(std::begin(allDirectories), std::end(allDirectories));
			
			std::size_t h = 0;
			avk::hash_combine(h, std::accumulate(std::begin(allDirectories), std::end(allDirectories), std::string{}));
			return h;
		}
	};
}

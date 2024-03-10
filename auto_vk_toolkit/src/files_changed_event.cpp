#include "files_changed_event.hpp"

namespace avk
{
	FW::FileWatcher files_changed_event::sFileWatcher{};
	files_changed_event::file_events_handler files_changed_event::sFileEventsHandler{};
	
	files_changed_event::file_events_handler::~file_events_handler()
	{
		for (const auto& watchId : mWatchIds) {
			sFileWatcher.removeWatch(watchId);
		}
		mWatchIds.clear();
	}
	
	void files_changed_event::file_events_handler::handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action)
	{
		if (FW::Actions::Modified == action) {
			LOG_INFO(std::format("File '{}' in directory '{}' has been modified ({}#{})", filename, dir, reinterpret_cast<intptr_t>(this), watchid));

			if (!mDirectoriesAndFilesModifiedInLastUpdate.contains(dir)) {
				mDirectoriesAndFilesModifiedInLastUpdate.insert({dir, {}});
			}
			mDirectoriesAndFilesModifiedInLastUpdate[dir].insert(filename);
		}
	}

	void files_changed_event::file_events_handler::add_directory_watch(std::string aDirectory)
	{
		mWatchIds.push_back(sFileWatcher.addWatch(aDirectory, this));
		mWatchedDirectories.insert(aDirectory);
	}
	
	bool files_changed_event::file_events_handler::is_directory_already_watched(std::string aDirectory) const
	{
		return mWatchedDirectories.contains(aDirectory);
	}

	void files_changed_event::file_events_handler::prepare_for_new_update()
	{
		mDirectoriesAndFilesModifiedInLastUpdate.clear();
	}

	bool files_changed_event::file_events_handler::was_any_file_modified_during_last_update(const std::unordered_map<std::string, std::unordered_set<std::string>>& aDirAndFiles) const
	{
		if (mDirectoriesAndFilesModifiedInLastUpdate.empty()) {
			return false;
		}
		
		for (const auto& pair : aDirAndFiles) {
			if (mDirectoriesAndFilesModifiedInLastUpdate.contains(pair.first)) {
				const auto& dir = mDirectoriesAndFilesModifiedInLastUpdate.at(pair.first);
				for (const auto& file : pair.second) {
					if (dir.contains(file)) {
						return true;
					}
				}
			}
		}
		
		return false;
	}
	
	files_changed_event::files_changed_event(std::vector<std::string> aPathsToWatch)
	{
		// File watcher operates on directories, not files => get all unique directories
		for (const auto& file : aPathsToWatch)
		{
			auto directory = avk::extract_base_path(file);
			auto filename = avk::extract_file_name(file);
			auto mapResult = mUniqueDirectoriesToFiles.insert({directory, {}});
			auto setResult = mapResult.first->second.insert(filename);
			auto alreadyWatched = sFileEventsHandler.is_directory_already_watched(directory);
			LOG_DEBUG(std::format("Watching ({}) file[{}] in ({}) directory[{}] ({} to FileEventsHandler)", setResult.second ? "new" : "known", filename, mapResult.second ? "new" : "known", directory, alreadyWatched ? "known" : "new"));
			if (!alreadyWatched) {
				sFileEventsHandler.add_directory_watch(directory);
			}
		}	
	}

	bool files_changed_event::update(event_data& aData)
	{
		return sFileEventsHandler.was_any_file_modified_during_last_update(mUniqueDirectoriesToFiles);
	}

	void files_changed_event::update()
	{
		if (sFileEventsHandler.number_of_watch_ids() > 0) {
			sFileEventsHandler.prepare_for_new_update();
			sFileWatcher.update();
		}
	}

	bool operator==(const files_changed_event& left, const files_changed_event& right)
	{
		const auto& ld = left.watched_directories_and_files();
		const auto& rd = right.watched_directories_and_files();

		if (ld.size() != rd.size()) {
			return false;
		}

		for (const auto& dir : ld) {
			if (!rd.contains(dir.first)) {
				return false;
			}
			const auto& lf = dir.second;
			const auto& rf = rd.at(dir.first);
			if (lf.size() != rf.size()) {
				return false;
			}
			for (const auto& f : lf) {
				if (!rf.contains(f)) {
					return false;
				}
			}
		}

		return true;
	}

	bool operator!=(const files_changed_event& left, const files_changed_event& right)
	{
		return !(left == right);
	}

	files_changed_event shader_files_changed_event(const avk::graphics_pipeline_t& aPipeline)
	{
		std::vector<std::string> paths;
		for (auto& s : aPipeline.shaders()) {
			paths.push_back(s.actual_load_path());
		}
		return files_changed_event(std::move(paths));
	}

	files_changed_event shader_files_changed_event(const avk::compute_pipeline_t& aPipeline)
	{
		return files_changed_event({aPipeline.get_shader().actual_load_path()});
	}

	files_changed_event shader_files_changed_event(const avk::ray_tracing_pipeline_t& aPipeline)
	{
		std::vector<std::string> paths;
		for (auto& s : aPipeline.shaders()) {
			paths.push_back(s.actual_load_path());
		}
		return files_changed_event(std::move(paths));
	}
}

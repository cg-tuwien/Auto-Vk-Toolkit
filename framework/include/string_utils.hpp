#pragma once

namespace cgb
{

	static std::string trim_spaces(std::string_view s)
	{
		auto pos1 = s.find_first_not_of(' ');
		auto pos2 = s.find_last_not_of(' ');
		return std::string(s.substr(pos1, pos2-pos1+1));
	}

#ifdef USE_BACKSPACES_FOR_PATHS
	const char SEP_TO_USE = '\\';
	const char SEP_NOT_TO_USE = '/';
#else
	const char SEP_TO_USE = '/';
	const char SEP_NOT_TO_USE = '\\';
#endif

	static std::string clean_up_path(std::string_view path)
	{
		auto cleaned_up = trim_spaces(path);
		int consecutive_sep_cnt = 0;
		for (int i = 0; i < cleaned_up.size(); ++i) {
			if (cleaned_up[i] == SEP_NOT_TO_USE) {
				cleaned_up[i] = SEP_TO_USE;
			}
			if (cleaned_up[i] == SEP_TO_USE) {
				consecutive_sep_cnt += 1;
			}
			else {
				consecutive_sep_cnt = 0;
			}
			if (consecutive_sep_cnt > 1) {
				cleaned_up = cleaned_up.substr(0, i) + (i < cleaned_up.size() - 1 ? cleaned_up.substr(i + 1) : "");
				consecutive_sep_cnt -= 1;
				--i;
			}
		}
		return cleaned_up;
	}

	static std::string extract_file_name(std::string_view path)
	{
		auto cleaned_path = clean_up_path(path);
		auto last_sep_idx = cleaned_path.find_last_of(SEP_TO_USE);
		if (std::string::npos == last_sep_idx) {
			return cleaned_path;
		}
		return cleaned_path.substr(last_sep_idx + 1);
	}

	static std::string extract_base_path(std::string_view path)
	{
		auto cleaned_path = clean_up_path(path);
		auto last_sep_idx = cleaned_path.find_last_of(SEP_TO_USE);
		if (std::string::npos == last_sep_idx) {
			return cleaned_path;
		}
		return cleaned_path.substr(0, last_sep_idx + 1);
	}

	static std::string combine_paths(std::string_view first, std::string_view second)
	{
		return clean_up_path(std::string(first) + SEP_TO_USE + std::string(second));
	}

}
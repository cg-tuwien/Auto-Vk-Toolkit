#pragma once

namespace cgb
{
	/**	Make an array by only specifying its type and having its size automatically set.
	 *	Source: https://stackoverflow.com/questions/54125515/partial-template-argument-deduction-or-workaround-for-stdarray
	 */
	template<typename Type, typename ... T>
	constexpr auto make_array(T&&... t) -> std::array<Type, sizeof...(T)>
	{
		return { {std::forward<T>(t)...} };
	}

	template<class T> struct identity { using type = T; };
	template<class D, class... Ts>
	struct ret : identity<D> {};
	template<class... Ts>
	struct ret<void, Ts...> : std::common_type<Ts...> {};
	template<class D, class... Ts>
	using ret_t = typename ret<D, Ts...>::type;

	/** Make a vector.
	 *	Source: https://stackoverflow.com/questions/36994727/how-do-i-write-a-make-vector-similar-to-stdmake-tuple
	 */
	template<class D = void, class... Ts>
	std::vector<ret_t<D, Ts...>> make_vector(Ts&&... args) {
		std::vector<ret_t<D, Ts...>>  ret;
		ret.reserve(sizeof...(args));
		using expander = int[];
		(void) expander{ ((void)ret.emplace_back(std::forward<Ts>(args)), 0)..., 0 };
		return ret;
	}

	/** Makes a Vulkan-compatible version integer based on the three given numbers */
	static constexpr uint32_t make_version(uint32_t major, uint32_t minor, uint32_t patch)
	{
		return (((major) << 22) | ((minor) << 12) | (patch));
	}

	/** Find Case Insensitive Sub String in a given substring 
	 *  Credits: https://thispointer.com/implementing-a-case-insensitive-stringfind-in-c/
	 */
	static size_t find_case_insensitive(std::string data, std::string toFind, size_t startingPos)
	{
		// Convert complete given String to lower case
		std::transform(data.begin(), data.end(), data.begin(), ::tolower);
		// Convert complete given Sub String to lower case
		std::transform(toFind.begin(), toFind.end(), toFind.begin(), ::tolower);
		// Find sub string in given string
		return data.find(toFind, startingPos);
	}

	/**	Load a binary file into memory.
	 *	Adapted from: http://www.cplusplus.com/reference/istream/istream/read/
	 */
	static std::vector<char> load_binary_file(std::string path)
	{
		std::vector<char> buffer;
		std::ifstream is(path.c_str(), std::ifstream::binary);
		if (!is) {
			throw std::runtime_error(fmt::format("Couldn't load file '{}'", path));
		}

		// get length of file:
		is.seekg(0, is.end);
		size_t length = is.tellg();
		is.seekg(0, is.beg);

		buffer.resize(length);

		// read data as a block:
		is.read(buffer.data(), length);

		if (!is) {
			is.close();
			LOG_ERROR(fmt::format("cgb::load_binary_file could only read {} bytes instead of {}", is.gcount(), length));
			throw std::runtime_error(fmt::format("Couldn't read file '{}' into buffer.", path));
		}

		is.close();
		return buffer;
	}
}

#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <filesystem>

class lime::string;

inline lime::string operator+(const char *raw_str, const lime::string& lime_string) noexcept { return lime_string.insert(0, raw_str); }
inline lime::string operator+(const char *raw_str_1, const char *raw_str_2)         noexcept { return lime::string(raw_str_1) + raw_str_2; }
inline lime::string operator+(char character, const lime::string& lime_string)      noexcept { return lime_string.insert(0, character); }

// NOTE: STANDARD FOR THIS FUNCTION: Won't resolve symlinks (including . & ..).
inline lime::string operator/(const char *raw_str, const lime::string& lime_string) noexcept {
	return lime::string((std::filesystem::path(raw_str) / std::filesystem::path(*(const std::string*)&lime_string)).c_str());
}
inline lime::string operator/(const char *raw_str_1, const char *raw_str_2) noexcept {
	return lime::string((std::filesystem::path(raw_str_1) / std::filesystem::path(raw_str_2)).c_str());
}

namespace lime {

	// NOTE: Private inheritance, on purpose, don't change.
	class string : std::string {
		template <typename T>
		std::vector<lime::string> inner_split(T delimiter, size_t delimiter_length) noexcept {
			std::vector<lime::string> result;

			size_t last_delimiter_index = 0;
			size_t next_delimiter_index = 0;
			while ((next_delimiter_index = find(delimiter, next_delimiter_index)) != std::string::npos) {
				result.push_back(substr(last_delimiter_index + delimiter_length, next_delimiter_index - (last_delimiter_index + delimiter_length)));
				last_delimiter_index = next_delimiter_index;
			}

			result.push_back(substr(last_delimiter_index + delimiter_length));

			return result;
		}

	public:
		static constexpr size_t npos = std::string::npos;

		lime::string(const char *raw_str) noexcept : std::string(raw_str) { }

		lime::string(const std::string& std_string) noexcept : std::string(std_string) { }
		lime::string(std::string&& std_string)      noexcept : std::string(std::move(std_string)) { }

		lime::string operator+(const lime::string& right) const noexcept { return lime::string(std::string::operator+(*(const std::string*)&right)); }
		lime::string& operator+=(const lime::string& right) noexcept { std::string::operator+=(*(const std::string*)&right); return *this; }
		lime::string operator+(const char *raw_str) const noexcept { return lime::string(std::string::operator+(raw_str)); }
		lime::string& operator+=(const char *raw_str) noexcept { std::string::operator+=(raw_str); return *this; }

		lime::string operator/(const lime::string& right) const noexcept {
			return lime::string((std::filesystem::path(*(const std::string*)this) / std::filesystem::path(*(const lime::string*)&right)).c_str());
		}
		// TODO: Would returning void here be UB? I would think not, but if it isn't, it's harder to rely on this behavior for other classes right?
		lime::string& operator/=(const lime::string& right) noexcept {
			return (*this = operator/(right));
		}
		lime::string operator/(const char *raw_str) const noexcept {
			return lime::string((std::filesystem::path(*(const std::string*)this) / std::filesystem::path(raw_str)).c_str());
		}
		lime::string& operator/=(const char *raw_str) noexcept {
			return (*this = operator/(raw_str));
		}

		// NOTE: These just bring a select few functions, which are marked private because of the above private inheritance, into the public "space".
		using std::string::c_str();
		using std::string::length();

		size_t find(char character, size_t start_position)             const noexcept { return std::string::find(character, start_position); }
		size_t find(char character)                                    const noexcept { return std::string::find(character); }
		size_t find(const char *string, size_t start_position)         const noexcept { return std::string::find(string, start_position); }
		size_t find(const char *string)                                const noexcept { return std::string::find(string); }
		size_t find(const lime::string& string, size_t start_position) const noexcept { return std::string::find(string.c_str(), start_position); }
		size_t find(const lime::string& string)                        const noexcept { return std::string::find(string.c_str()); }

		lime::string substr(size_t index, size_t length) const noexcept {
			if (index >= length() || length > length() - index) { lime::error("substr(index, length) was called with out-of-bounds arguments"); exit_program(); }
			return lime::string(std::string::substr(index, length));
		}
		lime::string substr(size_t index) const noexcept {
			if (index >= length()) { lime::error("substr(index) was called with out-of-bounds arguments"); exit_program(); }
			return lime::string(std::string::substr(index));
		}

		std::vector<lime::string> split(const char *delimiter)         const noexcept { return inner_split(delimiter, std::strlen(delimiter)); }
		std::vector<lime::string> split(char delimiter)                const noexcept { return inner_split(delimiter, 1); }
		std::vector<lime::string> split(const lime::string& delimiter) const noexcept { return inner_split(delimiter.c_str(), delimiter.length()); }

		lime::string insert(size_t index, const char *string) const noexcept {
			if (index >= length()) { lime::error("insert(index, string) was called with out-of-bounds arguments"); exit_program(); }
			lime::string result = *this;
			result.std::string::insert(index, string);
			return result;
		}
		lime::string insert(size_t index, char character) const noexcept {
			if (index >= length()) { lime::error("insert(index, character) was called with out-of-bounds arguments"); exit_program(); }
			lime::string result = *this;
			result.std::string::insert(index, &character, 1);
			return result;
		}
		lime::string insert(size_t index, const lime::string& string) const noexcept {
			if (index >= length()) { lime::error("insert(index, string) was called with out-of-bounds arguments"); exit_program(); }
			lime::string result = *this;
			result.std::string::insert(index, string.c_str(), string.length());
			return result;
		}

		lime::string get_parent_folder() const noexcept {
			std::filesystem::path path;
			try {
				path = std::filesystem::absolute(std::filesystem::path(*(const std::string*)this));
				return path.parent_path().c_str();	// TODO: Convert in better way that doesn't require remeasuring the string.
			}
			catch (const std::filesystem::filesystem_error& exception) {
				// TODO: Replace std::filesystem, see below.
				lime::error("get_parent_folder() failed: " + exception.what());
				exit_program();
			}
		}

		lime::string get_relative_path(const lime::string& base) const noexcept {
			try {
				std::filesystem::path base_path(*(const std::string*)&base);
				std::filesystem::path this_path(*(const std::string*)this);
				return std::filesystem::relative(this_path, base_path);
			}
			catch (const std::filesystem::filesystem_error& exception) {
				// TODO: Replace std::filesystem, it's a half-baked solution. The documentation is shit.
				// For example, what exception does the thing throw if allocation fails, bad_alloc? Or does it throw one of these with a specific error code? Which error code? Only god knows.
				lime::error("get_relative_path(base) failed: " + exception.what());
				exit_program();
			}
		}
	};

	template <typename functor_t>
		// TODO: Use concepts.
	bool call_if_self_rebuild_necessary(const lime::string& src_file_path, functor_t functor) noexcept {
		try {
			std::filesystem::path self_exe_path("/proc/self/exe");
			std::filesystem::path source_file_path = std::weakly_canonical(std::filesystem::path(*(const std::string*)&src_file_path));
			if (std::filesystem::last_write_time(self_exe_path) < std::filesystem::last_write_time(source_file_path)) {
				lime::info("self rebuild necessary, calling self rebuild function...");
				functor();
				lime::info("self rebuild finished");
				return true;
			}
			return false;
		}
		catch (const std::filesystem::filesystem_error& exception) {
			// TODO: Replace std::filesystem, see above.
			lime::error("call_if_self_rebuild_necessary(src_file_path, functor) failed: " + exception.what());
			exit_program();
		}
	}

	template <typename functor_t>
	bool call_if_out_of_date(const lime::string& path, std::vector<lime::string> deps, functor_t functor) noexcept {
		try {
			std::filesystem::path filesystem_path = std::weakly_canonical(std::filesystem::path(*(const std::string*)&path));
			for (const lime::string& dep : deps) {
				std::filesystem::path dep_path = std::weakly_canonical(std::filesystem::path(*(const std::string*)&dep));
				if (std::filesystem::last_write_time(filesystem_path) < std::filesystem::last_write_time(dep_path)) {
					lime::info('\"' + path + '\"' + " is out-of-date, calling remedial function...");
					functor();
					lime::info('\"' + path + '\"' + " remedied");
					return true;
				}
			}
			return false;
		}
		catch (const std::filesystem::filesystem_error& exception) {
			// TODO: Replace std::filesystem, see above.
			lime::error("call_if_self_rebuild_necessary(src_file_path, functor) failed: " + exception.what());
			exit_program();
		}
	}

	enum class inner_execute_command_return_t : uint8_t {
		SUCCESS,
		COMMAND_FAILED,
		INVOKE_FAILED
	}

	inline inner_execute_command_return_t inner_execute_command(const lime::string& cmdline) noexcept {
		pid_t vfork_result = vfork();

		std::vector<lime::string> final_args;

		std::vector<lime::string> quote_separated = cmdline.split('\"');

		final_args = quote_separated[0].split(' ');
		// TODO: Finish this up.
		// In general, check through these latest functions to make sure you didn't mess something up because you're tired. Check the docs again as well.

		for (size_t i = 1; i < quote_separated.size(); i++) {

		}

		// CHILD
		if (vfork_result == 0) {
			if (exevp() == -1) { return inner_execute_command_return_t::INVOKE_FAILED; }
		}
		
		// PARENT
		if (vfork_result == -1) { return inner_execute_command_return_t::INVOKE_FAILED; }

		int wstatus;
		if (wait(&wstatus) == -1) { return inner_execute_command_return_t::INVOKE_FAILED; }
		if (WIFEXITED(wstatus)) {
			if (WEXITSTATUS() != EXIT_SUCCESS) { return inner_execute_command_return_t::COMMAND_FAILED; }
		}
		if (WIFSIGNALED(wstatus)) { return inner_execute_command_return_t::COMMAND_FAILED; }
		return inner_execute_command_return_t::SUCCESS;
	}

	inline void exec(const lime::string& cmdline) noexcept {
		lime::cmd_label(cmdline);
		switch (inner_execute_command(cmdline)) {
		case inner_execute_command_return_t::SUCCESS: break;
		case inner_execute_command_return_t::COMMAND_FAILED: lime::error("exec(cmdline) failed, invoked command failed"); exit_program();
		case inner_execute_command_return_t::INVOKE_FAILED: lime::error("exec(cmdline) failed because of unsuccessful invocation"); exit_program();
		}
	}

	template <typename functor_t>
	bool for_each_arg(unsigned int argc, const char * const *argv, functor_t functor) noexcept {
		if (argc == 1) { return false; }
		for (unsigned int i = 1; i < argc; i++) { functor(argv[i]); }
		return true;
	}

	template <char... matcher_specification, typename... functor_types>
	bool string_match(const lime::string& arg, functor_types... match_actions) noexcept {
		// TODO: implement
	}

	inline std::vector<lime::string> enum_files(const lime::string& target_dir, const lime::string& query) noexcept {
		// TODO: implement
	}

	inline std::vector<lime::string> enum_files_recursive(const lime::string& target_dir, const lime::string& query) noexcept {
		// TODO: implement
	}

	inline void create_path(const lime::string& path) noexcept {
		// TODO: implement
	}

	inline void error(const lime::string& message) noexcept {
		fflush(stdout);
		lime::string final_message = "[ERROR]: " + message;
		write(STDOUT_FILENO, final_message.c_str(), final_message.length() * sizeof(char));
	}

	inline void warn(const lime::string& message) noexcept {
		lime::string final_message = "[WARNING]: " + message;
		fwrite(final_message.c_str(), sizeof(char), final_message.length(), stdout);
	}

	inline void info(const lime::string& message) noexcept {
		lime::string final_message = "[INFO]: " + message;
		fwrite(final_message.c_str(), sizeof(char), final_message.length(), stdout);
	}

	inline void cmd_label(const lime::string& message) noexcept {
		lime::string final_message = "[CMD]: " + message;
		fwrite(final_message.c_str(), sizeof(char), final_message.length(), stdout);
	}

}

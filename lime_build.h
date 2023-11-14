#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <utility>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>

class lime::string;
class lime::string::path;

inline lime::string operator+(const char *raw_str, const lime::string& lime_string) noexcept { return lime_string.insert(0, raw_str); }
inline lime::string operator+(const char *raw_str_1, const char *raw_str_2)         noexcept { return lime::string(raw_str_1) + raw_str_2; }
inline lime::string operator+(char character, const lime::string& lime_string)      noexcept { return lime_string.insert(0, character); }

// NOTE: STANDARD FOR THESE FUNCTIONS: Won't resolve symlinks (including . & ..).
inline lime::string operator/(const char *raw_str, const lime::string& lime_string) noexcept {
	return lime::string((lime::string::path(raw_str) / lime::string::path(lime_string));
}
inline lime::string operator/(const char *raw_str_1, const char *raw_str_2) noexcept {
	return lime::string(lime::string::path(raw_str_1) / lime::string::path(raw_str_2));
}

#define LAST_ELEMENT(container) (*(container.end() - 1))

namespace lime {

	class lime::string;

	class string : private std::string {
		class path {
			void parse_inner(std::vector<std::string> &result, const char &character) const noexcept {
				switch (character) {
				case '\\':
				case '/':
					result.push_back();
					continue;
				}
				LAST_ELEMENT(result) += character;
			}

			std::vector<std::string> parse(const lime::string &input) const noexcept {
				std::vector<std::string> result;
				result.push_back();
				for (const char &character : input) { parse_inner(result, character); }
				return result;
			}

			std::vector<std::string> parse(const char *input) const noexcept {
				std::vector<std::string> result;
				result.push_back();
				for (; input != '\0'; input++) { parse_inner(result, character); }
				return result;
			}

			path concatinate(const path &right) const noexcept {
				path result = *this;
				// NOTE: If it's a directory, remove the trailing empty element.
				if (LAST_ELEMENT(*this) == "") { heirarchy.pop_back(); }
				for (const std::string &element : right) { result.heirarchy.push_back(element); }
				return result;
			}

		public:
			std::vector<std::string> heirarchy;

			path(const char *input) noexcept { heirarchy = parse(input); }
			path(const lime::string &input) noexcept { heirarchy = parse(input); }

			path operator/(const path &right) noexcept { return this->concatinate(right); }

			path to_absolute() const noexcept {
				char cwd[PATH_MAX + 1];
				if (getcwd(cwd, sizeof(cwd)) == nullptr) {
					lime::error("lime::string::path::to_absolute() failed");
					exit_program();
				}
				return path(cwd) / *this;
			}

			path get_parent_folder() const noexcept {
				path result = *this;
				if (*(heirarchy.end() - 1) == "") {
					result.heirarchy.erase(heirarchy.end() - 2);
					return result;
				}
				*(result.heirarchy.end() - 1) = "";
				return result;
			}

			bool is_directory() const noexcept { return *(heirarchy.end() - 1) == ""; }

			path get_relative_path(const path &base_path) const noexcept {
				if (base_path.is_directory() == false) {
					lime::error("lime::string::path::get_relative_path called with non-directory base_path");
				}

				const path absolute_base_path = base.to_absolute();
				const path absolute_this_path = this->to_absolute();

				const_iterator absolute_base_path_ptr = absolute_base_path.begin();
				const_iterator absolute_this_path_ptr = absolute_this_path.begin();
				for (; *absolute_base_path_ptr == *absolute_this_path_ptr
				       && absolute_base_path_ptr < absolute_base_path.end()
				       && absolute_this_path_ptr < absolute_this_path.end();
					absolute_base_path_ptr++, absolute_this_path_ptr++)
				{ }

				path result;
				for (; absolute_this_path_ptr < absolute_this_path.end(); absolute_this_path_ptr++) {
					result.heirarchy.push_back(*absolute_this_path_ptr);
				}
				if (result.heirarchy.size() == 0) {
					result.heirarchy.push_back(".");
					result.heirarchy.push_back("");
				}
				return result;
			}

			std::chrono::time_point get_last_modification_time() const noexcept {
				struct stat stat_buf;
				stat(this->to_std_string().c_str(), &state_buf);
				return std::chrono::time_point(std::chrono::milliseconds(stat_buf.st_mtime));
			}

			using const_iterator = const std::string*;
			using iterator = std::string*;

			const_iterator begin() const noexcept { return heirarchy.begin(); }
			iterator begin() noexcept { return heirarchy.begin(); }
			const_iterator end() const noexcept { return heirarchy.end(); }
			iterator end() noexcept { return heirarchy.end(); }

			path& operator=(const path &right) noexcept {
				heirarchy = right.heirarchy;
				return *this;
			}

			path& operator=(path &&right) noexcept {
				heirarchy = std::move(right.heirarchy);
				return *this;
			}

			std::string to_std_string() const noexcept { 
				std::string result;
				for (size_t i = 0; i < heirarchy.size() - 1; i++) {
					const std::string &element = heirarchy[i];
					result.push_back(element + '/');
				}
				result.push_back(*(heirarchy.end() - 1));
				return result;
			}
		};

		lime::string(const lime::path &path) noexcept : std::string(path.to_std_string()) { }

		template <typename T>
		std::vector<lime::string> inner_split(T delimiter, size_t delimiter_length) noexcept {
			std::vector<lime::string> result;

			size_t last_delimiter_index = 0;
			size_t next_delimiter_index = 0;
			while ((next_delimiter_index = find(delimiter, next_delimiter_index)) != std::string::npos) {
				result.push_back(substr(last_delimiter_index + delimiter_length,
							next_delimiter_index - (last_delimiter_index + delimiter_length)));
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
			return lime::string(path(*(const std::string*)this) / path(*(const lime::string*)&right));
		}
		lime::string& operator/=(const lime::string& right) noexcept {
			return (*this = operator/(right));
		}
		lime::string operator/(const char *raw_str) const noexcept {
			return lime::string(path(*(const std::string*)this) / path(raw_str));
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
			path absolute_path = path(*(const std::string*)this).to_absolute();
			return path.get_parent_folder();
		}

		lime::string get_relative_path(const lime::string& base) const noexcept {
			path base_path(base);
			path this_path(*this);
			return this_path.get_relative_path(base_path);
		}

		std::chrono::time_point get_last_modification_time() const noexcept {
			return path(*this).get_last_modification_time();
		}
	};

	template <typename functor_t>
	bool call_if_self_rebuild_necessary(const lime::string& src_file_path_string, functor_t functor) noexcept {
		lime::string::path self_exe_path("/proc/self/exe");
		lime::string::path src_file_path(src_file_path_string);
		if (self_exe_path.get_last_modification_time() < source_file_path.get_last_modification_time()) {
			lime::info("self rebuild necessary, calling self rebuild function...");
			functor();
			lime::info("self rebuild finished");
			return true;
		}
		return false;
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

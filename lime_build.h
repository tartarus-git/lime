#pragma once

// TODO: Sort through these, remove unnecessary ones if they exist.
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <utility>
#include <climits>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <glob.h>
#include <chrono>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>

namespace lime { class string; }

// NOTE: These are outside of the namespace because obviously you need to be able to use them
// from normal code, which is located outside of the namespace.
lime::string operator+(const char *raw_str, const lime::string &lime_string) noexcept;
lime::string operator+(char character, const lime::string& lime_string)      noexcept;

lime::string operator/(const char *raw_str, const lime::string& lime_string) noexcept;

namespace lime {

	enum class error_t : short {
		SUCCESS,
		ERRNO,
		PATH_ABSOLUTE
	}

	[[noreturn]] inline void exit_program(int exit_code) noexcept {
		std::exit(exit_code);
	}

	// NOTE: The way these channels work is pretty much obvious, except for the difference between error and bug.
	// Error is user error, bug is when library bug is detected by the library itself.
	// Often, the pattern is as follows: User-facing function checks condition on input values, if wrong, output error,
	// if not, call sub-function to handle inputs in some way. Sub-function checks again, and if the condition fails,
	// it must be because the library called this function with the wrong inputs, because the sub-function isn't user-facing.
	// That's a bug, so report it as such.
	// Essentially, as the function depth increases, errors will often turn into bugs.
	void error(const lime::string &message)		noexcept;
	void warn(const lime::string &message)		noexcept;
	void info(const lime::string &message)		noexcept;
	void cmd_label(const lime::string &message)	noexcept;
	void bug(const lime::string &message)		noexcept;

	// NOTE: These functions are needed within lime::string, so that's why we've pulled
	// the declarations up here.
	lime::string pwd()		       noexcept;
	void cd(lime::string target_directory) noexcept;

	class string : private std::string {

		// NOTE: Path handling. lime::strings don't handle paths themselves, they always construct a path
		// out of themselves and ask it for path handling.

		// NOTE: About the way we handle directories:
		// We don't denote directories in any special ways, they're files as far as the paths are concerned.
		// There's no trailing slash that's enforced or anything, although that is always accepted.
		// When it comes down to whether or not something is a directory, we just ask the filesystem.
		class path {

			// NOTE: The way we do path parsing is dead-simple. Split along the slashes.
			void parse_inner(std::vector<std::string> &result, const char &character) const noexcept {
				switch (character) {
				case '\\':
				case '/':
					result.push_back(std::string());
					break;
				default: 
					*(result.end() - 1) += character;
					break;
				}
			}

			std::vector<std::string> parse(const lime::string &input) const noexcept {
				std::vector<std::string> result;
				result.push_back(std::string());
				for (const char &character : input) { parse_inner(result, character); }
				return result;
			}

			std::vector<std::string> parse(const char *input) const noexcept {
				std::vector<std::string> result;
				result.push_back(std::string());
				for (; *input != '\0'; input++) { parse_inner(result, *input); }
				return result;
			}

			path concatinate(const path &right, error_t &error) const noexcept {
				error = error_t::SUCCESS;

				if (right.is_absolute()) {
					error = error_t::PATH_ABSOLUTE;
					return path();
				}

				if (heirarchy.size() == 0) { return right; }

				path result = *this;

				// NOTE: If there is a trailing slash in our path, account for it.
				//if (*(result.end() - 1) == "") { result.heirarchy.pop_back(); }
				// TODO: Make heirarchy strip this on creation.

				for (const std::string &element : right) { result.push_back(element); }

				return result;
			}

			path inner_to_absolute(error_t &error) const noexcept {
				error = error_t::SUCCESS;

				if (is_absolute()) { return *this; }

				char cwd[PATH_MAX + 1];	// NOTE: +1 because of trailing NUL, necessary says stackoverflow comment
				if (getcwd(cwd, sizeof(cwd)) == nullptr) {
					error = error_t::ERRNO;
					return path();
				}

				return path(cwd) / *this;
			}

			path inner_to_canonicalized_absolute(error_t &error) const noexcept {
				error = error_t::SUCCESS;

				path result;
				for (const std::string &element : *this) {
					result.push_back(element);

					while (true) {
						char buffer[PATH_MAX + 1];
						ssize_t bytes_read = readlink(result.to_std_string().c_str(), buffer, sizeof(buffer));

						if (bytes_read < 0) {
							switch (errno) {
							case EACCES: break;
							case EINVAL: break;

							default:
								error = error_t::ERRNO;
								return path();
							}
							break;
						}
						if (bytes_read == 0) {
							error = error_t::INVALID_SYMLINK;
							return path();
						}
						if (bytes_read > sizeof(buffer) - 1) {
							lime::bug("path::inner_to_canonicalized_absolute() failed, bytes_read too large, buffer overflow?");
							lime::exit_program(EXIT_FAILURE);
						}

						buffer[bytes_read] = '\0';
						result = path(buffer);
					}
				}
				return result;
			}

			path inner_get_parent_folder(error_t &error) const noexcept {
				error = error_t::SUCCESS;

				if (size() <= 1) {
					error = error_t::PATH_INVALID;
					return path();
				}

				path result = *this;

				if (*(heirarchy.end() - 1) == "") {
					if (heirarchy.size() <= 2) {
						// TODO: Make other bug error codes not return error code but just terminate and show bug.
						// NOTE: Already did this TODO, I don't know if I got all the spots. I probably did. Check again.
						lime::bug("path::inner_get_parent_folder() failed, last=\"\" and size() <= 2");
						lime::exit_program(EXIT_FAILURE);
					}

					result.heirarchy.erase(heirarchy.end() - 2);
					return result;
				}

				result.heirarchy.pop_back();

				return result;
			}

			path inner_get_relative_path(const path &base_path_original) const noexcept {
				//check_path_validity(base_path_original, "base_path_original", "path::get_relative_path");
				//check_this_validity("path::get_relative_path");

				const path base_path = base_path_original.to_canonicalized_absolute();
				const path this_path = this->to_canonicalized_absolute();
				// TODO: Line shouldn't be necessary, instead make sure that to_canonicalized_absolute checks for this and
				// validates it's output.
				//if (base_path.is_empty() || this_path.is_empty()) { return path(); }

				const_iterator base_path_ptr = base_path.begin();
				const_iterator this_path_ptr = this_path.begin();

				for (; *base_path_ptr == *this_path_ptr
				       && base_path_ptr < base_path.end();
				     base_path_ptr++, this_path_ptr++)
				{
					if (this_path_ptr == this_path.end() - 1) {

						if (base_path_ptr == base_path.end() - 1) { return { "." }; }

						lime::bug("path::get_relative_path failed, base_path more specific than this_path");
						lime::exit_program(1);

					}
				}

				path result;

				for (; this_path_ptr < this_path.end(); this_path_ptr++) {
					result.heirarchy.push_back(*this_path_ptr);
				}

				if (result.size() == 0) {
					lime::bug("path::get_relative_path failed, somehow result.size()=0");
					lime::exit_program(1);
				}

				return result;
			}

		public:
			std::vector<std::string> heirarchy;

			path() noexcept { }

			path(const path &other) noexcept { heirarchy = other.heirarchy; }

			path(const lime::string &input) noexcept { heirarchy = parse(input); }
			path(const char *input) noexcept { heirarchy = parse(input); }

			size_t size() const noexcept { return heirarchy.size(); }
			bool is_empty() const noexcept { return heirarchy.empty(); }
			void push_back(const std::string &input) noexcept { heirarchy.push_back(input); }

			path operator/(const path &right) const noexcept {
				error_t error;
				const path result = concatinate(right, error);

				switch (error) {
				case error_t::SUCCESS: break;

				case error_t::PATH_ABSOLUTE:
					lime::error("path::operator/(right) failed, right is absolute, not allowed");
					lime::exit_program(EXIT_FAILURE);
				default:
					lime::bug("path::operator/(right) failed, unknown error");
					lime::exit_program(EXIT_FAILURE);
				}

				return result;
			}

			// TODO: Every function and every way you can use to modify the heirarchy vector needs to check it's validity and throw
			// error if it's invalid.

			bool is_absolute() const noexcept {
				if (is_empty()) { return false; }
				return (*this)[0] == "";
			}

			path to_absolute() const noexcept {
				error_t error;
				const path result = inner_to_absolute(error);

				switch (error) {
				case error_t::SUCCESS: break;

				case error_t::ERRNO:
					lime::bug("path::to_absolute() failed, general failure");
					lime::exit_program(EXIT_FAILURE);
				default:
					lime::bug("path::to_absolute() failed, unknown failure");
					lime::exit_program(EXIT_FAILURE);
				}

				return result;
			}

			path to_canonicalized_absolute() const noexcept {
				error_t error;
				const path result = inner_to_canonicalized_absolute(error);

				switch (error) {
				case error_t::SUCCESS: break;

				case error_t::INVALID_SYMLINK:
					lime::error("path::to_canonicalized_absolute() failed, invalid symlink");
					lime::exit_program(EXIT_FAILURE);

				default:
					lime::bug("path::to_canonicalized_absolute() failed, unknown error");
					lime::exit_program(EXIT_FAILURE);
				}

				return result;
			}

			path get_parent_folder() const noexcept {
				error_t error;
				const path result = inner_get_parent_folder(&error);

				switch (error) {
				case error_t::SUCCESS: break;

				case error_t::PATH_INVALID:
					lime::error("path::get_parent_folder() failed, path doesn't have parent");
					lime::exit_program(EXIT_FAILURE);

				default:
					lime::bug("path::get_parent_folder() failed, unknown error");
					lime::exit_program(EXIT_FAILURE);
				}

				return result;
			}

			path get_relative_path(const path &base_path_original) const noexcept {
				// TODO: Finish implementing inner_get_relative_path and then implement this function.
			}

			path strip_trailing_slash() const noexcept {
				check_this_validity("path::strip_trailing_slash");

				path result = *this;
				// TODO: Think about the functions that path should have. Should it have it's own pop_back().
				// Should we just inherit from std::vector<std::string>?
				// Probably not, but think about it.
				if (*(result.end() - 1) == "") { result.heirarchy.pop_back(); }
				return result;
			}

			std::string get_filename() const noexcept {
				check_this_validity("path::get_filename");

				path normalized = this->strip_trailing_slash();
				return *(normalized.end() - 1);
			}

			// TODO: This function is useless. Remove it and it's fellow.
			// You gotta check the validity in the functions where you change the heirarchy, not in the functions where you read
			// the heirarchy. Makes more sense that way.
			void check_path_validity(const path &p, const lime::string &p_name, const lime::string &function_name) const noexcept {
				if (p.size() == 0) {
					lime::bug(function_name + " failed, " + p_name + " empty");
					lime::exit_program(1);
				}
				if (p.size() == 1) {
					if (p[0] == "") {
						lime::bug(function_name + " failed, single empty element invalid in " + p_name);
						lime::exit_program(1);
					}
				}
			}

			void check_this_validity(const lime::string &function_name) const noexcept {
				return check_path_validity(*this, "this", function_name);
			}

			std::chrono::system_clock::time_point get_last_modification_time() const noexcept {
				check_this_validity("path::get_last_modification_time");

				struct stat stat_buf;

				if (stat(this->to_std_string().c_str(), &stat_buf) < 0) {
					lime::bug("path::get_last_modification_time for " + this->to_std_string() + "failed, general failure");
					lime::exit_program(1);
				}

				return std::chrono::system_clock::from_time_t(stat_buf.st_mtime);
			}

			// NOTE: For some reason, the vector iterators don't play nice with pointers,
			// so have to use the vector iterators as our iterators, which makes more sense anyway.
			// It's what we should be doing anyway. I just assumed std::vector used pointers as iterators,
			// but I guess that's totally implementation defined. For strange architectures with strange memory
			// geometries, it makes sense that they wouldn't be pointers.
			// Still, they should be representable by pointers on my system, so it surprises me that they're not.
			// I guess it's a safety feature to stop you from assuming that they're pointers, because they're not,
			// even when they could be.
			using const_iterator = decltype(heirarchy)::const_iterator;
			using iterator = decltype(heirarchy)::iterator;

			const_iterator begin() const noexcept { return heirarchy.begin(); }
			iterator begin() noexcept { return heirarchy.begin(); }
			const_iterator end() const noexcept { return heirarchy.end(); }
			iterator end() noexcept { return heirarchy.end(); }

			const std::string& operator[](size_t index) const noexcept {
				if (index >= heirarchy.size()) {
					lime::bug("path::operator[] const failed, index out-of-bounds");
					lime::exit_program(1);
				}
				return heirarchy[index];
			}
			std::string& operator[](size_t index) noexcept {
				if (index >= heirarchy.size()) {
					lime::bug("path::operator[] failed, index out-of-bounds");
					lime::exit_program(1);
				}
				return heirarchy[index];
			}

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
					const std::string &element = (*this)[i];
					result += element + '/';
				}
				result += *(heirarchy.end() - 1);
				return result;
			}
		};

		// TODO: FROM HERE

		string(const path &path) noexcept : std::string(path.to_std_string()) { }

		template <typename T>
		std::vector<lime::string> inner_split(T delimiter, size_t delimiter_length) const noexcept {
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

		string() noexcept = default;

		string(const char *raw_str) noexcept : std::string(raw_str) { }

		string(const std::string& std_string) noexcept : std::string(std_string) { }
		string(std::string&& std_string)      noexcept : std::string(std::move(std_string)) { }

		lime::string operator+(const lime::string& right) const noexcept { return lime::string(*(const std::string*)this + *(const std::string*)&right); }
		lime::string& operator+=(const lime::string& right) noexcept { std::string::operator+=(*(const std::string*)&right); return *this; }

		lime::string operator+(const char *raw_str) const noexcept { return lime::string(*(const std::string*)this + raw_str); }
		lime::string& operator+=(const char *raw_str) noexcept { std::string::operator+=(raw_str); return *this; }

		lime::string operator+(char character) const noexcept { return lime::string(*(const std::string*)this + character); }
		lime::string& operator+=(char character) noexcept { std::string::operator+=(character); return *this; }

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

		bool operator==(const lime::string &other) const noexcept {
			return to_std_string() == other.to_std_string();
		}
		bool operator==(const char *other) const noexcept {
			return to_std_string() == other;
		}

		std::string to_std_string() const noexcept {
			return *(const std::string*)this;
		}

		// NOTE: These just bring a select few functions, which are marked private because of the above private inheritance, into the public "space".
		using std::string::c_str;
		using std::string::data;
		using std::string::length;

		size_t find(char character, size_t start_position)             const noexcept { return std::string::find(character, start_position); }
		size_t find(char character)                                    const noexcept { return std::string::find(character); }
		size_t find(const char *string, size_t start_position)         const noexcept { return std::string::find(string, start_position); }
		size_t find(const char *string)                                const noexcept { return std::string::find(string); }
		size_t find(const lime::string& string, size_t start_position) const noexcept { return std::string::find(string.c_str(), start_position); }
		size_t find(const lime::string& string)                        const noexcept { return std::string::find(string.c_str()); }

		lime::string substr(size_t index, size_t substr_length) const noexcept {
			if (index >= length() || substr_length > length() - index) { lime::error("substr(index, substr_length) was called with out-of-bounds arguments"); exit_program(1); }
			return lime::string(std::string::substr(index, substr_length));
		}
		lime::string substr(size_t index) const noexcept {
			if (index >= length()) { lime::error("substr(index) was called with out-of-bounds arguments"); exit_program(1); }
			return lime::string(std::string::substr(index));
		}

		std::vector<lime::string> split(const char *delimiter)         const noexcept { return inner_split(delimiter, std::strlen(delimiter)); }
		std::vector<lime::string> split(char delimiter)                const noexcept { return inner_split(delimiter, 1); }
		std::vector<lime::string> split(const lime::string& delimiter) const noexcept { return inner_split(delimiter.c_str(), delimiter.length()); }

		lime::string insert(size_t index, const char *string) const noexcept {
			if (index > length()) { lime::error("insert(index, string) was called with out-of-bounds arguments"); exit_program(1); }
			lime::string result = *this;
			result.std::string::insert(index, string);
			return result;
		}
		lime::string insert(size_t index, char character) const noexcept {
			if (index > length()) { lime::error("insert(index, character) was called with out-of-bounds arguments"); exit_program(1); }
			lime::string result = *this;
			result.std::string::insert(index, &character, 1);
			return result;
		}
		lime::string insert(size_t index, const lime::string& string) const noexcept {
			if (index > length()) { lime::error("insert(index, string) was called with out-of-bounds arguments"); exit_program(1); }
			lime::string result = *this;
			result.std::string::insert(index, string.c_str(), string.length());
			return result;
		}

		lime::string get_parent_folder() const noexcept {
			path absolute_path = path(*(const std::string*)this).to_absolute();
			return absolute_path.get_parent_folder();
		}

		lime::string get_filename() const noexcept {
			return path(*this).get_filename();
		}

		bool file_exists() const noexcept {
			int fd = open(c_str(), O_RDONLY);
			if (fd < 0) {
				switch (errno) {
				case EACCES:
					return false;
				default:
					lime::error("lime::file_exists() failed, general failure");
					exit_program(1);
				}
			}
			if (close(fd) < 0) {
				lime::error("lime::file_exists() failed, close failed, general failure");
				exit_program(1);
			}
			return true;
		}

		bool is_existing_directory() const noexcept {
			if (!file_exists()) { return false; }

			struct stat stat_buf;
			if (stat(c_str(), &stat_buf) < 0) {
				lime::error("lime::is_existing_directory() failed, stat failed, general failure");
				exit_program(1);
			}

			return S_ISDIR(stat_buf.st_mode);
		}

		bool is_absolute() const noexcept { return path(*this).is_absolute(); }

		lime::string to_absolute() const noexcept { return path(*this).to_absolute(); }

		lime::string get_relative_path(const lime::string& base) const noexcept {
			path base_path(base);
			path this_path(*this);
			return this_path.get_relative_path(base_path);
		}

		std::chrono::system_clock::time_point get_last_modification_time() const noexcept {
			return path(*this).get_last_modification_time();
		}

		size_t num_path_parts() const noexcept {
			return path(*this).size();
		}

		lime::string path_part(size_t index) const noexcept {
			path temp_path(*this);
			if (index >= temp_path.size()) {
				lime::error("lime::string::path_part(index) called with out-of-bounds index");
				lime::exit_program(1);
			}
			return temp_path[index];
		}

		lime::string to_canonicalized_absolute() const noexcept {
			path result = path(*this).to_canonicalized_absolute();
			if (result.is_empty()) {
				lime::error("lime::string::to_canonicalized_absolute failed, general failure");
				lime::exit_program(1);
				// TODO: Can get more specific with errno.
			}
		}

		lime::string strip_trailing_slash() const noexcept {
			if (this->length() == 0) {
				lime::error("lime::string::strip_trailing_slash() failed, string empty");
				lime::exit_program(1);
			}

			return path(*this).strip_trailing_slash();
		}
	};

	lime::string pwd() noexcept {
		char buffer[PATH_MAX + 1];	// NOTE: +1 because NUL character
		if (getcwd(buffer, sizeof(buffer)) == nullptr) {
			lime::error("getcwd failed in lime::pwd(), couldn't get cwd");
			lime::exit_program(1);
		}
		return lime::string(buffer);
	}

	void cd(lime::string new_directory) noexcept {
		if (chdir(new_directory.c_str()) < 0) {
			switch (errno) {
			case ENOTDIR:
				lime::error("lime::cd failed, at least one component of new_directory is a file");
				lime::exit_program(1);
			default:
				lime::error("lime:cd failed, general failure, platform explanation: " + lime::string(strerror(errno)));
				lime::exit_program(1);
			}
		}
	}

	template <typename functor_t>
	bool call_if_self_rebuild_necessary(const lime::string& src_file_path, functor_t functor) noexcept {
		lime::string self_exe_path("/proc/self/exe");
		if (self_exe_path.get_last_modification_time() < src_file_path.get_last_modification_time()) {
			lime::info("self rebuild necessary, calling self rebuild function...");
			functor();
			lime::info("self rebuild finished");
			return true;
		}
		return false;
	}

	template <typename functor_t>
	bool call_if_out_of_date(const lime::string& path, std::vector<lime::string> deps, functor_t functor) noexcept {
		for (const lime::string& dep : deps) {
			if (path.get_last_modification_time() < dep.get_last_modification_time()) {
				lime::info('\"' + path + '\"' + " is out-of-date, calling remedial function...");
				functor();
				lime::info('\"' + path + '\"' + " remedied");
				return true;
			}
		}
		return false;
	}

	enum class inner_execute_command_return_t : uint8_t {
		SUCCESS,
		COMMAND_FAILED,
		INVOKE_FAILED
	};

	inline inner_execute_command_return_t inner_execute_command(const lime::string& cmdline) noexcept {
		std::vector<lime::string> quote_separated = cmdline.split('\"');

		std::vector<lime::string> final_args;
		for (size_t i = 0; i < quote_separated.size(); i += 2) {
			std::vector<lime::string> new_args = quote_separated[0].split(' ');
			for (const lime::string &arg : new_args) { final_args.push_back(arg); }
			final_args.push_back(quote_separated[i + 1]);
		}

		const char * const target_file = final_args[0].c_str();

		// TODO: Figure out why the consts are the way they are for this.
		char ** const converted_final_args = new (std::nothrow) char *[final_args.size() + 1];
		for (size_t i = 0; i < final_args.size(); i++) {
			converted_final_args[i] = final_args[i].data();
		}
		converted_final_args[final_args.size()] = nullptr;

		pid_t vfork_result = vfork();

		// CHILD
		if (vfork_result == 0) {
			if (execvp(target_file, converted_final_args) == -1) { return inner_execute_command_return_t::INVOKE_FAILED; }
		}
		
		// PARENT
		delete[] converted_final_args;

		if (vfork_result == -1) { return inner_execute_command_return_t::INVOKE_FAILED; }

		int wstatus;
		if (wait(&wstatus) == -1) { return inner_execute_command_return_t::INVOKE_FAILED; }
		if (WIFEXITED(wstatus)) {
			if (WEXITSTATUS(wstatus) != EXIT_SUCCESS) { return inner_execute_command_return_t::COMMAND_FAILED; }
		}
		if (WIFSIGNALED(wstatus)) { return inner_execute_command_return_t::COMMAND_FAILED; }
		return inner_execute_command_return_t::SUCCESS;
	}

	inline void exec(const lime::string& cmdline) noexcept {
		lime::cmd_label(cmdline);
		switch (inner_execute_command(cmdline)) {
		case inner_execute_command_return_t::SUCCESS: break;
		case inner_execute_command_return_t::COMMAND_FAILED: lime::error("exec(cmdline) failed, invoked command failed"); exit_program(1);
		case inner_execute_command_return_t::INVOKE_FAILED: lime::error("exec(cmdline) failed because of unsuccessful invocation"); exit_program(1);
		}
	}

	template <typename functor_t>
	bool for_each_arg(unsigned int argc, const char * const *argv, functor_t functor) noexcept {
		if (argc == 1) { return false; }
		for (unsigned int i = 1; i < argc; i++) { functor(argv[i]); }
		return true;
	}

	inline std::vector<lime::string> enum_files(const lime::string& target_dir, const lime::string& query) noexcept {
		lime::string previous_working_dir = lime::pwd();
		lime::cd(target_dir.c_str());

		glob_t glob_result;
		// TODO: Fix that thing where if glob spec if an absolute path then it'll look in root instead. Make sure glob path isn't absolute.
		if (glob(query.c_str(), 0, nullptr, &glob_result) != 0) {
			lime::error("lime::enum_files failed, glob failed, general failure");
			lime::exit_program(1);
		}

		std::vector<lime::string> result;
		for (size_t i = 0; i < glob_result.gl_pathc; i++) {
			lime::string matched_file = lime::string(glob_result.gl_pathv[i]);
			lime::string matched_file_filename = matched_file.get_filename();
			if (matched_file_filename == "." || matched_file_filename == "..") { continue; }
			result.push_back(matched_file.to_absolute());
			// TODO: Make sure all lime function return absolute paths.
		}

		globfree(&glob_result);

		// NOTE: We do this here because to_absolute in the above for loop needs the cwd to be a certain way.
		lime::cd(previous_working_dir.c_str());

		return result;
	}

	inline std::vector<lime::string> enum_files_recursive(const lime::string& target_dir, const lime::string& query) noexcept {
		std::vector<lime::string> children = enum_files(target_dir, query);
		for (decltype(children)::iterator it = children.begin(); it < children.end(); it++) {
			if ((*it).is_existing_directory()) {
				std::vector<lime::string> grandchildren = enum_files_recursive(*it, query);
				children.erase(it);
				children.insert(children.end(), grandchildren.begin(), grandchildren.end());
				it--;
			}
		}
		return children;
	}

	inline void create_path(const lime::string& path) noexcept {
		lime::string real_path = path.to_canonicalized_absolute();
		lime::string current_path;
		for (size_t i = 0; i < real_path.num_path_parts(); i++) {
			current_path /= real_path.path_part(i);
			if (!current_path.is_existing_directory()) {
				// TODO: Just inherit from parent folder, I think that's the best option, right?
				if (open(current_path.c_str(), O_CREAT, S_IRWXU | S_IRGRP | S_IROTH) < 0) {
					lime::error("failed to create the target file in lime::create_path for some reason");
				}
				return;
			}
			// TODO: Just inherit from parent folder, same as above.
			if (mkdir(current_path.c_str(), S_IRWXU | S_IRGRP | S_IROTH) < 0) {
				switch (errno) {
				case EEXIST: continue;
				default:
					lime::error("mkdir failed in lime::create_path, general failure");
					lime::exit_program(1);
				}
			}
		}
	}

	inline void error(const lime::string& message) noexcept {
		fflush(stdout);
		lime::string final_message = "[ERROR]: " + message + '\n';
		write(STDERR_FILENO, final_message.c_str(), final_message.length() * sizeof(char));
		// no point in detecting error
	}

	inline void warn(const lime::string& message) noexcept {
		lime::string final_message = "[WARNING]: " + message + '\n';
		if (fwrite(final_message.c_str(), sizeof(char), final_message.length(), stdout) < final_message.length()) {
			lime::error("lime::warn failed, fwrite failed");
			lime::exit_program(1);
		}
	}

	inline void info(const lime::string& message) noexcept {
		lime::string final_message = "[INFO]: " + message + '\n';
		if (fwrite(final_message.c_str(), sizeof(char), final_message.length(), stdout) < final_message.length()) {
			lime::error("lime::info failed, fwrite failed");
			lime::exit_program(1);
		}
	}

	inline void cmd_label(const lime::string& message) noexcept {
		lime::string final_message = "[CMD]: " + message + '\n';
		if (fwrite(final_message.c_str(), sizeof(char), final_message.length(), stdout) < final_message.length()) {
			lime::error("lime::cmd_label failed, fwrite failed");
			lime::exit_program(1);
		}
	}

	inline void bug(const lime::string &message) noexcept {
		fflush(stdout);
		lime::string final_message = "[BUG DETECTED]: " + message + '\n';
		write(STDERR_FILENO, final_message.c_str(), final_message.length() * sizeof(char));
		// no point in detecting error
	}

}

inline lime::string operator+(const char *raw_str, const lime::string &lime_string) noexcept { return lime_string.insert(0, raw_str); }
inline lime::string operator+(char character, const lime::string& lime_string)      noexcept { return lime_string.insert(0, character); }

inline lime::string operator/(const char *raw_str, const lime::string& lime_string) noexcept { return lime::string(raw_str) / lime_string; }

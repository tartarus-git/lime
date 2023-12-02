#include "lime_build.h"

#include <ctime>

int main() {
	lime::info("testing output methods...");
	lime::info("test");
	lime::error("test");
	lime::warn("test");
	lime::bug("test");
	lime::cmd_label("test");

	lime::info("testing basic path operations");
	lime::string cwd = lime::pwd();
	lime::info(cwd);
	for (size_t i = 0; i < cwd.num_path_parts(); i++) {
		lime::info(cwd.path_part(i));
	}
	lime::string cwd_relative = ".";
	lime::info(cwd_relative);
	lime::string cwd_relative_absolutized = cwd_relative.to_absolute();
	lime::info(cwd_relative_absolutized);		// TODO: Not optimal.

	lime::info("enumeration...");
	std::vector<lime::string> enumerated_files = lime::enum_files(cwd_relative, "*");
	for (const lime::string &file : enumerated_files) {
		lime::info(file);
	}
	lime::info("");
	std::vector<lime::string> enumerated_files_recursive = lime::enum_files_recursive(cwd_relative, "*");
	for (const lime::string &file : enumerated_files_recursive) {
		lime::info(file);
	}
	lime::info("");
	std::vector<lime::string> enumerated_dotfiles_recursive = lime::enum_files_recursive(cwd_relative, ".*");
	for (const lime::string &file : enumerated_dotfiles_recursive) {
		lime::info(file);
	}

	lime::info("");
	lime::info("relative path testing");
	lime::string abs_1 = "../..";
	lime::info(abs_1);
	lime::string abs_2 = "./.";
	lime::info(abs_2);
	lime::info(abs_2.get_relative_path(abs_1));

	lime::info("");
	lime::info("modification time testing");
	lime::string target_file = "lime_build.h";
	std::chrono::system_clock::time_point modification_time = target_file.get_last_modification_time();
	std::time_t c_time = std::chrono::system_clock::to_time_t(modification_time);
	const char *time_output = std::ctime(&c_time);
	lime::info(time_output);

	lime::info("");
	lime::info("filename");
	lime::info("./bin/../lime_build.h");
	lime::info(lime::string("./bin/../lime_build.h").get_filename());
}

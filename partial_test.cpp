#include "lime_build.h"

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
}

#include "lime_build.h"

#define COMPILER "clang++-15"
#define BINARY_NAME "bin/test_binary"

void build_and_link_all_cpp_files() noexcept {
	for (lime::string path : lime::enum_files_recursive(".", "*.cpp")) {
		lime::string object_path = path.get_relative_path("src").remove_extention().add_extention("o").apply_relative_path_to("bin");
		lime::call_if_out_of_date(object_path, path, []() {
			lime::create_path(object_path.get_parent_folder());
			lime::exec(COMPILER + "-o " + object_path + ' ' + path);
		});
	}

	lime::string object_files;
	for (lime::string path : lime::enum_files_recursive("bin", "*.o")) {
		object_files += " " += path;
	}
	lime::exec(COMPILER + "-o " + BINARY_NAME + object_files);
}

void clean() noexcept {
	lime::exec("git clean -fdx");
}

int main(int argc, const char **argv) noexcept {
	lime::call_if_self_rebuild_necessary("build.cpp", []() {
		lime::exec("mv build build.old");
		lime::exec(COMPILER + "-o build build.cpp");
	});

	bool args_present = lime::for_each_arg(argc, argv, [](lime::string arg) {
		if (!lime::string_match<"all | clean">(arg,
			[]() { build_and_link_all_cpp_files(); },
			[]() { clean(); }
		)) {
			lime::error("invalid argument");
		}
	});

	if (!args_present) { build_and_link_all_cpp_files(); }
}

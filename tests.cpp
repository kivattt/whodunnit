#include <iostream>
#include <string>
#include <map>

#include "util.hpp"

using std::string;

int main() {
	std::cout << "Testing parent_dir()" << std::endl;
	std::map<string, string> expected = {
		{"/home/user", "/home"},
		{"/home/user/file.txt", "/home/user"},
		{"/home/kivah/main/projects/fen/lua-file-preview-examples/markdown.lua", "/home/kivah/main/projects/fen/lua-file-preview-examples"}
	};
	for (auto &[input, expected] : expected) {
		const string got = parent_dir(input);
		if (got != expected) {
			std::cerr << "Expected \"" << expected << "\", but got: \"" << got << "\"" << std::endl;
			return 1;
		}
	}

	std::cout << "Testing without_ending_slash()" << std::endl;
	expected = {
		{"/home/user", "/home/user"},
		{"/home/user/", "/home/user"}
	};
	for (auto &[input, expected] : expected) {
		const string got = without_ending_slash(input);
		if (got != expected) {
			std::cerr << "Expected \"" << expected << "\", but got: \"" << got << "\"" << std::endl;
			return 1;
		}
	}

	std::cout << "\nTests succeeded" << std::endl;
}

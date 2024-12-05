#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>

using std::string;

string sanitize_shell_argument(const string &input){
	string octal = "";

	for (char c : input){
		octal += "\\0";
		octal += '0' + (c >> 6 & 7);
		octal += '0' + (c >> 3 & 7);
		octal += '0' + (c      & 7);
	}

	return "$(printf '%b' '" + octal + "')";
}

string without_ending_slash(string input) {
	if (input.empty() || input.back() != '/') {
		return input;
	}

	return input.substr(0, input.size() - 1);
}

string parent_dir(string filename) {
	size_t lastSlash = filename.find_last_of('/');
	if (lastSlash == string::npos) {
		return filename;
	}

	return without_ending_slash(filename.substr(0, lastSlash));
}

#endif // UTIL_HPP

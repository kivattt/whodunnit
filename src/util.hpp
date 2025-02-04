#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <cmath>
#include <SFML/Graphics.hpp>

using std::string;

template <typename T>
bool within(T n, T min, T max) {
	if (n < min) {
		return false;
	}
	if (n >= max) {
		return false;
	}

	return true;
}

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

string without_ending_slash(string path) {
	if (path.empty() || path.back() != '/') {
		return path;
	}

	return path.substr(0, path.size() - 1);
}

string with_ending_slash(string path) {
	if (path.empty() || path.back() == '/') {
		return path;
	}

	return path + "/";
}

string absolute_path(string path) {
	if (path.empty()) {
		return path;
	}

	if (path[0] == '/') {
		return path;
	}

	char *cwd = get_current_dir_name();
	return with_ending_slash(cwd) + path;
}

string parent_path(string path) {
	size_t lastSlash = path.find_last_of('/');
	if (lastSlash == string::npos) {
		return ".";
	}

	return without_ending_slash(path.substr(0, lastSlash));
}

string basename(string path) {
	size_t lastSlash = path.find_last_of('/');
	if (lastSlash == string::npos) {
		return path;
	}

	return path.substr(lastSlash+1);
}

sf::Color hsv_to_rgb(double h, double s, double v) {
	if (h > 360)
		h = fmod(h, 360);

	double hh, p, q, t, ff;
	long i;

	double r,g,b;

	if(s <= 0.0) { // < is bogus, just shuts up warnings
		r = v;
		g = v;
		b = v;
		//return sf::Color((std::uint8_t)r*255, (std::uint8_t)g*255, (std::uint8_t)b*255);
		return sf::Color(r*255, g*255, b*255);
	}
	hh = h;
	if(hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = v * (1.0 - s);
	q = v * (1.0 - (s * ff));
	t = v * (1.0 - (s * (1.0 - ff)));
	
	switch(i) {
	case 0:
		r = v;
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = v;
		b = p;
		break;
	case 2:
		r = p;
		g = v;
		b = t;
		break;
	
	case 3:
		r = p;
		g = q;
		b = v;
		break;
	case 4:
		r = t;
		g = p;
		b = v;
		break;
	case 5:
	default:
		r = v;
		g = p;
		b = q;
		break;
	}

	//return sf::Color((std::uint8_t)r*255, (std::uint8_t)g*255, (std::uint8_t)b*255);
	return sf::Color(r*255, g*255, b*255);
}

// Returns an empty string "" if nothing found
string get_remote_url(string filename) {
	char tempFilename[] = "/tmp/whodunnit-XXXXXX";
	int fd = mkstemp(tempFilename);
	if (fd == -1) {
		return "";
	}
	close(fd);

	char *previousDirName = get_current_dir_name();
	if (chdir(absolute_path(parent_path(filename)).c_str()) == -1) {
		free(previousDirName);
		return "";
	}

	int exitCode = system(string("git config --get remote.origin.url > " + string(tempFilename)).c_str());
	if (exitCode != 0) {
		free(previousDirName);
		return "";
	}

	if (chdir(previousDirName) == -1) {
		free(previousDirName);
		return "";
	}

	std::ifstream file(tempFilename);
	std::stringstream ss;
	ss << file.rdbuf();
	string ret = ss.str();
	file.close();

	// Delete the temp file
	unlink(tempFilename);

	free(previousDirName);
	return ret.substr(0, ret.size()-1);
}

string lowercase(string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){
		return std::tolower(c);
	});

	return str;
}

string remote_url_to_site_name(string remote) {
	if (remote.starts_with("https://")) {
		remote = remote.substr(string("https://").size());
	}

	if (remote.starts_with("http://")) {
		remote = remote.substr(string("http://").size());
	}

	if (remote.starts_with("github.com")) {
		return "GitHub";
	}
	if (remote.starts_with("gitlab.com")) {
		return "GitLab";
	}

	return "Remote website";
}

#endif // UTIL_HPP

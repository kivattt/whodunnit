#ifndef UTIL_HPP
#define UTIL_HPP

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

string parent_dir(string path) {
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

	return sf::Color(r*255, g*255, b*255);
}

#endif // UTIL_HPP

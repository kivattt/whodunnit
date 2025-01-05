#include <iostream>
#include <string>

#include "util.hpp"
#include "whodunnit.hpp"

using std::string;
using std::vector;

void usage(string programName) {
	std::cout << "Usage: " << programName << " [FILE]" << std::endl;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage(argv[0]);
		return 2;
	}

	vector<string> filenames;
	for (int i = 1; i < argc; i++) {
		filenames.push_back(argv[i]);
	}

	WhoDunnit whoDunnit;
	return whoDunnit.run(filenames);
}

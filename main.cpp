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

	WhoDunnit whoDunnit;
	return whoDunnit.run(argv[1]);
}

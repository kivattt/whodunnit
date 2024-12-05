#include <iostream>
#include <fstream>
#include <optional>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

#include "util.hpp"

#define WIDTH 1280
#define HEIGHT 720

using std::string;
using std::vector;

void usage(string programName) {
	std::cout << "Usage: " << programName << " [FILE]" << std::endl;
}

struct BlameLine{
	string line;
};

std::optional<vector<BlameLine>> run_git_blame(string filename) {
	char tempFilename[] = "/tmp/whodunnit-XXXXXX";
	int fd = mkstemp(tempFilename);
	if (fd == -1) {
		return std::nullopt;
	}

	close(fd);

	char *previousDirName = get_current_dir_name();
	if (chdir(parent_dir(filename).c_str()) == -1) {
		free(previousDirName);
		return std::nullopt;
	}

	int exitCode = system(string("git blame --line-porcelain -t " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
	if (exitCode != 0) {
		free(previousDirName);
		return std::nullopt;
	}

	if (chdir(previousDirName) == -1) {
		free(previousDirName);
		return std::nullopt;
	}

	vector<BlameLine> ret;

	std::ifstream file(tempFilename);

	BlameLine currentBlameLine;
	unsigned long long lineNum = 0;
	for (string line; std::getline(file, line); ++lineNum) {
		if (line.empty()) continue;

		if (line.front() == '\t') {
			currentBlameLine.line = line.substr(1);
			ret.push_back(currentBlameLine);
		}
	}

	// Delete the temp file
	unlink(tempFilename);

	free(previousDirName);
	return ret;
}

sf::Font theFont;

struct BlameFile{
	vector<BlameLine> blameLines;
	vector<sf::Text> textLines;

	BlameFile(vector<BlameLine> newBlameLines) {
		blameLines = newBlameLines;
		for (BlameLine &e : blameLines) {
			sf::Text text;
			text.setFont(theFont);
			text.setString(e.line);
			text.setCharacterSize(20);
			text.setFillColor(sf::Color::White);
			textLines.push_back(text);
		}
	}
};

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage(argv[0]);
		return 2;
	}

	string filename = argv[1];
	std::cout << filename << '\n';

	std::optional<vector<BlameLine>> gitBlame = run_git_blame(filename);
	if (!gitBlame) {
		std::cerr << "Failed to run git blame\n";
		return 1;
	}

	for (auto e : gitBlame.value()) {
		std::cout << "Line: " << e.line << '\n';
	}

	if (!theFont.loadFromFile("fonts/JetBrainsMono-Regular.ttf")) {
		std::cerr << "Failed to load font at fonts/JetBrainsMono-Regular.ttf";
		return 1;
	}

	BlameFile theFile(gitBlame.value());

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "blame-viewer");
	window.setVerticalSyncEnabled(true);

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}

			if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Q)
					window.close();
			}

			if (event.type == sf::Event::Resized) {
				sf::FloatRect visibleArea(0.0f, 0.0f, event.size.width, event.size.height);
				window.setView(sf::View(visibleArea));
			}
		}

		window.clear();
		for (int i = 0; i < theFile.textLines.size(); i++) {
			theFile.textLines[i].setPosition(0, i*20);
			window.draw(theFile.textLines[i]);
		}
		window.display();
	}

	return 0;
}

#ifndef WHODUNNIT_HPP
#define WHODUNNIT_HPP

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

sf::Font theFont;

struct BlameLine{
	//string commitHash;
	string author;
	string line;
};

struct BlameFile{
	vector<BlameLine> blameLines;
	vector<sf::Text> textLines;
	vector<sf::Text> authorLines;

	BlameFile(vector<BlameLine> newBlameLines) {
		blameLines = newBlameLines;
		for (BlameLine &e : blameLines) {
			sf::Text text;
			text.setFont(theFont);
			text.setString(e.line);
			text.setCharacterSize(15);
			text.setFillColor(sf::Color::White);
			textLines.push_back(text);

			text.setFont(theFont);
			text.setString(e.author);
			text.setCharacterSize(15);
			text.setFillColor(sf::Color(150,150,150));
			authorLines.push_back(text);
		}
	}
};

class WhoDunnit{
	public:
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

		unsigned long long lineNum = 0;
		BlameLine currentBlameLine;
		for (string line; std::getline(file, line); ++lineNum) {
			std::cout << line << std::endl;
			if (line.empty()) continue;

			if (line.front() == '\t') {
				currentBlameLine.line = line.substr(1);
				ret.push_back(currentBlameLine);
				currentBlameLine = BlameLine();
				continue;
			}

			if (line.starts_with("author ")) {
				currentBlameLine.author = line.substr(string("author ").size());
			}
		}

		// Delete the temp file
		unlink(tempFilename);

		free(previousDirName);
		return ret;
	}

	int run(string filename) {
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
				switch (event.type) {
					case sf::Event::Closed:
						window.close();
						break;
					case sf::Event::KeyPressed:
						if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Q) {
							window.close();
						}
						break;
					case sf::Event::Resized:
						sf::FloatRect visibleArea(0.0f, 0.0f, event.size.width, event.size.height);
						window.setView(sf::View(visibleArea));
						break;
				}
			}

			window.clear();
			for (int i = 0; i < theFile.textLines.size(); i++) {
				theFile.textLines[i].setPosition(100, i*20);
				window.draw(theFile.textLines[i]);

				theFile.authorLines[i].setPosition(0, i*20);
				window.draw(theFile.authorLines[i]);
			}
			window.display();
		}

		return 0;
	}
};

#endif // WHODUNNIT_HPP

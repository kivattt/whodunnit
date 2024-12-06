#ifndef WHODUNNIT_HPP
#define WHODUNNIT_HPP

#include <iostream>
#include <fstream>
#include <climits>
#include <algorithm>
#include <cmath>
#include <optional>
#include <cstdlib>
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
int fontSizePixels = 15;

struct BlameLine{
	//string commitHash;
	string author;

	unsigned long long committerTime = 0;

	string line;
};

struct BlameFile{
	vector<BlameLine> blameLines;
	unsigned long long oldestCommitterTime = ULLONG_MAX;
	unsigned long long newestCommitterTime = 0;

	double committer_time_0_to_1(unsigned long long committerTime) {
		double zeroToOne = double(committerTime - oldestCommitterTime) / double(newestCommitterTime - oldestCommitterTime);
		return zeroToOne;
	}

	vector<sf::Text> textLines;
	vector<sf::Text> authorLines;

	void set_texts() {
		for (BlameLine &e : blameLines) {
			sf::Text text;
			text.setFont(theFont);
			text.setString(e.line);
			text.setCharacterSize(fontSizePixels);
			text.setFillColor(sf::Color(200,200,200));
			textLines.push_back(text);

			text.setFont(theFont);
			text.setString(e.author);
			text.setCharacterSize(fontSizePixels);

			const double lowestBrightness = 0.3;
			int color = (lowestBrightness + committer_time_0_to_1(e.committerTime) * (1.0 - lowestBrightness)) * 255;

			text.setFillColor(sf::Color(color, color/1.5, color/1.5));
			authorLines.push_back(text);
		}
	}
};

class WhoDunnit{
	public:
	std::optional<BlameFile> run_git_blame(string filename) {
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

		BlameFile ret;

		std::ifstream file(tempFilename);

		unsigned long long lineNum = 0;
		BlameLine currentBlameLine;
		for (string line; std::getline(file, line); ++lineNum) {
			//std::cout << line << std::endl;
			if (line.empty()) {
				continue;
			}

			if (line.front() == '\t') {
				currentBlameLine.line = line.substr(1);
				ret.blameLines.push_back(currentBlameLine);
				currentBlameLine = BlameLine();
				continue;
			}

			// We could be doing bounds-checking before the line.substr() is called
			if (line.starts_with("author ")) {
				currentBlameLine.author = line.substr(string("author ").size());
				continue;
			}

			if (line.starts_with("committer-time ")) {
				unsigned long long time = 0;
				try {
					time = std::stoull(line.substr(string("committer-time ").size()).c_str());
				} catch(std::invalid_argument &e) {
					continue;
				}
				ret.oldestCommitterTime = std::min(time, ret.oldestCommitterTime);
				ret.newestCommitterTime = std::max(time, ret.newestCommitterTime);
				currentBlameLine.committerTime = time;
				continue;
			}
		}

		// Delete the temp file
		unlink(tempFilename);

		free(previousDirName);

		ret.set_texts();
		return ret;
	}

	int run(string filename) {
		std::optional<BlameFile> blameFile = run_git_blame(filename);
		if (! blameFile) {
			std::cerr << "Failed to run git blame\n";
			return 1;
		}

		if (! theFont.loadFromFile("fonts/JetBrainsMono-Regular.ttf")) {
			std::cerr << "Failed to load font at fonts/JetBrainsMono-Regular.ttf";
			return 1;
		}

		BlameFile theFile = blameFile.value();

		sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "blame-viewer");
		window.setVerticalSyncEnabled(true);

		sf::RectangleShape verticalDividerRect;

		while (window.isOpen()) {
			sf::Event event;
			while (window.pollEvent(event)) {
				switch (event.type) {
					case sf::Event::Closed:
						window.close();
						break;
					case sf::Event::Resized:
						{
							sf::FloatRect visibleArea(0.0f, 0.0f, event.size.width, event.size.height);
							window.setView(sf::View(visibleArea));
						}
						break;
					case sf::Event::KeyPressed:
						if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::Q) {
							window.close();
						}
						break;
					case sf::Event::MouseWheelScrolled:
						if (! (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl))) {
							break;
						}

						// TODO: Make it logarithmic or whatever so the zoom feels intuitive
						fontSizePixels = std::max(1, int(fontSizePixels + event.mouseWheelScroll.delta));

						for (sf::Text &text : theFile.textLines) {
							text.setCharacterSize(fontSizePixels);
						}
						for (sf::Text &text : theFile.authorLines) {
							text.setCharacterSize(fontSizePixels);
						}
						break;
					default:
						break;
				}
			}

			window.clear();

			verticalDividerRect.setSize(sf::Vector2f(2, window.getSize().y));
			verticalDividerRect.setPosition(140, 0);
			verticalDividerRect.setFillColor(sf::Color(100,100,100));
			window.draw(verticalDividerRect);

			for (int i = 0; i < theFile.textLines.size(); i++) {
				float step = (fontSizePixels + fontSizePixels/5);
				float y = i * step;

				theFile.textLines[i].setPosition(150, y);
				window.draw(theFile.textLines[i]);

				theFile.authorLines[i].setPosition(0, y);
				window.draw(theFile.authorLines[i]);

				if (y + step > window.getSize().y) {
					break;
				}

			}
			window.display();
		}

		return 0;
	}
};

#endif // WHODUNNIT_HPP

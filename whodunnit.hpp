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

#define START_WIDTH 1280
#define START_HEIGHT 720

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

template <typename T>
bool within(T n, T min, T max) {
	if (n < min) {
		return false;
	}
	if (n > max) {
		return false;
	}

	return true;
}

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
	vector<sf::RectangleShape> blameBgs;

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

			//text.setFillColor(sf::Color(color, color/1.5, color/1.5));
			text.setFillColor(sf::Color(200,200,200));
			authorLines.push_back(text);

			/*const double lowestBrightness = 0.3;
			int color = (lowestBrightness + committer_time_0_to_1(e.committerTime) * (1.0 - lowestBrightness)) * 255;*/

			sf::RectangleShape rect;
			//rect.setFillColor(sf::Color(color, color/1.5, color/1.5));
			//int randomHue = rand() / double(RAND_MAX) * 360;
			double zeroToOne = committer_time_0_to_1(e.committerTime);
			rect.setFillColor(hsv_to_rgb(221, zeroToOne*0.65, 0.7d * (0.3 + zeroToOne * (1 - 0.3))));
			blameBgs.push_back(rect);
		}
	}
};

class WhoDunnit{
	public:

	int verticalDividerX = 190;
	bool movingVerticalDivider = false;

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

		sf::RenderWindow window(sf::VideoMode(START_WIDTH, START_HEIGHT), "blame-viewer");
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
					case sf::Event::MouseButtonPressed:
						if (event.mouseButton.button != sf::Mouse::Left) {
							break;
						}

						if (within(event.mouseButton.x, verticalDividerX-15, verticalDividerX+15)) {
							movingVerticalDivider = true;
						}
						break;
					case sf::Event::MouseButtonReleased:
						if (event.mouseButton.button == sf::Mouse::Left) {
							movingVerticalDivider = false;
						}
						break;
					case sf::Event::MouseMoved:
						if (! movingVerticalDivider) {
							break;
						}

						verticalDividerX = event.mouseMove.x;
						break;
					default:
						break;
				}
			}

			window.clear();

			for (int i = 0; i < theFile.textLines.size(); i++) {
				float step = (fontSizePixels + fontSizePixels/2);
				float y = i * step;

				theFile.blameBgs[i].setSize(sf::Vector2f(verticalDividerX, step));
				theFile.blameBgs[i].setPosition(0, y);
				window.draw(theFile.blameBgs[i]);

				sf::RectangleShape rect;
				rect.setSize(sf::Vector2f(window.getSize().x, step));
				rect.setPosition(verticalDividerX+2, y);
				sf::Color c = theFile.blameBgs[i].getFillColor();
				c.r /= 5;
				c.g /= 5;
				c.b /= 5;
				rect.setFillColor(c);
				window.draw(rect);

				theFile.authorLines[i].setPosition(0, y);
				window.draw(theFile.authorLines[i]);

				theFile.textLines[i].setPosition(verticalDividerX+10, y);
				window.draw(theFile.textLines[i]);

				if (y + step > window.getSize().y) {
					break;
				}

			}

			verticalDividerRect.setSize(sf::Vector2f(2, window.getSize().y));
			verticalDividerRect.setFillColor(sf::Color(100,100,100));
			verticalDividerRect.setPosition(verticalDividerX, 0);
			window.draw(verticalDividerRect);

			window.display();
		}

		return 0;
	}
};

#endif // WHODUNNIT_HPP

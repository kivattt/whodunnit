#ifndef WHODUNNIT_HPP
#define WHODUNNIT_HPP

#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
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
#include "button.hpp"

#define START_WIDTH 1280
#define START_HEIGHT 720
#define LEFT_DIVIDER_FROM_RIGHT 100
#define LEFT_DIVIDER_FROM_LEFT 100

using std::string;
using std::vector;

sf::Font theFont;
int fontSizePixels = 15;

struct BlameLine {
	string commitHash;
	string author;

	unsigned long long committerTime = 0;

	string line;
};

struct Commit {
	string author;
	string time;
	string commitHash;
	string title;
};

struct CommitThing {
	sf::Text authorText;
	sf::Text timeText;
	sf::Text commitHashText;
	sf::Text titleText;
};

struct BlameFile {
	vector<BlameLine> blameLines;
	vector<Commit> commitLog;
	vector<CommitThing> commitTexts;
	unsigned long long oldestCommitterTime = ULLONG_MAX;
	unsigned long long newestCommitterTime = 0;
	string oldestCommitHash = "";
	string newestCommitHash = "";

	vector<string> ignoreRevsList;

	double committer_time_0_to_1(unsigned long long committerTime) {
		double zeroToOne = double(committerTime - oldestCommitterTime) / double(newestCommitterTime - oldestCommitterTime);
		return zeroToOne;
	}

	int scrollPositionPixels = 0;
	vector<sf::Text> textLines;
	vector<sf::Text> authorLines;
	vector<sf::RectangleShape> blameBgs;

	void set_texts() {
		textLines.clear();
		authorLines.clear();
		blameBgs.clear();
		commitTexts.clear();

		for (BlameLine &e : blameLines) {
			sf::Text text;
			text.setFont(theFont);
			text.setString(sf::String::fromUtf8(e.line.begin(), e.line.end()));
			text.setCharacterSize(fontSizePixels);
			text.setFillColor(sf::Color(200,200,200));
			textLines.push_back(text);

			text.setFont(theFont);
			text.setString(sf::String::fromUtf8(e.author.begin(), e.author.end()));
			text.setCharacterSize(fontSizePixels);

			//text.setFillColor(sf::Color(color, color/1.5, color/1.5));
			if (e.commitHash == newestCommitHash) {
				text.setFillColor(sf::Color(40,40,40));
			} else {
				text.setFillColor(sf::Color(200,200,200));
			}
			authorLines.push_back(text);

			/*const double lowestBrightness = 0.3;
			int color = (lowestBrightness + committer_time_0_to_1(e.committerTime) * (1.0 - lowestBrightness)) * 255;*/

			sf::RectangleShape rect;
			//rect.setFillColor(sf::Color(color, color/1.5, color/1.5));
			//int randomHue = rand() / double(RAND_MAX) * 360;
			if (e.commitHash == newestCommitHash) {
				rect.setFillColor(hsv_to_rgb(130, 0.65, 1 - 0.3));
			} else {
				double zeroToOne = committer_time_0_to_1(e.committerTime);
				rect.setFillColor(hsv_to_rgb(221, zeroToOne*0.65, 0.7d * (0.3 + zeroToOne * (1 - 0.3))));
			}
			blameBgs.push_back(rect);
		}

		for (Commit &c : commitLog) {
			CommitThing thing;
			thing.authorText.setFont(theFont);
			thing.timeText.setFont(theFont);
			thing.commitHashText.setFont(theFont);
			thing.titleText.setFont(theFont);

			thing.authorText.setCharacterSize(fontSizePixels);
			thing.timeText.setCharacterSize(fontSizePixels);
			thing.commitHashText.setCharacterSize(fontSizePixels);
			thing.titleText.setCharacterSize(fontSizePixels);

			thing.authorText.setFillColor(sf::Color(200,200,200));
			thing.timeText.setFillColor(sf::Color(200,200,200));
			thing.commitHashText.setFillColor(sf::Color(200,200,200));
			thing.titleText.setFillColor(sf::Color(200,200,200));

			thing.authorText.setString(sf::String::fromUtf8(c.author.begin(), c.author.end()));
			thing.timeText.setString(c.time);
			thing.commitHashText.setString(c.commitHash);
			thing.titleText.setString(sf::String::fromUtf8(c.title.begin(), c.title.end()));

			commitTexts.push_back(thing);
		}
	}
};

class WhoDunnit{
	public:

	int leftDividerX = 190;
	int rightDividerX = START_WIDTH - 400;
	bool movingLeftDivider = false;
	bool movingRightDivider = false;

	void zoom(int level, BlameFile &theFile) {
		// TODO: Make it logarithmic or whatever so the zoom feels intuitive
		fontSizePixels = std::max(1, level);
		fontSizePixels = std::min(100, fontSizePixels); // Going higher will use ridiculous amounts of memory

		for (sf::Text &text : theFile.textLines) {
			text.setCharacterSize(fontSizePixels);
		}
		for (sf::Text &text : theFile.authorLines) {
			text.setCharacterSize(fontSizePixels);
		}

		// TODO: Maybe make separate zoom for git log
		for (CommitThing &c : theFile.commitTexts) {
			c.authorText.setCharacterSize(fontSizePixels);
			c.timeText.setCharacterSize(fontSizePixels);
			c.commitHashText.setCharacterSize(fontSizePixels);
			c.titleText.setCharacterSize(fontSizePixels);
		}
	}

	std::optional<vector<Commit>> run_git_log(string filename) {
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

		//int exitCode = system(string("git log --pretty=format:\"%an%n%at %H %s\" " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		int exitCode = system(string("git log --pretty=format:\"%an%n%as %H %s\" " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		if (exitCode != 0) {
			free(previousDirName);
			return std::nullopt;
		}

		if (chdir(previousDirName) == -1) {
			free(previousDirName);
			return std::nullopt;
		}

		vector<Commit> ret;

		std::ifstream file(tempFilename);
		unsigned long long lineNum = 0;
		bool lookingForAuthor = true;

		Commit currentCommit;
		for (string line; std::getline(file, line); ++lineNum) {
			//std::cout << line << std::endl;
			if (line.empty()) {
				continue;
			}

			if (lookingForAuthor) {
				currentCommit.author = line;
			} else {
				std::stringstream stream(line);
				stream >> currentCommit.time;
				stream >> currentCommit.commitHash;

				std::stringstream rest;
				rest << stream.rdbuf();
				currentCommit.title = rest.str().substr(1);

				ret.push_back(currentCommit);
				currentCommit = Commit();
			}

			lookingForAuthor = !lookingForAuthor;
		}

		// Delete the temp file
		unlink(tempFilename);

		free(previousDirName);
		return ret;
	}

	// Ignores commit hashes from the ignoreRevsList
	std::optional<BlameFile> run_git_blame(string filename, const vector<string> &ignoreRevsList) {
		auto start = std::chrono::high_resolution_clock::now();

		char tempFilename[] = "/tmp/whodunnit-XXXXXX";
		int fd = mkstemp(tempFilename);
		if (fd == -1) {
			return std::nullopt;
		}
		close(fd);

		// Temporary file for --ignore-revs-file
		char tempIgnoreRevsFilename[] = "/tmp/whodunnit-i-XXXXXX";
		int ignoreRevsFd = mkstemp(tempIgnoreRevsFilename);
		if (ignoreRevsFd == -1) {
			return std::nullopt;
		}
		close(ignoreRevsFd);

		std::ofstream ignoreRevsFile(tempIgnoreRevsFilename, std::ofstream::out | std::ofstream::trunc);
		for (const string &revision : ignoreRevsList) {
			ignoreRevsFile << revision << '\n';
		}
		ignoreRevsFile.close();

		char *previousDirName = get_current_dir_name();
		if (chdir(parent_dir(filename).c_str()) == -1) {
			free(previousDirName);
			return std::nullopt;
		}

		int exitCode = system(string("git blame --line-porcelain -t --ignore-revs-file " + string(tempIgnoreRevsFilename) + " " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		if (exitCode != 0) {
			free(previousDirName);
			return std::nullopt;
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto msInt = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
		std::cout << "Git blame time: " << msInt.count() << "ms" << std::endl;

		if (chdir(previousDirName) == -1) {
			free(previousDirName);
			return std::nullopt;
		}

		BlameFile ret;
		ret.ignoreRevsList = ignoreRevsList;

		std::ifstream file(tempFilename);

		bool lookingForCommitHash = true;
		unsigned long long lineNum = 0;
		BlameLine currentBlameLine;
		for (string line; std::getline(file, line); ++lineNum) {
			//std::cout << line << std::endl;
			if (line.empty()) {
				continue;
			}

			if (lookingForCommitHash) {
				size_t firstSpace = line.find_first_of(' ');
				if (firstSpace == string::npos) {
					currentBlameLine.commitHash = line;
					std::cerr << "Unexpected git blame --line-porcelain output" << std::endl;
				} else {
					currentBlameLine.commitHash = line.substr(0, firstSpace);
				}

				lookingForCommitHash = false;
				continue;
			}

			if (line.front() == '\t') {
				currentBlameLine.line = line.substr(1);
				ret.blameLines.push_back(currentBlameLine);
				currentBlameLine = BlameLine();

				lookingForCommitHash = true;
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

				if (time < ret.oldestCommitterTime) {
					ret.oldestCommitterTime = time;
					ret.oldestCommitHash = currentBlameLine.commitHash;
				}

				if (time > ret.newestCommitterTime) {
					ret.newestCommitterTime = time;
					ret.newestCommitHash = currentBlameLine.commitHash;
				}

				currentBlameLine.committerTime = time;
				continue;
			}
		}

		std::cout << "newest: " << ret.newestCommitHash << '\n';

		// Delete the temp file
		unlink(tempFilename);
		// Delete the temporary file for --ignore-revs-file
		unlink(tempIgnoreRevsFilename);

		free(previousDirName);

		ret.set_texts();
		return ret;
	}

	int run(string filename) {
		std::optional<vector<Commit>> gitLog = run_git_log(filename);
		if (! gitLog) {
			std::cerr << "Failed to run git log\n";
			return 1;
		}
		std::optional<BlameFile> blameFile = run_git_blame(filename, {});
		if (! blameFile) {
			std::cerr << "Failed to run git blame\n";
			return 1;
		}

		if (! theFont.loadFromFile("fonts/JetBrainsMono-Regular.ttf")) {
			std::cerr << "Failed to load font at fonts/JetBrainsMono-Regular.ttf";
			return 1;
		}

		BlameFile theFile = blameFile.value();
		theFile.commitLog = gitLog.value();
		theFile.set_texts();

		for (Commit &c : theFile.commitLog) {
			std::cout << c.time << ' ' << c.author << ' ' << c.commitHash << ' ' << c.title << '\n';
		}

		sf::RenderWindow window(sf::VideoMode(START_WIDTH, START_HEIGHT), "whodunnit - " + basename(filename));
		window.setVerticalSyncEnabled(true);

		sf::RectangleShape leftDividerRect;
		leftDividerRect.setFillColor(sf::Color(100,100,100));

		sf::RectangleShape rightDividerRect;
		rightDividerRect.setFillColor(sf::Color(100,100,100));

		/*sf::RectangleShape topbarRect;
		topbarRect.setPosition(0,0);
		topbarRect.setSize(sf::Vector2f(window.getSize().x, topbarHeight));
		topbarRect.setFillColor(sf::Color(160,160,160));*/

		auto updateGitBlame = [&]() {
			std::optional<BlameFile> blameFile = run_git_blame(filename, theFile.ignoreRevsList);
			if (! blameFile) {
				std::cerr << "Failed to run git blame\n";
				return;
			}
			theFile = blameFile.value();
			theFile.commitLog = gitLog.value();
			theFile.set_texts();
		};

		int topbarHeight = 35;

		float h = topbarHeight;
		Button button1({0,0}, {h,h}, theFont, "<");
		button1.set_on_click([&](){
			theFile.ignoreRevsList.push_back(theFile.newestCommitHash);
			/*for (auto &e : theFile.ignoreRevsList) {
				std::cout << "ignore: " << e << '\n';
			}*/
			updateGitBlame();
		});

		Button button2({h,0}, {h,h}, theFont, ">");
		button2.set_on_click([&](){
			if (theFile.ignoreRevsList.size() == 0) {
				return;
			}

			theFile.ignoreRevsList.pop_back();
			updateGitBlame();
		});

		while (window.isOpen()) {
			sf::Event event;
			while (window.pollEvent(event)) {
				button1.update(event);
				button2.update(event);

				bool ctrlDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

				switch (event.type) {
					case sf::Event::Closed:
						window.close();
						break;
					case sf::Event::Resized:
						{
							sf::FloatRect visibleArea(0.0f, 0.0f, event.size.width, event.size.height);
							window.setView(sf::View(visibleArea));

							leftDividerX = std::min(leftDividerX, std::max(0, int(window.getSize().x) - LEFT_DIVIDER_FROM_RIGHT));
							//leftDividerX = std::max(LEFT_DIVIDER_FROM_LEFT, leftDividerX - LEFT_DIVIDER_FROM_RIGHT);
							leftDividerX = std::max(leftDividerX, LEFT_DIVIDER_FROM_LEFT);

							rightDividerX = std::min(rightDividerX, std::max(0, int(window.getSize().x) - LEFT_DIVIDER_FROM_RIGHT));
							rightDividerX = std::max(leftDividerX + LEFT_DIVIDER_FROM_LEFT, rightDividerX);
						}
						break;
					case sf::Event::KeyPressed:
						switch (event.key.code) {
							case sf::Keyboard::Escape:
							case sf::Keyboard::Q:
								window.close();
								break;
							case sf::Keyboard::Home:
								if (ctrlDown) {
									theFile.scrollPositionPixels = 0;
								}
								break;
							case sf::Keyboard::End:
								if (ctrlDown) {
									float step = (fontSizePixels + fontSizePixels/2);
									theFile.scrollPositionPixels = std::max(0, int(theFile.textLines.size() * step - window.getSize().y));
								}
								break;
							case sf::Keyboard::Add: // The '+' key
							case sf::Keyboard::Unknown: // My '+' key isn't recognized, and we don't have event.key.scancode in SFML 2.5.1
								if (ctrlDown) {
									zoom(fontSizePixels + 1, theFile);
								}
								break;
							case sf::Keyboard::Hyphen: // The '-' key
								if (ctrlDown) {
									zoom(fontSizePixels - 1, theFile);
								}
								break;
						}
						break;
					case sf::Event::MouseWheelScrolled:
						if (! (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl))) {
							// Scrolling
							theFile.scrollPositionPixels -= event.mouseWheelScroll.delta * 60;
							if (theFile.scrollPositionPixels < 0) {
								theFile.scrollPositionPixels = 0;
							}
							break;
						}

						// Zooming
						zoom(fontSizePixels + event.mouseWheelScroll.delta, theFile);
						break;
					case sf::Event::MouseButtonPressed:
						if (event.mouseButton.button != sf::Mouse::Left) {
							break;
						}

						if (event.mouseButton.y > topbarHeight && within(event.mouseButton.x, leftDividerX-15, leftDividerX+15)) {
							movingLeftDivider = true;
						} else if (event.mouseButton.y > topbarHeight && within(event.mouseButton.x, rightDividerX-15, rightDividerX+15)) {
							movingRightDivider = true;
						}
						break;
					case sf::Event::MouseButtonReleased:
						if (event.mouseButton.button == sf::Mouse::Left) {
							movingLeftDivider = false;
							movingRightDivider = false;
						}
						break;
					case sf::Event::MouseMoved:
						if (movingLeftDivider || (event.mouseMove.y > topbarHeight && within(event.mouseMove.x, leftDividerX-15, leftDividerX+15))) {
							leftDividerRect.setFillColor(sf::Color(220,220,220));
						} else {
							leftDividerRect.setFillColor(sf::Color(100,100,100));
						}

						if (movingRightDivider || (event.mouseMove.y > topbarHeight && within(event.mouseMove.x, rightDividerX-15, rightDividerX+15))) {
							rightDividerRect.setFillColor(sf::Color(220,220,220));
						} else {
							rightDividerRect.setFillColor(sf::Color(100,100,100));
						}

						if (movingLeftDivider) {
							leftDividerX = std::max(LEFT_DIVIDER_FROM_LEFT, event.mouseMove.x);
							//leftDividerX = std::min(leftDividerX, std::max(0, int(window.getSize().x) - LEFT_DIVIDER_FROM_RIGHT));
							leftDividerX = std::min(leftDividerX, rightDividerX - LEFT_DIVIDER_FROM_RIGHT);
						} else if (movingRightDivider) {
							rightDividerX = std::max(leftDividerX + LEFT_DIVIDER_FROM_LEFT, event.mouseMove.x);
							rightDividerX = std::min(rightDividerX, std::max(0, int(window.getSize().x) - LEFT_DIVIDER_FROM_RIGHT));
						}
						break;
					default:
						break;
				}
			}

			window.clear(sf::Color(10,10,10));

			button1.set_size(topbarHeight, topbarHeight);
			button2.set_size(topbarHeight, topbarHeight);

			float yOffset = theFile.scrollPositionPixels % fontSizePixels - topbarHeight;
			//int startIdx = std::max(0, int(std::floor(theFile.scrollPositionPixels / fontSizePixels)));
			float step = (fontSizePixels + fontSizePixels/2);
			int startIdx = std::max(0, int(std::floor(theFile.scrollPositionPixels / step)));

			for (int i = startIdx; i < theFile.textLines.size(); i++) {
				int iFromZero = i - startIdx;
				float y = iFromZero * step;

				theFile.blameBgs[i].setSize(sf::Vector2f(leftDividerX, step));
				theFile.blameBgs[i].setPosition(0, y - yOffset);
				window.draw(theFile.blameBgs[i]);

				theFile.authorLines[i].setPosition(0, y - yOffset);
				window.draw(theFile.authorLines[i]);

				sf::RectangleShape rect;
				rect.setSize(sf::Vector2f(window.getSize().x, step));
				rect.setPosition(leftDividerX+2, y - yOffset);
				sf::Color c = theFile.blameBgs[i].getFillColor();
				c.r /= 5;
				c.g /= 5;
				c.b /= 5;
				rect.setFillColor(c);
				window.draw(rect);

				theFile.textLines[i].setPosition(leftDividerX+10, y - yOffset);
				window.draw(theFile.textLines[i]);

				if (y + step > window.getSize().y) {
					break;
				}
			}

			sf::RectangleShape gitLogBGRect;
			gitLogBGRect.setPosition(rightDividerX, 0);
			gitLogBGRect.setSize(sf::Vector2f(window.getSize().x - rightDividerX, window.getSize().y));
			gitLogBGRect.setFillColor(sf::Color(30,30,30));
			window.draw(gitLogBGRect);
			for (int i = 0; i < theFile.commitTexts.size(); i++) {
				auto &e = theFile.commitTexts[i];
				float x = rightDividerX;
				float y = topbarHeight + i * (step-4);
				e.timeText.setPosition(x,y);
				e.authorText.setPosition(x+100,y);
				e.commitHashText.setPosition(x,y);
				e.titleText.setPosition(x+300,y);

				window.draw(e.authorText);
				window.draw(e.timeText);
				//window.draw(e.commitHashText);
				window.draw(e.titleText);
			}

			sf::VertexArray topbarRect(sf::TriangleStrip, 4);
			topbarRect[0].position = sf::Vector2f(0,0);
			topbarRect[1].position = sf::Vector2f(window.getSize().x,0);
			topbarRect[2].position = sf::Vector2f(0,topbarHeight);
			topbarRect[3].position = sf::Vector2f(window.getSize().x, topbarHeight);

			topbarRect[0].color = sf::Color(50,50,50);
			topbarRect[2].color = sf::Color(50,50,50);
			topbarRect[1].color = sf::Color(10,10,10);
			topbarRect[3].color = sf::Color(10,10,10);

			window.draw(topbarRect);

			leftDividerRect.setSize(sf::Vector2f(2, window.getSize().y));
			leftDividerRect.setPosition(leftDividerX, topbarHeight);

			rightDividerRect.setSize(sf::Vector2f(2, window.getSize().y));
			rightDividerRect.setPosition(rightDividerX, topbarHeight);

			window.draw(leftDividerRect);
			window.draw(rightDividerRect);

			button1.draw(window);
			button2.draw(window);

			window.display();
		}

		return 0;
	}
};

#endif // WHODUNNIT_HPP

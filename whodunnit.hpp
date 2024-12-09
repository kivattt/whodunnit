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
#include "colors.hpp"

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

	double committer_time_0_to_1(unsigned long long committerTime) {
		double zeroToOne = double(committerTime - oldestCommitterTime) / double(newestCommitterTime - oldestCommitterTime);
		return zeroToOne;
	}

	int scrollPositionPixels = 0;
	vector<sf::Text> textLines;
	vector<sf::Text> authorLines;
	vector<sf::RectangleShape> blameBgs;
	vector<sf::RectangleShape> gitLogBgs;

	void set_texts() {
		textLines.clear();
		authorLines.clear();
		blameBgs.clear();
		commitTexts.clear();
		gitLogBgs.clear();

		std::map<string, bool> commitsInBlame;

		for (BlameLine &e : blameLines) {
			commitsInBlame[e.commitHash] = true;

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
				//text.setFillColor(sf::Color(30,30,30));
				text.setFillColor(sf::Color(200,200,200));
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
				//rect.setFillColor(hsv_to_rgb(130, 0.50, 1 - 0.3));
				rect.setFillColor(hsv_to_rgb(130, 0.50, 0.4));
			} else {
				double zeroToOne = committer_time_0_to_1(e.committerTime);
				rect.setFillColor(hsv_to_rgb(221, zeroToOne*0.65, 0.7d * (0.3 + zeroToOne * (1 - 0.3))));
			}
			blameBgs.push_back(rect);
		}

		int i = 0;
		for (Commit &c : commitLog) {
			++i;
			CommitThing thing;
			thing.authorText.setFont(theFont);
			thing.timeText.setFont(theFont);
			thing.commitHashText.setFont(theFont);
			thing.titleText.setFont(theFont);

			int size = std::max(1, fontSizePixels-1);
			thing.authorText.setCharacterSize(size);
			thing.timeText.setCharacterSize(size);
			thing.commitHashText.setCharacterSize(size);
			thing.titleText.setCharacterSize(size);

			sf::Color textColor = commitsInBlame.contains(c.commitHash) ? gitLogTextColor : gitLogTextColorDarker;
			thing.authorText.setFillColor(textColor);
			thing.timeText.setFillColor(textColor);
			thing.commitHashText.setFillColor(textColor);
			thing.titleText.setFillColor(textColor);

			thing.authorText.setString(sf::String::fromUtf8(c.author.begin(), c.author.end()));
			thing.timeText.setString(c.time);
			thing.commitHashText.setString(c.commitHash);
			thing.titleText.setString(sf::String::fromUtf8(c.title.begin(), c.title.end()));

			commitTexts.push_back(thing);

			sf::RectangleShape rect;
			rect.setFillColor(gitLogBackgroundColor);
			rect.setFillColor(i & 1 ? gitLogBackgroundColor : gitLogBackgroundColorAlternate);
			gitLogBgs.push_back(rect);
		}
	}

	// Returns false on failure
	bool run_git_log(string filename) {
		char tempFilename[] = "/tmp/whodunnit-XXXXXX";
		int fd = mkstemp(tempFilename);
		if (fd == -1) {
			return false;
		}
		close(fd);

		char *previousDirName = get_current_dir_name();
		if (chdir(parent_dir(filename).c_str()) == -1) {
			free(previousDirName);
			return false;
		}

		//int exitCode = system(string("git log --pretty=format:\"%an%n%at %H %s\" " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		int exitCode = system(string("git log --pretty=format:\"%an%n%as %H %s\" " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		if (exitCode != 0) {
			free(previousDirName);
			return false;
		}

		if (chdir(previousDirName) == -1) {
			free(previousDirName);
			return false;
		}

		commitLog.clear();

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

				commitLog.push_back(currentCommit);
				currentCommit = Commit();
			}

			lookingForAuthor = !lookingForAuthor;
		}

		// Delete the temp file
		unlink(tempFilename);

		free(previousDirName);
		return true;
	}


	// Run git blame on filename, with known revisions from commitLog
	// Returns false on failure
	bool run_git_blame(string filename) {
		auto start = std::chrono::high_resolution_clock::now();

		char tempFilename[] = "/tmp/whodunnit-XXXXXX";
		int fd = mkstemp(tempFilename);
		if (fd == -1) {
			return false;
		}
		close(fd);

		// Temporary file for -S <revs-file>
		char tempRevsFilename[] = "/tmp/whodunnit-r-XXXXXX";
		int revsFd = mkstemp(tempRevsFilename);
		if (revsFd == -1) {
			return false;
		}
		close(revsFd);

		std::ofstream revsFile(tempRevsFilename, std::ofstream::out | std::ofstream::trunc);
		for (const Commit &c : commitLog) {
			revsFile << c.commitHash << '\n';
		}
		revsFile.close();

		char *previousDirName = get_current_dir_name();
		if (chdir(parent_dir(filename).c_str()) == -1) {
			free(previousDirName);
			return false;
		}

		//int exitCode = system(string("git blame --line-porcelain -t -S " + string(tempRevsFilename) + " " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		int exitCode = system(string("git blame --line-porcelain -t " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		if (exitCode != 0) {
			free(previousDirName);
			return false;
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto msInt = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
		std::cout << "Git blame time: " << msInt.count() << "ms" << std::endl;

		if (chdir(previousDirName) == -1) {
			free(previousDirName);
			return false;
		}

		std::ifstream file(tempFilename);

		blameLines.clear();
		oldestCommitterTime = ULLONG_MAX;
		newestCommitterTime = 0;

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
				blameLines.push_back(currentBlameLine);
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

				if (time < oldestCommitterTime) {
					oldestCommitterTime = time;
					oldestCommitHash = currentBlameLine.commitHash;
				}

				if (time > newestCommitterTime) {
					newestCommitterTime = time;
					newestCommitHash = currentBlameLine.commitHash;
				}

				currentBlameLine.committerTime = time;
				continue;
			}
		}

		std::cout << "newest: " << newestCommitHash << '\n';

		// Delete the temp file
		unlink(tempFilename);
		// Delete the temporary file for -S <revs-file>
		unlink(tempRevsFilename);

		free(previousDirName);

		set_texts();
		return true;
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

		int size = std::max(1, fontSizePixels-1);
		// TODO: Maybe make separate zoom for git log
		for (CommitThing &c : theFile.commitTexts) {
			c.authorText.setCharacterSize(size);
			c.timeText.setCharacterSize(size);
			c.commitHashText.setCharacterSize(size);
			c.titleText.setCharacterSize(size);
		}
	}

	int run(string filename) {
		BlameFile theFile;
		if (! theFile.run_git_log(filename)) {
			std::cerr << "Failed to run git log\n";
			return 1;
		}

		if (! theFile.run_git_blame(filename)) {
			std::cerr << "Failed to run git blame\n";
			return 1;
		}
		/*std::optional<BlameFile> blameFile = run_git_blame(filename, {});
		if (! blameFile) {
			std::cerr << "Failed to run git blame\n";
			return 1;
		}*/

		if (! theFont.loadFromFile("fonts/JetBrainsMono-Regular.ttf")) {
			std::cerr << "Failed to load font at fonts/JetBrainsMono-Regular.ttf";
			return 1;
		}

		//BlameFile theFile = blameFile.value();
		theFile.set_texts();

		for (Commit &c : theFile.commitLog) {
			std::cout << c.time << ' ' << c.author << ' ' << c.commitHash << ' ' << c.title << '\n';
		}

		sf::RenderWindow window(sf::VideoMode(START_WIDTH, START_HEIGHT), "whodunnit - " + basename(filename));
		window.setVerticalSyncEnabled(true);

		sf::RectangleShape leftDividerRect;
		leftDividerRect.setFillColor(dividerColor);

		sf::RectangleShape rightDividerRect;
		rightDividerRect.setFillColor(dividerColor);

		/*sf::RectangleShape topbarRect;
		topbarRect.setPosition(0,0);
		topbarRect.setSize(sf::Vector2f(window.getSize().x, topbarHeight));
		topbarRect.setFillColor(sf::Color(160,160,160));*/

		auto updateGitBlame = [&]() {
			if (! theFile.run_git_blame(filename)) {
				std::cerr << "Failed to run git blame\n";
				return;
			}
			/*std::optional<BlameFile> blameFile = run_git_blame(filename, theFile.ignoreRevsList);
			if (! blameFile) {
				std::cerr << "Failed to run git blame\n";
				return;
			}*/
			//int lastScrollPositionPixels = theFile.scrollPositionPixels;
			//theFile = blameFile.value();
			//theFile.scrollPositionPixels = lastScrollPositionPixels;
			theFile.set_texts();
		};

		int topbarHeight = 35;
		int gitLogTopBarHeight = 31;

		sf::Text gitLogTopBarTitleText;
		gitLogTopBarTitleText.setFont(theFont);
		gitLogTopBarTitleText.setString("Git Log");
		gitLogTopBarTitleText.setFillColor(paneTitleTextColor);

		float h = topbarHeight;
		Button button1({0,0}, {h,h}, theFont, "<");
		button1.set_on_click([&](){
			updateGitBlame();
		});

		Button button2({h,0}, {h,h}, theFont, ">");
		button2.set_on_click([&](){
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
						leftDividerRect.setFillColor(dividerColor);
						rightDividerRect.setFillColor(dividerColor);

						if (movingLeftDivider) {
							leftDividerRect.setFillColor(dividerColorHighlight);
						} else if (movingRightDivider) {
							rightDividerRect.setFillColor(dividerColorHighlight);
						} else {
							if (event.mouseMove.y > topbarHeight && within(event.mouseMove.x, leftDividerX-15, leftDividerX+15)) {
								leftDividerRect.setFillColor(dividerColorHighlight);
							} else if (event.mouseMove.y > topbarHeight && within(event.mouseMove.x, rightDividerX-15, rightDividerX+15)) {
								rightDividerRect.setFillColor(dividerColorHighlight);
							}
						}

						if (movingLeftDivider) {
							leftDividerX = std::max(LEFT_DIVIDER_FROM_LEFT, event.mouseMove.x);
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
			gitLogBGRect.setFillColor(gitLogBackgroundColor);
			window.draw(gitLogBGRect);
			//float gitLogStep = fontSizePixels * 1.1;
			int gitLogStep = (float)fontSizePixels * 1.3;
			for (int i = 0; i < theFile.commitTexts.size(); i++) {
				auto &e = theFile.commitTexts[i];
				float x = rightDividerX+5;
				//float y = int(topbarHeight + i * gitLogStep);
				float y = topbarHeight + gitLogTopBarHeight + i * gitLogStep;

				theFile.gitLogBgs[i].setSize(sf::Vector2f(window.getSize().x - rightDividerX, gitLogStep));
				theFile.gitLogBgs[i].setPosition(rightDividerX, y);
				window.draw(theFile.gitLogBgs[i]);

				e.timeText.setPosition(x,y);
				e.authorText.setPosition(x+100,y);
				e.commitHashText.setPosition(x,y);
				e.titleText.setPosition(x+220,y);

				window.draw(e.authorText);
				window.draw(e.timeText);
				//window.draw(e.commitHashText);
				window.draw(e.titleText);
			}

			sf::RectangleShape gitLogTopBarDivider;
			gitLogTopBarDivider.setPosition(rightDividerX,topbarHeight+gitLogTopBarHeight-1);
			gitLogTopBarDivider.setSize(sf::Vector2f(window.getSize().x - rightDividerX, 1));
			gitLogTopBarDivider.setFillColor(dividerColor);
			window.draw(gitLogTopBarDivider);

			gitLogTopBarTitleText.setCharacterSize(15);
			gitLogTopBarTitleText.setPosition(rightDividerX + 7, topbarHeight + 7);
			window.draw(gitLogTopBarTitleText);

			sf::VertexArray topbarRect(sf::TriangleStrip, 4);
			topbarRect[0].position = sf::Vector2f(0,0);
			topbarRect[1].position = sf::Vector2f(window.getSize().x,0);
			topbarRect[2].position = sf::Vector2f(0,topbarHeight);
			topbarRect[3].position = sf::Vector2f(window.getSize().x, topbarHeight);

			topbarRect[0].color = sf::Color(50,50,50);
			topbarRect[2].color = sf::Color(50,50,50);
			topbarRect[1].color = sf::Color(10,10,10);
			topbarRect[3].color = sf::Color(10,10,10);

			sf::RectangleShape topbarDivider;
			topbarDivider.setPosition(0,topbarHeight-1);
			topbarDivider.setSize(sf::Vector2f(window.getSize().x, 1));
			topbarDivider.setFillColor(dividerColor);

			window.draw(topbarRect);
			window.draw(topbarDivider);

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

#ifndef WHODUNNIT_HPP
#define WHODUNNIT_HPP

#include <iostream>
#include <algorithm>
#include <optional>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <climits>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <SFML/Graphics.hpp>

#include "util.hpp"
#include "button.hpp"
#include "colors.hpp"
#include "rightclickmenu.hpp"

#define START_WIDTH 1280
#define START_HEIGHT 720
#define LEFT_DIVIDER_FROM_RIGHT 100
#define LEFT_DIVIDER_FROM_LEFT 100
#define GIT_LOG_ENTRY_HEIGHT_MULTIPLIER 1.3

using std::string;
using std::vector;

const int nBackgroundsPerGitLogEntry = 4; // 3 text field backgrounds, and a 1-pixel in height separator background

sf::Font monospaceFont;
sf::Font interFont;
int fontSizePixels = 15;

struct BlameLine {
	string commitHash;
	string author;
	string line;

	unsigned long long committerTime = 0;
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

	CommitThing() : authorText(interFont), timeText(monospaceFont), commitHashText(monospaceFont), titleText(interFont) {}
};

struct BlameFile {
	vector<sf::Text> textLines;
	vector<sf::Text> authorLines;
	sf::VertexArray blameBgsVertexArray;
	sf::VertexArray sourceCodeBgsVertexArray;
	sf::VertexArray gitLogBgsVertexArray;
	vector<BlameLine> blameLines;
	vector<Commit> commitLog;
	vector<CommitThing> commitTexts;

	string filename = "";
	string oldestCommitHash = "";
	string newestCommitHash = "";
	string selectedCommitHash = "";

	void set_filename(string newFilename) {
		filename = newFilename;
	}

	double committer_time_0_to_1(unsigned long long committerTime) {
		double zeroToOne = double(committerTime - oldestCommitterTime) / double(newestCommitterTime - oldestCommitterTime);
		return zeroToOne;
	}

	unsigned long long oldestCommitterTime = ULLONG_MAX;
	unsigned long long newestCommitterTime = 0;
	
	int scrollPositionPixels = 0;
	int gitLogScrollPositionPixels = 0;

	// Returns -1 on error
	int mouse_y_to_blame_line_index(float mouseY, int topbarHeight) {
		float step = (fontSizePixels + fontSizePixels/2);
		float yOffset = scrollPositionPixels % int(step) - topbarHeight;
		int startIdx = std::max(0, int(std::floor(scrollPositionPixels / step)));

		int ret = startIdx + (mouseY + yOffset) / step;
		if (ret < startIdx) {
			return -1;
		}

		if (ret >= blameLines.size()) {
			return -1;
		}

		return ret;
	}

	// Returns -1 on error
	// FIXME: Doesn't work, also fix scrolling for the Git Log window
	int mouse_y_to_git_log_index(float mouseY, int topbarHeight) {
		float gitLogStep = (float)fontSizePixels * GIT_LOG_ENTRY_HEIGHT_MULTIPLIER;
		float gitLogYOffset = gitLogScrollPositionPixels % int(gitLogStep) - topbarHeight;
		int startIdx = std::max(0, int(std::floor(gitLogScrollPositionPixels / gitLogStep)));

		int ret = startIdx + (mouseY + gitLogYOffset) / gitLogStep;
		if (ret < startIdx) {
			return -1;
		}

		if (ret >= commitLog.size()) {
			return -1;
		}

		return ret;
	}

	void set_texts() {
		textLines.clear();
		authorLines.clear();
		blameBgsVertexArray.clear();
		sourceCodeBgsVertexArray.clear();
		commitTexts.clear();
		gitLogBgsVertexArray.clear();

		std::map<string, bool> commitsInBlame;

		blameBgsVertexArray.resize(6 * blameLines.size());
		sourceCodeBgsVertexArray.resize(6 * blameLines.size());
		int i = 0;
		for (BlameLine &e : blameLines) {
			commitsInBlame[e.commitHash] = true;

			sf::Text text(monospaceFont);
			text.setString(sf::String::fromUtf8(e.line.begin(), e.line.end()));
			text.setCharacterSize(fontSizePixels);
			//text.setFillColor(sf::Color(200,200,200));
			text.setFillColor(genericTextColor);
			textLines.push_back(text);

			text.setFont(interFont);
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
			
			sf::Color rectColor;

			if (e.commitHash == selectedCommitHash) {
				rectColor = hsv_to_rgb(200, 0.60, 0.65);
			} else if (e.commitHash == newestCommitHash) {
				//rect.setFillColor(hsv_to_rgb(130, 0.50, 1 - 0.3));
				rectColor = latestCommitColor;
			} else {
				double zeroToOne = committer_time_0_to_1(e.committerTime);
				rectColor = hsv_to_rgb(221, zeroToOne*0.65, 0.7d * (0.3 + zeroToOne * (1 - 0.3)));
			}

			blameBgsVertexArray[i+0].color = rectColor;
			blameBgsVertexArray[i+1].color = rectColor;
			blameBgsVertexArray[i+2].color = rectColor;
			blameBgsVertexArray[i+3].color = rectColor;
			blameBgsVertexArray[i+4].color = rectColor;
			blameBgsVertexArray[i+5].color = rectColor;

			sf::Color rectColorDim = rectColor;
			rectColorDim.r /= 5;
			rectColorDim.g /= 5;
			rectColorDim.b /= 5;
			sourceCodeBgsVertexArray[i+0].color = rectColorDim;
			sourceCodeBgsVertexArray[i+1].color = rectColorDim;
			sourceCodeBgsVertexArray[i+2].color = rectColorDim;
			sourceCodeBgsVertexArray[i+3].color = rectColorDim;
			sourceCodeBgsVertexArray[i+4].color = rectColorDim;
			sourceCodeBgsVertexArray[i+5].color = rectColorDim;
			i += 6;
		}

		i = 0;
		gitLogBgsVertexArray.resize(nBackgroundsPerGitLogEntry * 6 * commitLog.size());
		bool alternatingColor = false;
		for (Commit &c : commitLog) {
			alternatingColor = !alternatingColor;
			CommitThing thing;

			int size = std::max(1, fontSizePixels-1);
			thing.authorText.setCharacterSize(size);
			thing.timeText.setCharacterSize(size);
			thing.commitHashText.setCharacterSize(size);
			thing.titleText.setCharacterSize(size);

			sf::Color textColor = commitsInBlame.contains(c.commitHash) ? genericTextColor : gitLogTextColorDarker;
			thing.authorText.setFillColor(textColor);
			thing.timeText.setFillColor(textColor);
			thing.commitHashText.setFillColor(textColor);
			thing.titleText.setFillColor(textColor);

			thing.authorText.setString(sf::String::fromUtf8(c.author.begin(), c.author.end()));
			thing.timeText.setString(c.time);
			thing.commitHashText.setString(c.commitHash);
			thing.titleText.setString(sf::String::fromUtf8(c.title.begin(), c.title.end()));

			commitTexts.push_back(thing);

			sf::Color color = alternatingColor ? gitLogBackgroundColor : gitLogBackgroundColorAlternate;
			if (c.commitHash == selectedCommitHash) {
				color = hsv_to_rgb(200, 0.60, 0.65);
			}
			for (int j = 0; j < nBackgroundsPerGitLogEntry; j++) {
				gitLogBgsVertexArray[i + 6*j + 0].color = color;
				gitLogBgsVertexArray[i + 6*j + 1].color = color;
				gitLogBgsVertexArray[i + 6*j + 2].color = color;
				gitLogBgsVertexArray[i + 6*j + 3].color = color;
				gitLogBgsVertexArray[i + 6*j + 4].color = color;
				gitLogBgsVertexArray[i + 6*j + 5].color = color;
			}

			i += 6 * nBackgroundsPerGitLogEntry;
		}
	}

	// Returns false on failure
	bool run_git_log() {
		char tempFilename[] = "/tmp/whodunnit-XXXXXX";
		int fd = mkstemp(tempFilename);
		if (fd == -1) {
			return false;
		}
		close(fd);

		char *previousDirName = get_current_dir_name();
		if (chdir(absolute_path(parent_path(filename)).c_str()) == -1) {
			free(previousDirName);
			return false;
		}

		//int exitCode = system(string("git log --pretty=format:\"%an%n%at %H %s\" " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		int exitCode = system(string("git log --pretty=format:\"%an%n%as %H %s\" " + sanitize_shell_argument(basename(filename)) + " > " + tempFilename).c_str());
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
	bool run_git_blame() {
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
		if (chdir(absolute_path(parent_path(filename)).c_str()) == -1) {
			free(previousDirName);
			return false;
		}

		//int exitCode = system(string("git blame --line-porcelain -t -S " + string(tempRevsFilename) + " " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		int exitCode = system(string("git blame --line-porcelain -t " + sanitize_shell_argument(basename(filename)) + " > " + tempFilename).c_str());
		//int exitCode = system(string("git blame " + oldestCommitHash + " --line-porcelain -t " + sanitize_shell_argument(filename) + " > " + tempFilename).c_str());
		if (exitCode != 0) {
			free(previousDirName);
			return false;
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto msInt = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
		std::cout << "Git blame took " << msInt.count() << "ms for " << basename(filename) << std::endl;

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

	vector <BlameFile> blameFiles;
	BlameFile *theFile;

	int tabIndex = 0;

	int leftDividerX = 190;
	int rightDividerX = START_WIDTH - 400;
	bool movingLeftDivider = false;
	bool movingRightDivider = false;

	void zoom(int level) {
		// TODO: Make it logarithmic or whatever so the zoom feels intuitive
		fontSizePixels = std::max(1, level);
		fontSizePixels = std::min(100, fontSizePixels); // Going higher will use ridiculous amounts of memory

		for (sf::Text &text : theFile->textLines) {
			text.setCharacterSize(fontSizePixels);
		}
		for (sf::Text &text : theFile->authorLines) {
			text.setCharacterSize(fontSizePixels);
		}

		int size = std::max(1, fontSizePixels-1);
		// TODO: Maybe make separate zoom for git log
		for (CommitThing &c : theFile->commitTexts) {
			c.authorText.setCharacterSize(size);
			c.timeText.setCharacterSize(size);
			c.commitHashText.setCharacterSize(size);
			c.titleText.setCharacterSize(size);
		}
	}

	void switchToTab(sf::RenderWindow &window) {
		theFile = &blameFiles[tabIndex];
		window.setTitle("whodunnit - " + basename(theFile->filename));
	}

	int run(vector<string> filenames) {
		if (filenames.empty()) {
			return 1;
		}

		if (! monospaceFont.openFromFile("fonts/JetBrainsMono-Regular/JetBrainsMono-Regular.ttf")) {
			std::cerr << "Failed to load font at fonts/JetBrainsMono-Regular/JetBrainsMono-Regular.ttf";
			return 1;
		}

		if (! interFont.openFromFile("fonts/Inter/Inter-Regular.ttf")) {
			std::cerr << "Failed to load font at fonts/Inter/Inter-Regular.ttf";
			return 1;
		}

		for (string &filename : filenames) {
			BlameFile b;
			b.set_filename(filename);

			if (! b.run_git_log()) {
				std::cerr << "Failed to run git log\n";
				return 1;
			}

			if (! b.run_git_blame()) {
				std::cerr << "Failed to run git blame for " << filename << std::endl;
				continue;
			}

			b.set_texts();
			blameFiles.push_back(b);
		}

		if (blameFiles.empty()) {
			std::cerr << "Failed to open any files\n";
			return 1;
		}

		tabIndex = 0;
		theFile = &blameFiles[tabIndex];

		sf::RenderWindow window(sf::VideoMode({START_WIDTH, START_HEIGHT}), "whodunnit - " + basename(theFile->filename));
		//window.setVerticalSyncEnabled(true);

		sf::RectangleShape leftDividerRect;
		leftDividerRect.setFillColor(dividerColor);

		sf::RectangleShape rightDividerRect;
		rightDividerRect.setFillColor(dividerColor);

		sf::Texture clipboardTxt;
		if (! clipboardTxt.loadFromFile("icons/clipboard.png")) {
			std::cerr << "Failed to load icons/clipboard.png" << std::endl;
			return 1;
		}
		sf::Sprite clipboardSpr(clipboardTxt);

		string remote = get_remote_url(theFile->filename);
		string remoteName = remote_url_to_site_name(remote);

		sf::Texture remoteTxt;
		if (! remoteTxt.loadFromFile("icons/" + lowercase(remoteName) + ".png")) {
			std::cerr << "Failed to load icons/" << lowercase(remoteName) << ".png" << std::endl;
			return 1;
		}
		sf::Sprite remoteSpr(remoteTxt);

		RightClickMenu rightClickMenu;
		rightClickMenu.add_button(monospaceFont, "Copy Revision Number", &clipboardSpr, [&](){
			if (theFile->selectedCommitHash != "") {
				sf::Clipboard::setString(theFile->selectedCommitHash);
			}
		});
		rightClickMenu.add_button(monospaceFont, "Open on " + remoteName, &remoteSpr, [&](){
			string remote = get_remote_url(theFile->filename);
			if (remote == "") {
				return;
			}

			if (remote.ends_with(".git")) {
				remote = remote.substr(0, remote.size()-4);
			}

			// Checked that it works for GitHub, Gitlab and Gitea
			string url = remote + "/commit/" + theFile->selectedCommitHash;
			int ignored = system(string("xdg-open " + sanitize_shell_argument(url)).c_str());
		});
		/*rightClickMenu.add_button(monospaceFont, "Checkout revision", nullptr, [&](){
			std::cout << "Checkout revision\n";
		});*/

		auto updateGitBlame = [&]() {
			if (! theFile->run_git_blame()) {
				std::cerr << "Failed to run git blame\n";
				return;
			}
			theFile->set_texts();
		};

		int topbarHeight = 35;
		int secondTopBarHeight = 31;
		int topbarFullHeight = topbarHeight + secondTopBarHeight;

		sf::Text gitLogTopBarTitleText(monospaceFont);
		gitLogTopBarTitleText.setString("Git Log");
		gitLogTopBarTitleText.setFillColor(paneTitleTextColor);

		float h = topbarHeight;
		Button button1({0,0}, {h,h}, monospaceFont, "<");
		button1.set_on_click([&](){
			updateGitBlame();
		});

		Button button2({h,0}, {h,h}, monospaceFont, ">");
		button2.set_on_click([&](){
			updateGitBlame();
		});

		sf::Clock clock;
		while (window.isOpen()) {
			while (const std::optional event = window.pollEvent()) {
				button1.update(event);
				button2.update(event);
				rightClickMenu.update(event);

				bool ctrlDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

				if (event->is<sf::Event::Closed>()) {
					window.close();
				} else if (const auto e = event->getIf<sf::Event::Resized>()) {
					sf::Vector2u size = e->size;

					sf::FloatRect visibleArea({0.0f, 0.0f}, {(float)size.x, (float)size.y});
					window.setView(sf::View(visibleArea));
					
					leftDividerX = std::min(leftDividerX, std::max(0, int(window.getSize().x) - LEFT_DIVIDER_FROM_RIGHT));
					//leftDividerX = std::max(LEFT_DIVIDER_FROM_LEFT, leftDividerX - LEFT_DIVIDER_FROM_RIGHT);
					leftDividerX = std::max(leftDividerX, LEFT_DIVIDER_FROM_LEFT);
					
					rightDividerX = std::min(rightDividerX, std::max(0, int(window.getSize().x) - LEFT_DIVIDER_FROM_RIGHT));
					rightDividerX = std::max(leftDividerX + LEFT_DIVIDER_FROM_LEFT, rightDividerX);
				} else if (const auto e = event->getIf<sf::Event::KeyPressed>()) {
					sf::Keyboard::Key keyCode = e->code;

					rightClickMenu.hide();

					switch (keyCode) {
						case sf::Keyboard::Key::Q:
							window.close();
							break;
						case sf::Keyboard::Key::Escape:
							theFile->selectedCommitHash = "";
							theFile->set_texts();
							break;
						case sf::Keyboard::Key::Home:
							if (ctrlDown) {
								theFile->scrollPositionPixels = 0;
							}
							break;
						case sf::Keyboard::Key::End:
							if (ctrlDown) {
								float step = (fontSizePixels + fontSizePixels/2);
								theFile->scrollPositionPixels = std::max(0, int(theFile->textLines.size() * step - window.getSize().y));
							}
							break;
						case sf::Keyboard::Key::Add: // The '+' key
						case sf::Keyboard::Key::Unknown: // FIXME: (we now have SFML 3.0.0) My '+' key isn't recognized, and we don't have event.key.scancode in SFML 2.5.1
							if (ctrlDown) {
								zoom(fontSizePixels + 1);
							}
							break;
						case sf::Keyboard::Key::Hyphen: // The '-' key
							if (ctrlDown) {
								zoom(fontSizePixels - 1);
							}
							break;
						case sf::Keyboard::Key::Tab: // Switching tabs
							if (! (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl))) {
								break;
							}

							if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift)) {
								--tabIndex;
								if (tabIndex < 0) {
									tabIndex = std::max(0, (int)blameFiles.size() - 1);
								}
							} else {
								tabIndex = (tabIndex + 1) % blameFiles.size();
							}
							switchToTab(window);
							zoom(fontSizePixels); // To update the font sizes
							break;
					}
				} else if (const auto e = event->getIf<sf::Event::MouseWheelScrolled>()) {
					sf::Vector2i position = e->position;

					rightClickMenu.hide();

					if (! (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl))) {
						// Scrolling
						if (position.x < rightDividerX) {
							theFile->scrollPositionPixels -= e->delta * 60;
							if (theFile->scrollPositionPixels < 0) {
								theFile->scrollPositionPixels = 0;
							}
						} else {
							theFile->gitLogScrollPositionPixels -= e->delta * 60;
							if (theFile->gitLogScrollPositionPixels < 0) {
								theFile->gitLogScrollPositionPixels = 0;
							}
						}
						break;
					}

					// Zooming
					zoom(fontSizePixels + e->delta);
				} else if (const auto e = event->getIf<sf::Event::MouseButtonPressed>()) {
					sf::Mouse::Button button = e->button;
					sf::Vector2i position = e->position;

					bool isLeftClick = button == sf::Mouse::Button::Left;
					bool isRightClick = button == sf::Mouse::Button::Right;
					if (rightClickMenu.is_visible()) {
						rightClickMenu.hide();
						break;
					}

					rightClickMenu.hide();
					
					if (position.y <= topbarFullHeight) {
						break;
					}

					if (isLeftClick && within(position.x, leftDividerX-15, leftDividerX+15)) {
						movingLeftDivider = true;
					} else if (isLeftClick && within(position.x, rightDividerX-15, rightDividerX+15)) {
						movingRightDivider = true;
					} else if (position.x < leftDividerX) { // Clicking a blame on the left
						//int index = theFile->mouse_y_to_blame_line_index(position.y, topbarHeight);
						int index = theFile->mouse_y_to_blame_line_index(position.y, topbarFullHeight);
						if (index == -1) {
							theFile->selectedCommitHash = "";
							theFile->set_texts();
							break;
						}

						theFile->selectedCommitHash = theFile->blameLines[index].commitHash;
						theFile->set_texts();
						if (isRightClick) {
							rightClickMenu.set_position(position.x, position.y);
							rightClickMenu.show();
						}
					} else if (position.y > (topbarFullHeight) && position.x > rightDividerX) { // Clicking a commit (Git Log) on the right
						int index = theFile->mouse_y_to_git_log_index(position.y, topbarFullHeight);
						if (index == -1) {
							theFile->selectedCommitHash = "";
							theFile->set_texts();
							break;
						}

						theFile->selectedCommitHash = theFile->commitLog[index].commitHash;
						theFile->set_texts();
						if (isRightClick) {
							rightClickMenu.set_position(position.x, position.y);
							rightClickMenu.show();
						}
					}
				} else if (const auto e = event->getIf<sf::Event::MouseButtonReleased>()) {
					sf::Mouse::Button button = e->button;
					if (button == sf::Mouse::Button::Left) {
						movingLeftDivider = false;
						movingRightDivider = false;
					}
				} else if (const auto e = event->getIf<sf::Event::MouseMoved>()) {
					sf::Vector2i position = e->position;

					leftDividerRect.setFillColor(dividerColor);
					rightDividerRect.setFillColor(dividerColor);

					if (rightClickMenu.is_visible()) {
						break;
					}

					if (movingLeftDivider) {
						leftDividerRect.setFillColor(dividerColorHighlight);
					} else if (movingRightDivider) {
						rightDividerRect.setFillColor(dividerColorHighlight);
					} else {
						//if (position.y > topbarHeight && within(position.x, leftDividerX-15, leftDividerX+15)) {
						if (position.y > topbarFullHeight && within(position.x, leftDividerX-15, leftDividerX+15)) {
							leftDividerRect.setFillColor(dividerColorHighlight);
						//} else if (position.y > topbarHeight && within(position.x, rightDividerX-15, rightDividerX+15)) {
						} else if (position.y > topbarFullHeight && within(position.x, rightDividerX-15, rightDividerX+15)) {
							rightDividerRect.setFillColor(dividerColorHighlight);
						}
					}

					if (movingLeftDivider) {
						leftDividerX = std::max(LEFT_DIVIDER_FROM_LEFT, position.x);
						leftDividerX = std::min(leftDividerX, rightDividerX - LEFT_DIVIDER_FROM_RIGHT);
					} else if (movingRightDivider) {
						rightDividerX = std::max(leftDividerX + LEFT_DIVIDER_FROM_LEFT, position.x);
						rightDividerX = std::min(rightDividerX, std::max(0, int(window.getSize().x) - LEFT_DIVIDER_FROM_RIGHT));
					}
				}
			}

			window.clear(backgroundColor);

			button1.set_size(topbarHeight, topbarHeight);
			button2.set_size(topbarHeight, topbarHeight);

			float step = (fontSizePixels + fontSizePixels/2);
			//float yOffset = theFile->scrollPositionPixels % int(step) - topbarHeight;
			float yOffset = theFile->scrollPositionPixels % int(step) - topbarFullHeight;
			int startIdx = std::max(0, int(std::floor(theFile->scrollPositionPixels / step)));
			int numElements = std::min(int(theFile->textLines.size()-startIdx), std::max(0, int(std::ceil((window.getSize().y + yOffset) / step))));

			for (int i = startIdx; i < startIdx+numElements && i < theFile->textLines.size(); i++) {
				int iFromZero = i - startIdx;
				float y = iFromZero * step - yOffset;

				theFile->blameBgsVertexArray[i*6+0].position = sf::Vector2f(0, y);
				theFile->blameBgsVertexArray[i*6+1].position = sf::Vector2f(leftDividerX, y);
				theFile->blameBgsVertexArray[i*6+2].position = sf::Vector2f(0, y+step);
				theFile->blameBgsVertexArray[i*6+3].position = sf::Vector2f(0, y+step);
				theFile->blameBgsVertexArray[i*6+4].position = sf::Vector2f(leftDividerX, y);
				theFile->blameBgsVertexArray[i*6+5].position = sf::Vector2f(leftDividerX, y+step);

				theFile->sourceCodeBgsVertexArray[i*6+0].position = sf::Vector2f(leftDividerX+2, y);
				theFile->sourceCodeBgsVertexArray[i*6+1].position = sf::Vector2f(leftDividerX+2 + window.getSize().x, y);
				theFile->sourceCodeBgsVertexArray[i*6+2].position = sf::Vector2f(leftDividerX+2, y+step);
				theFile->sourceCodeBgsVertexArray[i*6+3].position = sf::Vector2f(leftDividerX+2, y+step);
				theFile->sourceCodeBgsVertexArray[i*6+4].position = sf::Vector2f(leftDividerX+2 + window.getSize().x, y);
				theFile->sourceCodeBgsVertexArray[i*6+5].position = sf::Vector2f(leftDividerX+2 + window.getSize().x, y+step);
			}

			// Just to make OpenGL shut up
			if (numElements > 0) {
				window.draw(&theFile->blameBgsVertexArray[6*startIdx], 6*numElements, sf::PrimitiveType::Triangles);
			}

			for (int i = startIdx; i < startIdx+numElements && i < theFile->textLines.size(); i++) {
				int iFromZero = i - startIdx;
				float y = iFromZero * step - yOffset;

				theFile->authorLines[i].setPosition({2, y});
				window.draw(theFile->authorLines[i]);
			}

			// Just to make OpenGL shut up
			if (numElements > 0) {
				window.draw(&theFile->sourceCodeBgsVertexArray[6*startIdx], 6*numElements, sf::PrimitiveType::Triangles);
			}

			for (int i = startIdx; i < startIdx+numElements && i < theFile->textLines.size(); i++) {
				int iFromZero = i - startIdx;
				float y = iFromZero * step - yOffset;

				theFile->textLines[i].setPosition({leftDividerX+10, y});
				window.draw(theFile->textLines[i]);
			}

			sf::RectangleShape topBarTabsRect;
			topBarTabsRect.setPosition({0, topbarHeight});
			topBarTabsRect.setSize(sf::Vector2f(window.getSize().x, secondTopBarHeight));
			topBarTabsRect.setFillColor(gitLogBackgroundColor);
			window.draw(topBarTabsRect);

			int currentTabXOffset = leftDividerX;
			const int tabPaddingX = 10;
			for (int i = 0; i < blameFiles.size(); i++) {
				sf::Text text(interFont);
				text.setString(basename(blameFiles[i].filename));
				text.setPosition({currentTabXOffset + tabPaddingX, topbarHeight + 7});
				text.setCharacterSize(13);

				sf::RectangleShape highlightRect;
				highlightRect.setFillColor(tabHighlightColor);

				sf::RectangleShape backgroundRect;
				backgroundRect.setFillColor(tabIndex == i ? sf::Color(30,30,40) : sf::Color(0,0,0));

				int lastCurrentTabXOffset = currentTabXOffset;
				currentTabXOffset += text.getGlobalBounds().size.x + 2*tabPaddingX;

				highlightRect.setPosition({currentTabXOffset, topbarFullHeight - 1 - 2});
				highlightRect.setSize(sf::Vector2f(lastCurrentTabXOffset - currentTabXOffset + 2, 2));
				backgroundRect.setPosition({currentTabXOffset, topbarHeight});
				backgroundRect.setSize(sf::Vector2f(lastCurrentTabXOffset - currentTabXOffset + 2, secondTopBarHeight - 1));

				window.draw(backgroundRect);
				if (tabIndex == i) {
					window.draw(highlightRect);
				}
				window.draw(text);

				sf::RectangleShape vertDividerRect;
				vertDividerRect.setPosition({currentTabXOffset, topbarHeight});
				vertDividerRect.setSize(sf::Vector2f(2, secondTopBarHeight));
				vertDividerRect.setFillColor(dividerColor); // TODO: Change to lower brightness?
				window.draw(vertDividerRect);
			}

			sf::RectangleShape gitLogBGRect;
			gitLogBGRect.setPosition({rightDividerX, 0});
			gitLogBGRect.setSize(sf::Vector2f(window.getSize().x - rightDividerX, window.getSize().y));
			gitLogBGRect.setFillColor(backgroundColor);
			window.draw(gitLogBGRect);

			float gitLogStep = (float)fontSizePixels * 1.3;
			float gitLogYOffset = theFile->gitLogScrollPositionPixels % int(gitLogStep) - topbarFullHeight;
			int gitLogStartIdx = std::max(0, int(std::floor(theFile->gitLogScrollPositionPixels / gitLogStep)));
			int numGitLogElements = std::min(int(theFile->commitTexts.size()-gitLogStartIdx), std::max(0, int(std::ceil((window.getSize().y + gitLogYOffset) / gitLogStep))));

			for (int i = gitLogStartIdx; i < gitLogStartIdx+numGitLogElements && i < theFile->commitTexts.size(); i++) {
				int iFromZero = i - gitLogStartIdx;
				float x = rightDividerX+5;
				int y = iFromZero * gitLogStep - gitLogYOffset;

				float timeTextBgX = rightDividerX;
				float timeTextBgY = y;
				float timeTextBgWidth = window.getSize().x - rightDividerX;
				float timeTextBgHeight = std::ceil(gitLogStep);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 0].position = sf::Vector2f(timeTextBgX, timeTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 1].position = sf::Vector2f(timeTextBgX + timeTextBgWidth, timeTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 2].position = sf::Vector2f(timeTextBgX, timeTextBgY +    timeTextBgHeight);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 3].position = sf::Vector2f(timeTextBgX, timeTextBgY +    timeTextBgHeight);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 4].position = sf::Vector2f(timeTextBgX + timeTextBgWidth, timeTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 5].position = sf::Vector2f(timeTextBgX + timeTextBgWidth, timeTextBgY + timeTextBgHeight);

				float authorTextBgX = rightDividerX+100;
				float authorTextBgY = y;
				float authorTextBgWidth = timeTextBgWidth;
				float authorTextBgHeight = timeTextBgHeight;
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i +  6].position = sf::Vector2f(authorTextBgX,  authorTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i +  7].position = sf::Vector2f(authorTextBgX + authorTextBgWidth, authorTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i +  8].position = sf::Vector2f(authorTextBgX, authorTextBgY +    authorTextBgHeight);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i +  9].position = sf::Vector2f(authorTextBgX, authorTextBgY +    authorTextBgHeight);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 10].position = sf::Vector2f(authorTextBgX + authorTextBgWidth, authorTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 11].position = sf::Vector2f(authorTextBgX + authorTextBgWidth, authorTextBgY + authorTextBgHeight);

				float titleTextBgX = rightDividerX+220;
				float titleTextBgY = y;
				float titleTextBgWidth = timeTextBgWidth;
				float titleTextBgHeight = timeTextBgHeight;
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 12].position = sf::Vector2f(titleTextBgX,  titleTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 13].position = sf::Vector2f(titleTextBgX + titleTextBgWidth, titleTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 14].position = sf::Vector2f(titleTextBgX, titleTextBgY +    titleTextBgHeight);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 15].position = sf::Vector2f(titleTextBgX, titleTextBgY +    titleTextBgHeight);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 16].position = sf::Vector2f(titleTextBgX + titleTextBgWidth, titleTextBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 17].position = sf::Vector2f(titleTextBgX + titleTextBgWidth, titleTextBgY + titleTextBgHeight);

				float bgLineBgX = rightDividerX;
				float bgLineBgY = y;
				float bgLineBgWidth = timeTextBgWidth;
				float bgLineBgHeight = 1;
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 18].position = sf::Vector2f(bgLineBgX,  bgLineBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 19].position = sf::Vector2f(bgLineBgX + bgLineBgWidth, bgLineBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 20].position = sf::Vector2f(bgLineBgX,  bgLineBgY +    bgLineBgHeight);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 21].position = sf::Vector2f(bgLineBgX,  bgLineBgY +    bgLineBgHeight);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 22].position = sf::Vector2f(bgLineBgX + bgLineBgWidth, bgLineBgY);
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 23].position = sf::Vector2f(bgLineBgX + bgLineBgWidth, bgLineBgY + bgLineBgHeight);

				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 18].color = backgroundColor;
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 19].color = backgroundColor;
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 20].color = backgroundColor;
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 21].color = backgroundColor;
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 22].color = backgroundColor;
				theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*i + 23].color = backgroundColor;
			}

			// Just to make OpenGL shut up
			if (numGitLogElements > 0) {
				window.draw(&theFile->gitLogBgsVertexArray[nBackgroundsPerGitLogEntry*6*gitLogStartIdx], nBackgroundsPerGitLogEntry*6*numGitLogElements, sf::PrimitiveType::Triangles);
			}

			//for (int i = gitLogStartIdx; i < gitLogStartIdx+numGitLogElements && i < theFile->commitTexts.size(); i++) {
			for (int i = gitLogStartIdx; i < theFile->commitTexts.size(); i++) {
				int iFromZero = i - gitLogStartIdx;
				float x = rightDividerX+5;
				int y = iFromZero * gitLogStep - gitLogYOffset;
				int textY = y + 2;
				if (fontSizePixels < 8) {
					textY = y;
				}

				auto &e = theFile->commitTexts[i];

				e.timeText.setPosition({x,textY});
				window.draw(e.timeText);

				e.authorText.setPosition({x+100,textY});
				window.draw(e.authorText);

				// XXX: Missing a gitLogBgsVertexArray thing, will need to increase nBackgroundsPerGitLogEntry by 1 to add it.
				//e.commitHashText.setPosition({x,textY});

				e.titleText.setPosition({x+220,textY});
				window.draw(e.titleText);
			}

			sf::RectangleShape topBarGitLogRect;
			topBarGitLogRect.setPosition({rightDividerX, topbarHeight});
			topBarGitLogRect.setSize(sf::Vector2f(window.getSize().x - rightDividerX, secondTopBarHeight));
			topBarGitLogRect.setFillColor(gitLogBackgroundColor);
			window.draw(topBarGitLogRect);

			sf::RectangleShape topBarFullDivider;
			topBarFullDivider.setPosition({0,topbarFullHeight-1});
			topBarFullDivider.setSize(sf::Vector2f(window.getSize().x, 1));
			topBarFullDivider.setFillColor(dividerColor);
			window.draw(topBarFullDivider);

			sf::RectangleShape leftVertDividerRect;
			leftVertDividerRect.setPosition({leftDividerX, topbarHeight});
			leftVertDividerRect.setSize(sf::Vector2f(2, secondTopBarHeight));
			leftVertDividerRect.setFillColor(dividerColor);
			window.draw(leftVertDividerRect);

			sf::RectangleShape gitLogVertDividerRect;
			gitLogVertDividerRect.setPosition({rightDividerX, topbarHeight});
			gitLogVertDividerRect.setSize(sf::Vector2f(2, secondTopBarHeight));
			gitLogVertDividerRect.setFillColor(dividerColor);
			window.draw(gitLogVertDividerRect);

			gitLogTopBarTitleText.setCharacterSize(15);
			gitLogTopBarTitleText.setPosition({rightDividerX + 7, topbarHeight + 7});
			window.draw(gitLogTopBarTitleText);

			// Gradient
			sf::VertexArray topbarRect(sf::PrimitiveType::TriangleStrip, 4);
			topbarRect[0].position = sf::Vector2f(0,0);
			topbarRect[1].position = sf::Vector2f(window.getSize().x,0);
			topbarRect[2].position = sf::Vector2f(0,topbarHeight);
			topbarRect[3].position = sf::Vector2f(window.getSize().x, topbarHeight);

			topbarRect[0].color = sf::Color(50,50,50);
			topbarRect[2].color = sf::Color(50,50,50);
			topbarRect[1].color = backgroundColor;
			topbarRect[3].color = backgroundColor;

			sf::RectangleShape topbarDivider;
			topbarDivider.setPosition({0,topbarHeight-1});
			topbarDivider.setSize(sf::Vector2f(window.getSize().x, 1));
			topbarDivider.setFillColor(dividerColor);

			window.draw(topbarRect);
			window.draw(topbarDivider);

			leftDividerRect.setSize(sf::Vector2f(2, window.getSize().y));
			leftDividerRect.setPosition({leftDividerX, topbarFullHeight});

			rightDividerRect.setSize(sf::Vector2f(2, window.getSize().y));
			rightDividerRect.setPosition({rightDividerX, topbarFullHeight});

			window.draw(leftDividerRect);
			window.draw(rightDividerRect);

			button1.draw(window);
			button2.draw(window);
			rightClickMenu.draw(window);

			window.display();
			//std::cout << 1000000.0f / clock.getElapsedTime().asMicroseconds() << '\n';
			clock.restart();
		}

		return 0;
	}
};

#endif // WHODUNNIT_HPP

#ifndef RIGHTCLICKMENU_HPP
#define RIGHTCLICKMENU_HPP

#include <algorithm>
#include <vector>
#include <string>
#include <SFML/Graphics.hpp>

#include "button.hpp"

using std::vector;
using std::string;

class RightClickMenu {
	private:
	const int buttonHeight = 25;
	sf::RectangleShape backgroundRect;
	vector<Button> buttons;

	float x = 0;
	float y = 0;
	float width = 250;

	bool visible = false;

	public:

	RightClickMenu() {
		backgroundRect.setFillColor(sf::Color(30,30,30));
		set_size(250);
	}

	bool is_visible() {
		return visible;
	}

	void set_size(float newWidth) {
		width = newWidth;

		// The height is atleast buttonHeight, so you don't get confused when the height is 0 when there are no buttons added
		float theHeight = std::max(buttonHeight, (int)buttons.size() * buttonHeight);
		backgroundRect.setSize(sf::Vector2f(width, theHeight));

		for (Button &button : buttons) {
			button.set_size(width, buttonHeight);
		}
	}

	void set_position(float newX, float newY) {
		x = newX;
		y = newY;

		backgroundRect.setPosition(x, y);

		for (int i = 0; i < buttons.size(); i++) {
			buttons[i].set_position(x, y + i * buttonHeight);
		}
	}

	void add_button(sf::Font &font, string text, std::function<void()> onClickFunction) {
		// The button position and size are set at the end
		Button button({0,0}, {0,0}, font, text);
		button.set_character_size(15);
		button.set_on_click(onClickFunction);
		button.set_click_on_release(false);
		button.set_text_y_offset(3);
		buttons.push_back(button);

		set_position(x, y);
		set_size(width);
	}

	void show() {
		visible = true;
	}

	void hide() {
		visible = false;
	}

	void update(sf::Event event) {
		if (!visible) {
			return;
		}

		for (Button &button : buttons) {
			button.update(event);
		}
	}

	void draw(sf::RenderWindow &window) {
		if (!visible) {
			return;
		}

		window.draw(backgroundRect);

		for (Button &button : buttons) {
			button.draw(window);
		}
	}
};

#endif // RIGHTCLICKMENU_HPP

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
	vector<sf::Sprite*> buttonsIcons;

	float x = 0;
	float y = 0;
	float width = 250;

	bool visible = false;

	public:

	RightClickMenu() {
		backgroundRect.setFillColor(rightClickMenuBackgroundColor);
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
		y = newY - buttonHeight/2;

		backgroundRect.setPosition(x, y);

		for (int i = 0; i < buttons.size(); i++) {
			buttons[i].set_position(x, y + i * buttonHeight);
		}

		int i = 0;
		for (sf::Sprite *spr : buttonsIcons) {
			if (spr == nullptr) {
				continue;
			}
			spr->setPosition(sf::Vector2f(x + 8, y + 3 + i * buttonHeight));
			++i;
		}
	}

	void add_button(sf::Font &font, string text, sf::Sprite *icon, std::function<void()> onClickFunction) {
		// The button position and size are set at the end
		Button button({0,0}, {0,0}, font, text);
		button.set_character_size(15);
		button.set_on_click(onClickFunction);
		button.set_click_on_press(true);
		button.set_text_y_offset(3);
		button.set_text_x_offset(30);
		buttons.push_back(button);
		buttonsIcons.push_back(icon);

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
			/*if (button.update(event)) {
				hide();
			}*/
		}
	}

	void draw(sf::RenderWindow &window) {
		if (!visible) {
			return;
		}

		float xx = x + 5;
		float yy = y + 5 + buttonHeight/2;
		float theWidth = width - 5;
		float height = (float)std::max(buttonHeight, (int)buttons.size() * buttonHeight);
		sf::VertexArray gradient(sf::TriangleStrip, 4);
		gradient[0].position = sf::Vector2f(xx, yy+height);
		gradient[1].position = sf::Vector2f(xx+theWidth, yy+height);
		gradient[2].position = sf::Vector2f(xx,yy);
		gradient[3].position = sf::Vector2f(xx+theWidth, yy);

		gradient[0].color = sf::Color(0,0,0, 0);
		gradient[1].color = sf::Color(0,0,0, 0);
		gradient[2].color = sf::Color(0,0,0, 255);
		gradient[3].color = sf::Color(0,0,0, 255);
		window.draw(gradient);

		window.draw(backgroundRect);

		for (Button &button : buttons) {
			button.draw(window);
		}

		for (sf::Sprite *spr : buttonsIcons) {
			if (spr == nullptr) {
				continue;
			}
			window.draw(*spr);
		}
	}
};

#endif // RIGHTCLICKMENU_HPP

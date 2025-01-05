#ifndef BUTTON_HPP
#define BUTTON_HPP

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

#include "util.hpp"

class Button{
	private:
	float x = 0;
	float y = 0;
	float width = 30;
	float height = 30;

	int textYOffset = -3;
	int textXOffset = 6;

	// TODO: Make this a rounded rectangle, see:
	// https://github.com/SFML/SFML/wiki/Source%3A-Draw-Rounded-Rectangle/_edit
	sf::RectangleShape backgroundRect;
	bool hovered = false;
	bool pressed = false;

	bool clickOnRelease = true;

	std::function<void()> onClickFunction;
	sf::Text theText;

	public:

	Button(sf::Vector2f position, sf::Vector2f size, sf::Font &font, string text) : theText(font) {
		x = position.x;
		y = position.y;
		theText.setPosition({x + textXOffset, y + textYOffset});
		backgroundRect.setPosition({x, y});

		width = size.x;
		height = size.y;
		backgroundRect.setSize(sf::Vector2f(width, height));

		theText.setFont(font);
		theText.setString(text);
		backgroundRect.setFillColor(sf::Color(100,100,100));
	}

	// By default, clicking is done when a mouse button is released.
	// By calling set_click_on_press(true), clicking is instead done when a mouse button is pressed.
	void set_click_on_press(bool shouldClickOnPress) {
		clickOnRelease = !shouldClickOnPress;
	}

	void set_character_size(int size) {
		theText.setCharacterSize(size);
	}

	void set_text_x_offset(int newTextXOffset) {
		textXOffset = newTextXOffset;
		theText.setPosition({x + textXOffset, y + textYOffset});
	}

	void set_text_y_offset(int newTextYOffset) {
		textYOffset = newTextYOffset;
		theText.setPosition({x + textXOffset, y + textYOffset});
	}

	void set_size(float newWidth, float newHeight) {
		width = newWidth;
		height = newHeight;
		backgroundRect.setSize(sf::Vector2f(width, height));
	}

	void set_position(float newX, float newY) {
		x = newX;
		y = newY;
		theText.setPosition({x + textXOffset, y + textYOffset});
		backgroundRect.setPosition({x, y});
	}

	void set_on_click(std::function<void()> func) {
		onClickFunction = func;
	}

	// Returns true if the button was triggered
	bool update(std::optional<sf::Event> event) {
		if (const auto mouseMoved = event->getIf<sf::Event::MouseMoved>()) { // MouseMoved
			int mouseX = mouseMoved->position.x;
			int mouseY = mouseMoved->position.y;
			hovered = within((float)mouseX, x, x+width) && within((float)mouseY, y, y+height);
		} else if (const auto mouseButton = event->getIf<sf::Event::MouseButtonPressed>()) { // MouseButtonPressed
			int mouseX = mouseButton->position.x;
			int mouseY = mouseButton->position.y;
			hovered = within((float)mouseX, x, x+width) && within((float)mouseY, y, y+height);
			pressed = hovered;

			if (!clickOnRelease && pressed) {
				pressed = false;
				onClickFunction();
				return true;
			}
		} else if (const auto mouseButton = event->getIf<sf::Event::MouseButtonReleased>()) { // MouseButtonReleased
			int mouseX = mouseButton->position.x;
			int mouseY = mouseButton->position.y;
			hovered = within((float)mouseX, x, x+width) && within((float)mouseY, y, y+height);
			if (!hovered) {
				pressed = false;
				return false;
			}

			if (clickOnRelease && pressed) {
				pressed = false;
				onClickFunction();
				return true;
			}
		} else if (event->is<sf::Event::FocusLost>()) { // FocusLost
			hovered = false;
			pressed = false;
		}

		return false;
	}

	void draw(sf::RenderWindow &window) {
		if (pressed) {
			backgroundRect.setFillColor(sf::Color(200,200,200));
		} else {
			backgroundRect.setFillColor(sf::Color(100,100,100));
		}

		if (hovered) {
			window.draw(backgroundRect);
		}

		window.draw(theText);
	}
};

#endif // BUTTON_HPP

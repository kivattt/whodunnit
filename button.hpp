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

	// TODO: Make this a rounded rectangle, see:
	// https://github.com/SFML/SFML/wiki/Source%3A-Draw-Rounded-Rectangle/_edit
	sf::RectangleShape backgroundRect;
	bool hovered = false;
	bool pressed = false;

	bool clickOnRelease = true;

	std::function<void()> onClickFunction;
	sf::Text theText;

	public:

	Button(sf::Vector2f position, sf::Vector2f size, sf::Font &font, string text) {
		x = position.x;
		y = position.y;
		theText.setPosition(x + 6, y + textYOffset);
		backgroundRect.setPosition(x, y);

		width = size.x;
		height = size.y;
		backgroundRect.setSize(sf::Vector2f(width, height));

		theText.setFont(font);
		theText.setString(text);
		backgroundRect.setFillColor(sf::Color(100,100,100));
	}

	// By default, clicking is done when a mouse button is released.
	// By calling set_click_on_release(false), clicking is done when a mouse button is pressed instead.
	void set_click_on_release(bool shouldClickOnRelease) {
		clickOnRelease = shouldClickOnRelease;
	}

	void set_character_size(int size) {
		theText.setCharacterSize(size);
	}

	void set_text_y_offset(int newTextYOffset) {
		textYOffset = newTextYOffset;
		theText.setPosition(x + 6, y + textYOffset);
	}

	void set_size(float newWidth, float newHeight) {
		width = newWidth;
		height = newHeight;
		backgroundRect.setSize(sf::Vector2f(width, height));
	}

	void set_position(float newX, float newY) {
		x = newX;
		y = newY;
		theText.setPosition(x + 6, y + textYOffset);
		backgroundRect.setPosition(x, y);
	}

	void set_on_click(std::function<void()> func) {
		onClickFunction = func;
	}

	void update(sf::Event event) {
		switch (event.type) {
			case sf::Event::MouseMoved:
				hovered = within((float)event.mouseMove.x, x, x+width) && within((float)event.mouseMove.y, y, y+height);
				break;
			case sf::Event::MouseButtonPressed:
				hovered = within((float)event.mouseButton.x, x, x+width) && within((float)event.mouseButton.y, y, y+height);
				pressed = hovered;

				if (!clickOnRelease && pressed) {
					pressed = false;
					onClickFunction();
				}
				break;
			case sf::Event::MouseButtonReleased:
				hovered = within((float)event.mouseButton.x, x, x+width) && within((float)event.mouseButton.y, y, y+height);
				if (!hovered) {
					pressed = false;
					break;
				}

				if (clickOnRelease && pressed) {
					pressed = false;
					onClickFunction();
				}
				break;
			case sf::Event::LostFocus:
				hovered = false;
				pressed = false;
				break;
		}
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

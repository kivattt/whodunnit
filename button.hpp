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

	// TODO: Make this a rounded rectangle, see:
	// https://github.com/SFML/SFML/wiki/Source%3A-Draw-Rounded-Rectangle/_edit
	sf::RectangleShape backgroundRect;
	bool backgroundRectVisible = false;
	bool pressed = false;

	std::function<void()> onClickFunction;
	sf::Text theText;

	public:

	Button(sf::Vector2f position, sf::Vector2f size, sf::Font &font, string text) {
		x = position.x;
		y = position.y;
		theText.setPosition(x + 6, y - 2);
		backgroundRect.setPosition(x, y);

		width = size.x;
		height = size.y;
		backgroundRect.setSize(sf::Vector2f(width, height));

		theText.setFont(font);
		theText.setString(text);
		backgroundRect.setFillColor(sf::Color(100,100,100));
	}

	void set_size(float newWidth, float newHeight) {
		width = newWidth;
		height = newHeight;
		backgroundRect.setSize(sf::Vector2f(width, height));
	}

	void set_on_click(std::function<void()> func) {
		onClickFunction = func;
	}

	void update(sf::Event event) {
		switch (event.type) {
			case sf::Event::MouseMoved:
				backgroundRectVisible = within((float)event.mouseMove.x, x, x+width) && within((float)event.mouseMove.y, y, y+height);
				break;
			case sf::Event::MouseButtonPressed:
				backgroundRectVisible = within((float)event.mouseButton.x, x, x+width) && within((float)event.mouseButton.y, y, y+height);
				pressed = backgroundRectVisible;
				break;
			case sf::Event::MouseButtonReleased:
				backgroundRectVisible = within((float)event.mouseButton.x, x, x+width) && within((float)event.mouseButton.y, y, y+height);
				if (!backgroundRectVisible) {
					pressed = false;
					break;
				}

				if (pressed) {
					pressed = false;
					onClickFunction();
				}
				break;
		}
	}

	void draw(sf::RenderWindow &window) {
		if (pressed) {
			backgroundRect.setFillColor(sf::Color(200,200,200));
		} else {
			backgroundRect.setFillColor(sf::Color(100,100,100));
		}
		if (backgroundRectVisible) {
			window.draw(backgroundRect);
		}

		window.draw(theText);
	}
};

#endif // BUTTON_HPP

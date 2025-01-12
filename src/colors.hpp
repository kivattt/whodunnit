#ifndef COLORS_HPP
#define COLORS_HPP

#include <SFML/Graphics.hpp>

const sf::Color backgroundColor = sf::Color(10,10,10);

//const sf::Color latestCommitColor = hsv_to_rgb(130, 0.50, 0.4);
const sf::Color latestCommitColor = hsv_to_rgb(130, 0.55, 0.5);

const sf::Color dividerColor = sf::Color(100,100,100);
const sf::Color dividerColorHighlight = sf::Color(220,220,220);

const sf::Color gitLogBackgroundColor = sf::Color(25,25,25);
const sf::Color gitLogBackgroundColorAlternate = sf::Color(35,35,35);
//const sf::Color gitLogBackgroundColorRed = sf::Color(80,40,40);

const sf::Color genericTextColor = sf::Color(200,200,200);
const sf::Color gitLogTextColorDarker = sf::Color(100,100,100);

const sf::Color paneTitleTextColor = sf::Color(230,230,230); // The "Git Log" text

const sf::Color rightClickMenuBackgroundColor = sf::Color(10,10,10);

const sf::Color tabHighlightColor = sf::Color(100,120,255);

const sf::Color selectedBlameColor = hsv_to_rgb(15, 0.50, 0.60);
const sf::Color selectedBlameLineRectColor = sf::Color(200,200,200);

#endif // COLORS_HPP

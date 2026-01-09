#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class Button {
public:
	Button(std::string path, float x, float y, int width = 0, int height = 0);
	bool pressed(const sf::Event::MouseButtonPressed* mouseButtonPressed, sf::Window* window);
	void draw(sf::RenderWindow* window);
	void draw(sf::RenderTexture* rt);
	void setText(std::string text, sf::Font font);
private:
	sf::Sprite* _sprite = NULL;
	sf::Text* _text = NULL;
	float _x = 0;
	float _y = 0;
};

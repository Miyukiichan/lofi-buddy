#include "../include/Button.h"
#include "../include/GraphicsManager.h"

Button::Button(std::string path, float x, float y, int width, int height) {
	_sprite = GraphicsManager::createSprite(path, x, y, width, height);
	_x = x;
	_y = y;
}

bool Button::pressed(const sf::Event::MouseButtonPressed* mouseButtonPressed, sf::Window* window) {
	// Only dealing with left clicks here
	if (mouseButtonPressed->button != sf::Mouse::Button::Left)
		return false;
	auto mousePos = sf::Mouse::getPosition(*window);
	auto mousePosVector = sf::Vector2f{static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)};
	return _sprite->getGlobalBounds().contains(mousePosVector);
}

void Button::draw(sf::RenderWindow* window) {
	window->draw(*_sprite);
	if (_text)
		window->draw(*_text);
}

void Button::draw(sf::RenderTexture* rt) {
	//Specifically do not draw text here - we just need the total area covered
	rt->draw(*_sprite);
}

void Button::setText(std::string text, sf::Font* font) {
	_text = new sf::Text(*font);
	_text->setString(text);
	_text->setCharacterSize(24);
	_text->setFillColor(sf::Color::Black);
	_text->setPosition(sf::Vector2f{ _x + 3, _y });
}

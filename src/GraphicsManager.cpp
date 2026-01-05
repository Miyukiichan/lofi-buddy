#include "../include/GraphicsManager.h"

sf::Texture* GraphicsManager::getTexture(const std::string& path) {
	static std::map<std::string, sf::Texture*> textureCache;
	auto texture = textureCache[path];
	if (!texture)
		texture = new sf::Texture();
	if (!texture->loadFromFile(path))
		return NULL;
	return texture;
}

sf::Sprite* GraphicsManager::createSprite(const std::string& path, float x, float y, int width, int height) {
	sf::Sprite* sprite = new sf::Sprite(*getTexture(path));
	if (sprite == NULL)
		return NULL;
	sprite->setPosition(sf::Vector2f{x, y});
	if (width > 0 && height > 0)
		sprite->setTextureRect(sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{width, height}));
	return sprite;
}

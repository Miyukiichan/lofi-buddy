#include <GraphicsManager.h>
#include <OSInterface.h>
#include <stdio.h>

sf::Texture* GraphicsManager::getTexture(std::string path) {
	static std::map<std::string, sf::Texture*> textureCache;
	path = OSInterface::asset(path);
	auto texture = textureCache[path];
	if (!texture)
		texture = new sf::Texture();
	assert(texture->loadFromFile(path));
	return texture;
}

sf::Sprite* GraphicsManager::createSprite(std::string path, float x, float y, int width, int height) {
	sf::Sprite* sprite = new sf::Sprite(*getTexture(path));
	if (sprite == NULL)
		return NULL;
	sprite->setPosition(sf::Vector2f{x, y});
	if (width > 0 && height > 0)
		sprite->setTextureRect(sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{width, height}));
	return sprite;
}

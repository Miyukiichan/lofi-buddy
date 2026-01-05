#pragma once

#include <SFML/Graphics.hpp>
#include <map>

class GraphicsManager {
public:
    static sf::Texture* getTexture(const std::string& path);
    static sf::Sprite* createSprite(const std::string& path, float x, float y, int width = 0, int height = 0);
};


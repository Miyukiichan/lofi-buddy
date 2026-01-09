#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class OSInterface {
public:
	static std::string asset(std::string fileName);
	static std::string getExecutableDir();
	static void bringWindowToTop(sf::Window* w);
	static bool setTransparency(sf::Window* w, const sf::Image& image);
};

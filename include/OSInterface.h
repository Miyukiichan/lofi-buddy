#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class OSInterface {
public:
	static std::string get_executable_dir();
	static void bringWindowToTop(sf::Window& w);
	static bool setTransparency(sf::Window& w, const sf::Image& image);
};

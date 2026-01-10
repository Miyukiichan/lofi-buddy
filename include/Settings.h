#pragma once
#include "../lib/toml.hpp"

class Settings {
public:
	Settings();
	bool getBool(std::string key);
private:
	toml::value _getValue(std::string key);
	toml::value _toml;
};

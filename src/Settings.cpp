#include <Settings.h>
#include <OSInterface.h>
#include <filesystem>

Settings::Settings() {
	auto configPath = OSInterface::getConfigPath();
	configPath += "/config.toml";
	if (!std::filesystem::exists(configPath)) {
		auto defaultConfig = OSInterface::asset("config.toml");
		std::filesystem::copy(defaultConfig, configPath);
	}
	_toml = toml::parse(configPath);
}

toml::value Settings::_getValue(std::string key) {
	assert(_toml.contains(key));
	return _toml.at(key);
}

bool Settings::getBool(std::string key) {
	auto v = _getValue(key);
	assert(v.is_boolean());
	return v.as_boolean();
}

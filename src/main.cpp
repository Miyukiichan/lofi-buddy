#include <SFML/Graphics.hpp>
#include <SFML/Audio/Music.hpp>
#include <stdio.h>
#include <vector>
#include <future>
#include "../lib/portable-file-dialogs.h"
#include "../include/GraphicsManager.h"
#include "../include/OSInterface.h"

std::string asset(std::string fileName) {
	return OSInterface::get_executable_dir() + "/" + fileName;
}

int main() {
	// Menu buttons
	const unsigned int BTN_PLAYLIST = 0;
	const unsigned int BTN_SETTINGS = 1;
	const unsigned int BTN_QUIT = 2;

	// Dimensions
	const unsigned int deskHeight = 146;
	const unsigned int deskWidth = 323;
	const unsigned int headWidth = 32;
	const unsigned int headHeight = 32;
	const unsigned int headVMargin = 10;
	const unsigned int menuButtonHeight = 32;
	const unsigned int menuButtonWidth = 128;
	const unsigned int menuButtonCount = 3;
	const unsigned int menuButtonVMargin = 10;
	const unsigned int menuPadding = 5;
	const unsigned int winVMargin = 50;
	const unsigned int winHMargin = 50;
	const unsigned int settingsWidth = 400;
	const unsigned int settingsHeight = 400;
	unsigned int menuHeight = ((menuButtonHeight + menuButtonVMargin) * menuButtonCount) + (menuPadding * 2) - menuButtonVMargin;
	unsigned int menuWidth = menuButtonWidth + (menuPadding * 2);
	unsigned int winWidth = deskWidth;
	unsigned int winHeight = deskHeight + headHeight + (headVMargin * 2) + menuHeight;
	sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
	int winX = desktop.size.x - winWidth - winHMargin;
	int winY = desktop.size.y - winHeight - winVMargin;
	int settingsX = desktop.size.x - settingsWidth - winHMargin;
	int settingsY = winY - settingsHeight + menuHeight;

	// Window init
	sf::RenderWindow window(sf::VideoMode({winWidth, winHeight}), "Lofi Buddy", sf::Style::None);
	window.setPosition(sf::Vector2i(winX, winY));
	window.setFramerateLimit(30);

	// Font init
	sf::Font font;
	if (!font.openFromFile(asset("BoldPixels.otf")))
		return -1;

	// Head and desk textures
	float headX = winWidth - headWidth;
	float headY = winHeight - headHeight - deskHeight - headVMargin;
	auto headSprite = GraphicsManager::createSprite(asset("head.png"), headX, headY);
	if (!headSprite)
		return -1;
	float deskX = winWidth - deskWidth;
	float deskY = winHeight - deskHeight;
	auto deskSprite = GraphicsManager::createSprite(asset("test.jpg"), deskX, deskY);
	if (!deskSprite)
		return -1;

	auto menuSprite = GraphicsManager::createSprite(asset("menu.png"), winWidth - menuWidth, 0, menuWidth, menuHeight);
	if (!menuSprite)
		return -1;
	// Menu entry buttons
	std::vector<sf::Sprite*> menuButtonSprites;
	std::vector<sf::Text*> menuButtonLabels;
	for (unsigned int i = 0; i < menuButtonCount; i++) {
		float mbX = winWidth - menuButtonWidth - menuPadding;
		float mbY = (i * (menuButtonHeight + menuButtonVMargin)) + menuPadding;
		auto s = GraphicsManager::createSprite(asset("menu-button.png"), mbX, mbY);
		if (!s)
			return -1;
		menuButtonSprites.push_back(s);

		// Labels
		std::string t = "";
		switch(i) {
			case BTN_PLAYLIST:
				t = "Playlist";
				break;
			case BTN_SETTINGS:
				t = "Settings";
				break;
			case BTN_QUIT:
				t = "Quit";
				break;
		}
		sf::Text* l = new sf::Text(font);
		l->setString(t);
		l->setCharacterSize(24);
		l->setFillColor(sf::Color::Black);
		l->setPosition(sf::Vector2f{ mbX + 3, mbY });
		menuButtonLabels.push_back(l);
	}

	// Settings menu sprites
	auto settingsBackgroundSprite = GraphicsManager::createSprite(asset("menu.png"), 0, 0);
	if (!settingsBackgroundSprite)
		return -1;

	// Collect main sprites that are always visible
	std::vector<sf::Sprite*> sprites;
	sprites.push_back(deskSprite);
	sprites.push_back(headSprite);

	// Initialise default music track
	std::vector<std::string> tracks = { asset("test.mp3") };
	unsigned int trackIndex = 0;
	sf::Music music;
	if (!music.openFromFile(tracks[trackIndex]))
		return -1;
	music.play();
	music.pause();

	// Global UI state
	std::future<std::vector<std::string>> openFileFuture;
	bool openFileOpen = false;
	bool menuOpen = false;
	sf::RenderWindow* settingsWindow = NULL;

	// Main loop
    while (window.isOpen()) {
		if (settingsWindow && settingsWindow->isOpen()) {
			// check all the window's events that were triggered since the last iteration of the loop
			while (const std::optional event = settingsWindow->pollEvent()) {
			}
			OSInterface::bringWindowToTop(*settingsWindow); // TODO: Only do this if I need to to improve performance
			settingsWindow->draw(*settingsBackgroundSprite);
			settingsWindow->display();
		}
        while (const std::optional event = window.pollEvent()) {
			// Emulate a modal dialog where we cannot interact with the main program
			if (openFileOpen || (settingsWindow && settingsWindow->isOpen()))
				continue;
			if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
				auto mousePos = sf::Mouse::getPosition(window);
				auto mousePosVector = sf::Vector2f{static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)};
				if (headSprite->getGlobalBounds().contains(mousePosVector)) {
					if (mousePressed->button == sf::Mouse::Button::Right) {
						menuOpen = !menuOpen;
					}
					else if (mousePressed->button == sf::Mouse::Button::Left) {
						if (music.getStatus() == sf::SoundSource::Status::Paused) 
							music.play();
						else 
							music.pause();
					}
				}
				// Check menu button sprites
				else if (mousePressed->button == sf::Mouse::Button::Left) {
					for (unsigned int i = 0; i < menuButtonSprites.size(); i++) {
						auto mbs = menuButtonSprites[i];
						if (!mbs->getGlobalBounds().contains(mousePosVector))
							continue;
						switch(i) {
							case BTN_PLAYLIST:
								// TODO: File filter for audio files
								if (!openFileOpen) {
									openFileFuture = std::async(std::launch::async, [] () { return pfd::open_file("Select music", ".", { "All Files" , "*" }, pfd::opt::multiselect).result();});
									openFileOpen = true;
									menuOpen = false;
								}
								break;
							case BTN_SETTINGS:
								if (!settingsWindow || !settingsWindow->isOpen()) {
									settingsWindow = new sf::RenderWindow(sf::VideoMode({settingsWidth, settingsHeight}), "Lofi Buddy Settings", sf::Style::None);
									settingsWindow->setPosition(sf::Vector2i{settingsX, settingsY});
									menuOpen = false;
								}
								break;
							case BTN_QUIT:
								window.close();
								break;
						}
						break;
					}
				}
			}
        }
		// Handle async file dialog closing
		if (openFileOpen && openFileFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			auto f = openFileFuture.get();
			if (f.size() > 0) {
				tracks = f;
				trackIndex = 0;
				auto track = tracks[trackIndex];
				if (!music.openFromFile(track))
					auto m = pfd::message("Error", "Error playing track: " + track).result();
				music.play();
			}
			openFileOpen = false;
		}
		
		// Automatically advance to the next track when it gets to the end
		if (music.getStatus() == sf::SoundSource::Status::Stopped) {
			trackIndex++;
			if (trackIndex >= tracks.size()) //Playlist loop by default for now
				trackIndex = 0;
			auto track = tracks[trackIndex];
			if (!music.openFromFile(track))
				auto m = pfd::message("Error", "Error playing track: " + track).result();
			music.play();
		}

		//printf("%s: %f / %f\n", tracks[trackIndex].c_str(), music.getPlayingOffset().asSeconds(), music.getDuration().asSeconds());

		// TODO: Only do this if the UI or animation changes to improve performance
		// Set transparency for anything that is not a sprite
		sf::RenderTexture rt({winWidth, winHeight});
		rt.clear(sf::Color::Transparent);
		for (const auto sprite : sprites)
			rt.draw(*sprite);
		// Just include the outer menu background
		if (menuOpen)
			rt.draw(*menuSprite);
		rt.display();
		sf::Image mask = rt.getTexture().copyToImage();
		OSInterface::setTransparency(window, mask);

		// Drawing all the sprites
		OSInterface::bringWindowToTop(window); // TODO: Only do this if I need to to improve performance
		for (auto s : sprites)
			window.draw(*s);
		if (menuOpen) {
			window.draw(*menuSprite);
			for (auto s : menuButtonSprites)
				window.draw(*s);
			for (auto l : menuButtonLabels)
				window.draw(*l);
		}
		window.display();
    }
}

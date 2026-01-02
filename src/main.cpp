#include <SFML/Graphics.hpp>
#include <SFML/Audio/Music.hpp>
#include <stdio.h>
#include <vector>
#include <future>
#include "../lib/portable-file-dialogs.h"

// Taken from https://github.com/texus/TransparentWindows/blob/master/Transparent.cpp

#ifdef SFML_SYSTEM_WINDOWS
#include <windows.h>

void bringWindowToTop(sf::Window& w) {
	HWND hWnd = w.getNativeHandle();
	BringWindowToTop(hWnd);
}

bool setShape(sf::Window& w, const sf::Image& image) {
	HWND hWnd = w.getNativeHandle();
	const sf::Uint8* pixelData = image.getPixelsPtr();

	// Create a region with the size of the entire window
	HRGN hRegion = CreateRectRgn(0, 0, image.getSize().x, image.getSize().y);

	// Loop over the pixels in the image and for each pixel where the alpha component equals 0,
	// we will remove that pixel from the region.
	// As an optimization, we will combine adjacent transparent pixels on the same row and
	// remove them together, instead of using "CreateRectRgn(x, y, x+1, y+1)" to define
	// a region for each transparent pixel individually.
	bool transparentPixelFound = false;
	unsigned int rectLeft = 0;
	for (unsigned int y = 0; y < image.getSize().y; y++) {
		for (unsigned int x = 0; x < image.getSize().x; x++) {
			const bool isTransparentPixel = (pixelData[y * image.getSize().x * 4 + x * 4 + 3] == 0);
			if (isTransparentPixel && !transparentPixelFound) {
				transparentPixelFound = true;
				rectLeft = x;
			} 
			else if (!isTransparentPixel && transparentPixelFound) {
				HRGN hRegionPixel = CreateRectRgn(rectLeft, y, x, y+1);
				CombineRgn(hRegion, hRegion, hRegionPixel, RGN_XOR);
				DeleteObject(hRegionPixel);
				transparentPixelFound = false;
			}
		}

		if (transparentPixelFound) {
			HRGN hRegionPixel = CreateRectRgn(rectLeft, y, image.getSize().x, y+1);
			CombineRgn(hRegion, hRegion, hRegionPixel, RGN_XOR);
			DeleteObject(hRegionPixel);
			transparentPixelFound = false;
		}
	}

	SetWindowRgn(hWnd, hRegion, true);
	DeleteObject(hRegion);
	return true;
}
#endif

#ifdef SFML_SYSTEM_LINUX
#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <X11/Xatom.h>

void bringWindowToTop(sf::Window& w) {
	Window wnd = w.getNativeHandle();
	Display* display = XOpenDisplay(NULL);
	
	// Multiple atoms for persistent window behavior
    Atom stateAtom = XInternAtom(display, "_NET_WM_STATE", 1);
    Atom stickyAtom = XInternAtom(display, "_NET_WM_STATE_STICKY", 1);
    Atom alwaysOnTopAtom = XInternAtom(display, "_NET_WM_STATE_ABOVE", 1);
    Atom keepAboveAtom = XInternAtom(display, "_NET_WM_STATE_KEEP_ABOVE", 1);

    // Prepare client message
    XClientMessageEvent event;
    event.type = ClientMessage;
    event.window = wnd;
    event.message_type = stateAtom;
    event.format = 32;
    event.data.l[0] = 1;
    event.data.l[1] = alwaysOnTopAtom;
    event.data.l[2] = 0;
    event.data.l[3] = 0;
    event.data.l[4] = 0;

    XSendEvent(display, DefaultRootWindow(display), False, 
               SubstructureRedirectMask | SubstructureNotifyMask, 
               (XEvent*)&event);

    // Additional window attribute modifications
    XSetWindowAttributes attributes;
    attributes.override_redirect = True;
    XChangeWindowAttributes(display, wnd, CWOverrideRedirect, &attributes);

    XFlush(display);
    XCloseDisplay(display);
}

#undef None
#undef Status

bool setShape(sf::Window& w, const sf::Image& image) {
	Window wnd = w.getNativeHandle();
	Display* display = XOpenDisplay(NULL);

	// Setting the window shape requires the XShape extension
	int event_base;
	int error_base;
	if (!XShapeQueryExtension(display, &event_base, &error_base)) {
		XCloseDisplay(display);
		return false;
	}

	const auto pixelData = image.getPixelsPtr();

	// Create a black and white pixmap that has the size of the window
	Pixmap pixmap = XCreatePixmap(display, wnd, image.getSize().x, image.getSize().y, 1);
	GC gc = XCreateGC(display, pixmap, 0, NULL);

	// Make the entire pixmap white
	XSetForeground(display, gc, 1);
	XFillRectangle(display, pixmap, gc, 0, 0, image.getSize().x, image.getSize().y);

	// Loop over the pixels in the image and change the color of the pixmap to black
	// for each pixel where the alpha component equals 0.
	// As an optimization, we will combine adjacent transparent pixels on the same row and
	// draw them together, instead of calling "XFillRectangle(display, pixmap, gc, x, y, 1, 1)"
	// for each transparent pixel individually.
	XSetForeground(display, gc, 0);
	bool transparentPixelFound = false;
	unsigned int rectLeft = 0;
	for (unsigned int y = 0; y < image.getSize().y; y++) {
		for (unsigned int x = 0; x < image.getSize().x; x++) {
			const bool isTransparentPixel = (pixelData[y * image.getSize().x * 4 + x * 4 + 3] == 0);
			if (isTransparentPixel && !transparentPixelFound) {
				transparentPixelFound = true;
				rectLeft = x;
			}
			else if (!isTransparentPixel && transparentPixelFound) {
				XFillRectangle(display, pixmap, gc, rectLeft, y, x - rectLeft, 1);
				transparentPixelFound = false;
			}
		}

		if (transparentPixelFound) {
			XFillRectangle(display, pixmap, gc, rectLeft, y, image.getSize().x - rectLeft, 1);
			transparentPixelFound = false;
		}
	}

	// Use the black and white pixmap to define the shape of the window. All pixels that are
	// white will be kept, while all black pixels will be clipped from the window.
	XShapeCombineMask(display, wnd, ShapeBounding, 0, 0, pixmap, ShapeSet);

	// Free resources
	XFreeGC(display, gc);
	XFreePixmap(display, pixmap);
	XFlush(display);
	XCloseDisplay(display);
	return true;
}
#endif


class GraphicsManager {
public:
    static sf::Texture* getTexture(const std::string& path) {
        static std::map<std::string, sf::Texture*> textureCache;
        auto texture = textureCache[path];
		if (texture == NULL)
			texture = new sf::Texture();
        if (!texture->loadFromFile(path))
			return NULL;
        return texture;
    }

    static sf::Sprite* createSprite(const std::string& path, float x, float y, int width = 0, int height = 0) {
        sf::Sprite* sprite = new sf::Sprite(*getTexture(path));
		if (sprite == NULL)
			return NULL;
        sprite->setPosition(sf::Vector2f{x, y});
		if (width > 0 && height > 0)
			sprite->setTextureRect(sf::IntRect(sf::Vector2i{0, 0}, sf::Vector2i{width, height}));
        return sprite;
    }
};

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
	const unsigned int winVMargin = 100;
	const unsigned int winHMargin = 50;
	unsigned int menuHeight = ((menuButtonHeight + menuButtonVMargin) * menuButtonCount) + (menuPadding * 2) - menuButtonVMargin;
	unsigned int menuWidth = menuButtonWidth + (menuPadding * 2);
	unsigned int winWidth = deskWidth;
	unsigned int winHeight = deskHeight + headHeight + (headVMargin * 2) + menuHeight;

	// Window init
	sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
	sf::RenderWindow window(sf::VideoMode({winWidth, winHeight}), "Lofi Buddy", sf::Style::None);
	window.setPosition(sf::Vector2i(desktop.size.x - winWidth - winVMargin, desktop.size.y - winHeight - winHMargin));
	window.setFramerateLimit(30);

	// Font init
	sf::Font font;
	if (!font.openFromFile("BoldPixels.otf"))
		return -1;

	// Head and desk textures
	float headX = winWidth - headWidth;
	float headY = winHeight - headHeight - deskHeight - headVMargin;
	auto headSprite = GraphicsManager::createSprite("head.png", headX, headY);
	if (headSprite == NULL)
		return -1;
	float deskX = winWidth - deskWidth;
	float deskY = winHeight - deskHeight;
	auto deskSprite = GraphicsManager::createSprite("test.jpg", deskX, deskY);
	if (deskSprite == NULL)
		return -1;

	auto menuSprite = GraphicsManager::createSprite("menu.png", winWidth - menuWidth, 0, menuWidth, menuHeight);
	if (menuSprite == NULL)
		return -1;
	// Menu entry buttons
	std::vector<sf::Sprite*> menuButtonSprites;
	std::vector<sf::Text*> menuButtonLabels;
	for (int i = 0; i < menuButtonCount; i++) {
		float mbX = winWidth - menuButtonWidth - menuPadding;
		float mbY = (i * (menuButtonHeight + menuButtonVMargin)) + menuPadding;
		auto s = GraphicsManager::createSprite("menu-button.png", mbX, mbY);
		if (s == NULL)
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

	// Collect main sprites that are always visible
	std::vector<sf::Sprite*> sprites;
	sprites.push_back(deskSprite);
	sprites.push_back(headSprite);

	// Initialise default music track
	std::vector<std::string> tracks = { "test.mp3" };
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

	// Main loop
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
			// Emulate a modal dialog where we cannot interact with the main program
			if (openFileOpen)
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
					for (int i = 0; i < menuButtonSprites.size(); i++) {
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
		setShape(window, mask);

		// Drawing all the sprites
		bringWindowToTop(window); // TODO: Only do this if I need to to improve performance
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

#include <SFML/Graphics.hpp>
#include <SFML/Audio/Music.hpp>
#include <stdio.h>
#include <vector>
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

int main() {
	const unsigned int deskHeight = 146;
	const unsigned int deskWidth = 323;
	const unsigned int headWidth = 32;
	const unsigned int headHeight = 32;
	const unsigned int headVMargin = 10;
	const unsigned int menuButtonHeight = 32;
	const unsigned int menuButtonWidth = 128;
	const unsigned int menuButtonCount = 3; //quit, addFiles, settings
	const unsigned int menuButtonVMargin = 10;
	const unsigned int winVMargin = 100;
	const unsigned int winHMargin = 50;
	unsigned int menuHeight = (menuButtonHeight + (menuButtonVMargin * 2)) * menuButtonCount;
	unsigned int winWidth = deskWidth;
	unsigned int winHeight = deskHeight + headHeight + (headVMargin * 2) + menuHeight;
	sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
	sf::RenderWindow window(sf::VideoMode({winWidth, winHeight}), "Lofi Buddy", sf::Style::None);
	window.setPosition(sf::Vector2i(desktop.size.x - winWidth - winVMargin, desktop.size.y - winHeight - winHMargin));
	window.setFramerateLimit(30);

	sf::Image headImage;
	if (!headImage.loadFromFile("head.png")) 
		return -1;

	sf::Texture headTexture;
	if (!headTexture.loadFromImage(headImage))
		return -1;
	sf::Sprite headSprite(headTexture);
	float headX = winWidth - headWidth;
	float headY = winHeight - headHeight - deskHeight - headVMargin;
	headSprite.setPosition(sf::Vector2f{headX, headY});

	sf::Image deskImage;
	if (!deskImage.loadFromFile("test.jpg")) 
		return -1;

	sf::Texture deskTexture;
	if (!deskTexture.loadFromImage(deskImage))
		return -1;
	sf::Sprite deskSprite(deskTexture);
	float deskX = winWidth - deskWidth;
	float deskY = winHeight - deskHeight;
	deskSprite.setPosition(sf::Vector2f{deskX, deskY});

	std::vector<sf::Sprite> sprites;
	sprites.push_back(deskSprite);
	sprites.push_back(headSprite);

	sf::Music music;
	if (!music.openFromFile("test.mp3"))
		return -1;
	music.play();

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
			else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
				auto mousePos = sf::Mouse::getPosition(window);
				if (headSprite.getGlobalBounds().contains(sf::Vector2f{static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)})) {
					if (mousePressed->button == sf::Mouse::Button::Right) {
						//window.close();
						//break;
						auto m = pfd::message("Hello", "This is a test").result();
						printf("%d\n", m);
					}
					else if (mousePressed->button == sf::Mouse::Button::Left) {
						if (music.getStatus() == sf::SoundSource::Status::Paused) 
							music.play();
						else 
							music.pause();
					}
				}
			}
        }
		// TODO: Only do this if the UI or animation changes to improve performance
		// Set transparency for anything that is not a sprite
		sf::RenderTexture rt({winWidth, winHeight});
		rt.clear(sf::Color::Transparent);
		for (const auto& sprite : sprites)
			rt.draw(sprite);
		rt.display();
		sf::Image mask = rt.getTexture().copyToImage();
		setShape(window, mask);

		bringWindowToTop(window); // TODO: Only do this if I need to to improve performance
		window.draw(deskSprite);
		window.draw(headSprite);
		window.display();
    }
}

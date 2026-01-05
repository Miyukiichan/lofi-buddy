#include "../include/OSInterface.h"

#ifdef SFML_SYSTEM_WINDOWS
#include <windows.h>
#include <shlwapi.h>  
#include <filesystem>
 
// Link against Shlwapi.lib (Windows only)  
#pragma comment(lib, "shlwapi.lib")  
 
std::string OSInterface::get_executable_dir() {  
    char buffer[MAX_PATH]; // MAX_PATH is 260 (Windows path limit)  
    DWORD length = GetModuleFileNameA(NULL, buffer, MAX_PATH);  
 
    if (length == 0 || length == MAX_PATH) {  
        throw std::runtime_error("GetModuleFileNameA failed or path truncated");  
    }  
 
    // Strip the filename to get the directory  
    PathRemoveFileSpecA(buffer);  
 
    return std::string(buffer);  
}

void OSInterface::bringWindowToTop(sf::Window& w) {
	HWND hWnd = w.getNativeHandle();
	BringWindowToTop(hWnd);
}

bool OSInterface::setTransparency(sf::Window& w, const sf::Image& image) {
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
#include <unistd.h>  
#include <limits.h>  
 
std::string OSInterface::get_executable_dir() {  
    char buffer[PATH_MAX];  
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);  
 
    if (length == -1) {  
        throw std::runtime_error("readlink failed");  
    }  
 
    buffer[length] = '\0';  
    return std::filesystem::path(buffer).parent_path().string();  
}

void OSInterface::bringWindowToTop(sf::Window& w) {
	Window wnd = w.getNativeHandle();
	Display* display = XOpenDisplay(NULL);
	
	// Multiple atoms for persistent window behavior
    Atom stateAtom = XInternAtom(display, "_NET_WM_STATE", 1);
    //Atom stickyAtom = XInternAtom(display, "_NET_WM_STATE_STICKY", 1);
    Atom alwaysOnTopAtom = XInternAtom(display, "_NET_WM_STATE_ABOVE", 1);
    //Atom keepAboveAtom = XInternAtom(display, "_NET_WM_STATE_KEEP_ABOVE", 1);

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

bool OSInterface::setTransparency(sf::Window& w, const sf::Image& image) {
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

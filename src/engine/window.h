#pragma once

#include <utils/types.h>

class Window {
public:
	void init();
	void showCursor(bool show);
	// Returns the new aspect ratio of the window
	float handleResize(); 
	void cleanup();

	bool isInitialised{ false };
	bool active{ true };

	VkExtent2D extent{ 1920, 1080 };
	struct SDL_Window* SDL_window{ nullptr };
	bool relativeMouseMode{ false };
};
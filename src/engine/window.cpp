#include "window.h"

#include <SDL2/SDL.h>

void Window::init() {
	SDL_Init(SDL_INIT_VIDEO);
	SDL_window = SDL_CreateWindow(
		"VNG Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		extent.width,
		extent.height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
	);
}

float Window::handleResize() {
	int32_t width = 0;
	int32_t height = 0;

	SDL_GetWindowSize(SDL_window, &width, &height);
	extent.width = width;
	extent.height = height;

	return (float)width / (float)height;
}

void Window::showCursor(bool show) {
	SDL_SetRelativeMouseMode(show ? SDL_FALSE : SDL_TRUE);
	relativeMouseMode = !show;
}

void Window::cleanup() {
	SDL_DestroyWindow(SDL_window);
}

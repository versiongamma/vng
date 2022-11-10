#pragma once

class Window;

#include <utils/types.h>
#include <glm/vec2.hpp>
#include <SDL2/SDL.h>

enum ButtonState
{
	NEUTRAL,
	HELD,
	PRESSED,
	RELEASED
};

class InputHandler {
public:
	void init(Window& window);
	void update();
	ButtonState getKeyState(SDL_Scancode key);
	glm::vec2 mousePosition{ 0.f };
	glm::vec2 mouseVelocity{ 0.f };

protected:
	Window* window;
	const uint8_t* keyboardState;
	uint8_t keyboardStatePrev[SDL_NUM_SCANCODES];
	glm::vec2 mousePositionPrev{ 0.f };
	bool lastFrameOnWindowMode{ false };
};
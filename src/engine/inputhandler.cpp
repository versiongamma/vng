#include "inputhandler.h"

#include <algorithm>
#include <iostream>

#include "window.h"

void InputHandler::init(Window& window) {
	this->window = &window;
	std::fill_n(keyboardStatePrev, SDL_NUM_SCANCODES, 0);
	keyboardState = SDL_GetKeyboardState(0);
}

void InputHandler::update() {
	std::copy(&keyboardState[0], &keyboardState[SDL_NUM_SCANCODES], keyboardStatePrev);
	mousePositionPrev = mousePosition;

	int mouseX = 0, mouseY = 0;
	if (window->relativeMouseMode)
	{
		SDL_GetRelativeMouseState(&mouseX, &mouseY);
		if (lastFrameOnWindowMode) {
			mouseVelocity = { 0.f, 0.f };
			lastFrameOnWindowMode = false;
		}
		else {
			mouseVelocity = { mouseX, -mouseY };
			
		}
	}
	else
	{
		lastFrameOnWindowMode = true;
		mouseVelocity = { 0.f, 0.f };
		lastFrameOnWindowMode = true;
	}

	mousePosition = { mouseX, mouseY };
	
}

ButtonState InputHandler::getKeyState(SDL_Scancode key) {
	if (keyboardStatePrev[key] == 0)
	{
		if (keyboardState[key] != 0)
		{
			return ButtonState::PRESSED;
		}
		else
		{
			return ButtonState::NEUTRAL;
		}
	}
	else
	{
		if (keyboardState[key] != 0)
		{
			return ButtonState::HELD;
		}
		else
		{
			return ButtonState::RELEASED;
		}
	}
}
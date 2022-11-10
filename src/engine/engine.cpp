#include "engine.h"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <imgui_impl_sdl.h>
#include <iostream>

#include "camera.h"
#include "scene.h"

void Engine::init(std::vector<std::shared_ptr<Scene>>& scenes) {
	console.init();
	window.init();
	renderer.init(window, &frameNumber, console);
	camera = &renderer.camera;
	inputHandler.init(window);

	window.showCursor(!relativeMode);

	this->scenes = &scenes;

	for (auto scene : scenes) {
		scene->init(renderer);
	}

	isInitialised = true;
}

void Engine::run() {
	SDL_Event event;
	bool quit = false;

	while (!quit) {
		while (SDL_PollEvent(&event) != 0)
		{
			if (event.type == SDL_QUIT) {
				quit = true;
			}

			if (event.type == SDL_WINDOWEVENT) {
				if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					camera->aspect = window.handleResize();
					renderer.framebufferResized = true;
				}
			}

			ImGuiIO& io = ImGui::GetIO();
			ImGui_ImplSDL2_ProcessEvent(&event);
		}

		uint64_t currentFrameTime = SDL_GetPerformanceCounter();
		float deltaTime = (currentFrameTime - lastFrameTime) / static_cast<float>(SDL_GetPerformanceFrequency());
		lastFrameTime = currentFrameTime;

		frameTime = deltaTime;
		fps = (uint16_t)(1.f / deltaTime);

		update(deltaTime);
		if (!(SDL_GetWindowFlags(window.SDL_window) & SDL_WINDOW_MINIMIZED)) {
			draw();
		}

		inputHandler.update();
		++frameNumber;
	}
}

void Engine::update(float deltaTime) {
	if (
		inputHandler.getKeyState(SDL_SCANCODE_GRAVE) == ButtonState::PRESSED ||
		inputHandler.getKeyState(SDL_SCANCODE_ESCAPE) == ButtonState::PRESSED
	) {
		relativeMode = !relativeMode;
		window.showCursor(!relativeMode);
		inputHandler.mouseVelocity = { 0.f, 0.f };

#ifdef NDEBUG
		showDebugWindows = !relativeMode;
#endif // NDEBUG
		return;
	}

	(*scenes)[activeSceneIndex]->update(deltaTime, inputHandler);
}

void Engine::draw() {
	(*scenes)[activeSceneIndex]->draw(renderer);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame(window.SDL_window);
	ImGui::NewFrame();
	if (showDebugWindows) {
		(*scenes)[activeSceneIndex]->drawDebug();
		renderer.drawDebug();
		drawDebug();
	}
	ImGui::Render();
	

	renderer.draw();
}

void Engine::drawDebug() {
	ImGui::Begin("Statistics");
	ImGui::Text("FPS: %d", fps);
	ImGui::Text("Frame Time: %.4fms", frameTime);
	ImGui::End();

	console.draw();
}

void Engine::cleanup() {
	renderer.cleanup();
	console.cleanup();
}
#pragma once

class Camera;
class Scene;

#include <utils/types.h>

#include <memory>
#include "window.h"
#include "renderer.h"
#include "console.h"
#include "inputhandler.h"
#include "model.h"

class Engine {
public:
	void init(std::vector<std::shared_ptr<Scene>>& scenes);
	void update(float deltaTime);
	void draw();
	void drawDebug();
	void cleanup();
	void run();

protected:
	Window window;
	Renderer renderer;
	Camera* camera;
	InputHandler inputHandler;
	Console console;
	std::vector<std::shared_ptr<Scene>>* scenes;

	bool isInitialised{ false };
	uint32_t frameNumber{ 0 };
	uint64_t lastFrameTime{ 0 };
	uint32_t activeSceneIndex{ 0 };

	float frameTime;
	uint16_t fps;

	bool relativeMode{ true };

	bool showDebugWindows{
#ifdef _DEBUG
		true
#endif // _DEBUG
#ifndef _DEBUG
		false
#endif // !_DEBUG
	};
};

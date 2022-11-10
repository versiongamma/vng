#pragma once

class Renderer;
class InputHandler;

class Scene {
public:
	virtual void init(Renderer& renderer) = 0;
	virtual void update(float deltaTime, InputHandler& inputHandler) = 0;
	virtual void draw(Renderer& renderer) = 0;
	virtual void drawDebug() = 0;
	virtual void cleanup() = 0;

protected:
};
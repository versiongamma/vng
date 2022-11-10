#pragma once

class Renderer;
class InputHandler;

#include <glm/glm.hpp>

class Entity {
public:
	virtual void update(float deltaTime, InputHandler& input) = 0;
	virtual void draw(Renderer& renderer) = 0;
	virtual void cleanup() = 0;
	void moveInDirection(glm::vec3 vector, glm::vec3 rotation);

	glm::vec3 position{ 0.f };
	glm::vec3 rotation{ 0.f };

protected:
};
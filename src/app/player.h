#pragma once

class Camera;

#include <engine/entity.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

class Player : public Entity {
public:
	void init(Camera& camera);
	virtual void update(float deltaTime, InputHandler& input) override;
	virtual void draw(Renderer& renderer) override;
	virtual void cleanup() override;

protected:
	Camera* camera;
	float speed{ 2.f };
};
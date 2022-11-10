#pragma once

#include <engine/entity.h>
#include <engine/model.h>
#include <engine/renderer.h>

#include <glm/vec3.hpp>

class Object : public Entity {
public: 
	void init(Renderer& renderer, LoadModelInfo info);
	virtual void update(float deltaTime, InputHandler& input) override;
	virtual void draw(Renderer& renderer) override;
	virtual void cleanup() override;

	Model* model;
	float scale{1.f};

protected:
	void updateTransformationMatrix();
};
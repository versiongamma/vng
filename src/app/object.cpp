#include "object.h"

#include <engine/renderer.h>
#include <engine/inputhandler.h>

#include <glm/gtx/transform.hpp>
#include <iostream>

void Object::init(Renderer& renderer, LoadModelInfo modelInfo) {
	model = new Model();
	renderer.loadModel(*model, modelInfo);
	model->transformMatrix = glm::mat4{ 1.f };
}

void Object::update(float deltaTime, InputHandler& inputHandler) {
	updateTransformationMatrix();
}

void Object::updateTransformationMatrix() {
	model->transformMatrix = glm::translate(glm::mat4{ 1.f }, position);
	model->transformMatrix = glm::rotate(model->transformMatrix, glm::radians(rotation.x), glm::vec3{ 0, 1, 0 });
	model->transformMatrix = glm::rotate(model->transformMatrix, glm::radians(rotation.y), glm::vec3{ 1, 0, 0 });
	model->transformMatrix = glm::rotate(model->transformMatrix, glm::radians(rotation.z), glm::vec3{ 0, 0, 1 });
	model->transformMatrix = glm::scale(model->transformMatrix, glm::vec3(scale, scale, scale));
}

void Object::draw(Renderer& renderer) {
	model->addToRenderQueue(renderer);
}

void Object::cleanup() {
	delete model;
}
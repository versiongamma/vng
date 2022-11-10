#include "player.h"

#include <engine/renderer.h>
#include <engine/camera.h>
#include <engine/inputhandler.h>

#include <algorithm>

void Player::init(Camera& camera) {
	this->camera = &camera;
}

void Player::update(float deltaTime, InputHandler& inputHandler) {
	if (inputHandler.getKeyState(SDL_SCANCODE_W) == ButtonState::HELD) {
		moveInDirection(glm::vec3{ 0.f, 0.f, speed * deltaTime }, { 0, rotation.y, 0 });
	}

	if (inputHandler.getKeyState(SDL_SCANCODE_S) == ButtonState::HELD) {
		moveInDirection(glm::vec3{ 0.f, 0.f, -speed * deltaTime }, { 0, rotation.y, 0 });
	}

	if (inputHandler.getKeyState(SDL_SCANCODE_A) == ButtonState::HELD) {
		moveInDirection(glm::vec3{ speed * deltaTime, 0.f, 0.f }, { 0, rotation.y, 0 });
	}

	if (inputHandler.getKeyState(SDL_SCANCODE_D) == ButtonState::HELD) {
		moveInDirection(glm::vec3{ -speed * deltaTime, 0.f, 0.f }, { 0, rotation.y, 0 });
	}

	if (inputHandler.getKeyState(SDL_SCANCODE_SPACE) == ButtonState::HELD) {
		position += glm::vec3{ 0.f, -speed * deltaTime, 0.f };
	}

	if (inputHandler.getKeyState(SDL_SCANCODE_LSHIFT) == ButtonState::HELD) {
		position += glm::vec3{ 0.f, speed * deltaTime, 0.f };
	}

	rotation.x = std::clamp(rotation.x - inputHandler.mouseVelocity.y * 0.05f, -90.f, 90.f);
	rotation.y = utils::clamp_loop(rotation.y + inputHandler.mouseVelocity.x * 0.05f, -180.f, 180.f);

	camera->position = position;
	camera->rotation = rotation;
}

void Player::draw(Renderer& renderer) {

}

void Player::cleanup() {

}
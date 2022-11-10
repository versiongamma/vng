#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

void Camera::init() {

}

void Camera::translate(glm::vec3 amount) {
	position += amount;
}

void Camera::pitch(float angle) {
	if (rotation.x > MAX_PITCH) {
		rotation.x = MAX_PITCH;
		return;
	}

	if (rotation.x < MIN_PITCH) {
		rotation.x = MIN_PITCH;
		return;
	}

	rotation.x -= angle;
}

void Camera::yaw(float angle) {
	rotation.y += angle;
}

glm::mat4 Camera::projection() const {
	glm::mat4 projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 200.f);
	projection[1][1] *= -1;

	return projection;
}

glm::mat4 Camera::view() const {
	glm::mat4 translationMatrix = glm::translate(glm::mat4{ 1.f }, position);
	glm::mat4 rotationMatrix = glm::mat4{ 1.f };

	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.x), glm::vec3{ 1, 0, 0 });
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3{ 0, 1, 0 });
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.z), glm::vec3{ 0, 0, 1 });

	return rotationMatrix * translationMatrix;
}

glm::mat4 Camera::matrix() const {
	return projection() * view();
}
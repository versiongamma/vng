#include "entity.h"

#include <glm/gtx/rotate_vector.hpp>

void Entity::moveInDirection(glm::vec3 vector, glm::vec3 rotation) {
	glm::vec3 rotatedVector = glm::rotateX(vector, glm::radians(-rotation.x));
	rotatedVector = glm::rotateY(rotatedVector, glm::radians(-rotation.y));
	rotatedVector = glm::rotateZ(rotatedVector, glm::radians(-rotation.z));

	position += rotatedVector;
}
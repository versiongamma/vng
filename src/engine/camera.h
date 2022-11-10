#pragma once

#include <glm/glm.hpp> 
#include <glm/gtx/quaternion.hpp>

constexpr float MIN_PITCH = -90.f;
constexpr float MAX_PITCH = 90.f;

class Camera {
public:
	void init();
	void translate(glm::vec3 amount);
	void pitch(float angle);
	void yaw(float angle);

	glm::mat4 projection() const;
	glm::mat4 view() const;
	glm::mat4 matrix() const;

	glm::vec3 position{ 0.f };
	glm::vec3 rotation{ 0.f };
	float fov{ 70.f };
	float aspect{ 1920.f / 1080.f };

};
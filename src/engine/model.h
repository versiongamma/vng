#pragma once

class Renderer;
struct Mesh;

#include <utils/types.h>
#include <glm/glm.hpp>

struct Model {
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;

	void addToRenderQueue(Renderer& renderer);
};
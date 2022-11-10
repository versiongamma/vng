#pragma once

class Console;

#include <utils/types.h>
#include <unordered_map>

#include "mesh.h"
#include "model.h"

struct CreateMaterialInfo {
	std::string name;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDescriptorSet textureSet;
};

class MeshManager {
public:
	void init(Console& console);
	Mesh* loadMesh(const std::string& name);
	Material* loadMaterial(CreateMaterialInfo info);

	std::unordered_map<std::string, Material> materials;
	std::unordered_map<std::string, Mesh> meshes;

protected:
	Material* createMaterial(CreateMaterialInfo info);

	Console* console;
};
#include "meshmanager.h"

#include <iostream>

#include "console.h"
#include "texturemanager.h"
#include "mesh.h"

void MeshManager::init(Console& console) {
	this->console = &console;
}

Material* MeshManager::createMaterial(CreateMaterialInfo info) {
	Material newMaterial;
	newMaterial.pipeline = info.pipeline;
	newMaterial.pipelineLayout = info.layout;
	newMaterial.textureSet = info.textureSet;
	materials[info.name] = newMaterial;
	return &materials[info.name];
}

Material* MeshManager::loadMaterial(CreateMaterialInfo info) {
	auto pair = materials.find(info.name);
	if (pair == materials.end()) {
		return createMaterial(info);
	} else {
		return &(*pair).second;
	}
}

Mesh* MeshManager::loadMesh(const std::string& name) {
	auto pair = meshes.find(name);
	if (pair == meshes.end()) {
		Mesh newMesh;
		std::string warn, err;
		const bool result = newMesh.loadFromOBJ(name.c_str(), &warn, &err);

		if (!warn.empty()) {
			console->log("[WARN]: " + warn);
		}

		if (!result) {
			console->log("[ERROR]: Failed to load mesh " + name + '\n' + err);
			return nullptr;
		}

		console->log("Loaded mesh " + name + " successfully");
		meshes[name] = newMesh;
		return &meshes[name];
	}

	return &(*pair).second;
}
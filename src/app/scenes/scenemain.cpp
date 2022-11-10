#include "scenemain.h"

#include <engine/renderer.h>
#include <engine/vulkankinitialisers.h>
#include <engine/inputhandler.h>

#include "imgui.h"

void SceneMain::init(Renderer& renderer) {
	player.init(renderer.camera);

	Object object;
	LoadModelInfo model1info = {
		.filePath = "assets/AW101.obj",
		.textured = true,
		.texturePath = "assets/AW101.png"
	};
	object.position = glm::vec3{ 4, 0, -6 };
	object.scale = 0.03f;
	object.init(renderer, model1info);

	Object object2;
	LoadModelInfo model2info = {
		.filePath = "assets/Trident-A10.obj",
		.textured = true,
		.texturePath = "assets/Trident_UV_No_Dekol_Color.png"
	};
	object2.position = glm::vec3{ -1, 0, -6 };
	object2.rotation = glm::vec3{ -90.f, -90.f, 0.f };
	object2.scale = 0.002f;
	object2.init(renderer, model2info);

	objects.push_back(object);
	objects.push_back(object2);
}

void SceneMain::update(float deltaTime, InputHandler& inputHandler) {
	player.update(deltaTime, inputHandler);
	objects[0].rotation.x = utils::clamp_loop(objects[0].rotation.x + deltaTime * 10.f, -180.f, 180.f);

	for (Object& object : objects) {
		object.update(deltaTime, inputHandler);
	}
}

void SceneMain::draw(Renderer& renderer) {
	for (Object& object : objects) {
		object.draw(renderer);
	}
}

void SceneMain::drawDebug() {
	ImGui::Begin("Scene");
	ImGui::SliderFloat3("Position", &objects[1].position.x, -10.f, 10.f, "% .5f");
	ImGui::SliderFloat("Scale", &objects[1].scale, 0.002f, 0.01f, "%.7f");
	ImGui::End();
}

void SceneMain::cleanup() {

}
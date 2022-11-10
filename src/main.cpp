#include <engine/engine.h>
#include <engine/scene.h>
#include <app/scenes/scenemain.h>
#include <SDL.h>
#include <vector>
#include <memory>

int main(int argc, char* argv[]) {
	Engine engine;

	std::shared_ptr<SceneMain> scene(new SceneMain());
	std::vector<std::shared_ptr<Scene>> scenes;
	scenes.push_back((std::shared_ptr<Scene>)scene);

	engine.init(scenes);
	engine.run();
	engine.cleanup();

	return 0;
}
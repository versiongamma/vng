#pragma once

#include <engine/scene.h>
#include <app/player.h>
#include <app/object.h>



class SceneMain : public Scene {
public:
	virtual void init(Renderer& renderer) override;
	virtual void update(float deltaTime, InputHandler& inputHandler) override;
	virtual void draw(Renderer& renderer) override;
	virtual void drawDebug() override;
	virtual void cleanup() override;

protected:
	std::vector<Object> objects;
	Player player;
};
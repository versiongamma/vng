#include "model.h"

#include "renderer.h"

void Model::addToRenderQueue(Renderer& renderer) {
	renderer.addToModelQueue(*this);
}

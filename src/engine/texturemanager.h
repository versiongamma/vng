#pragma once

class Renderer;
class Console;

#include <utils/types.h>
#include <unordered_map>

struct Texture {
	AllocatedImage image;
	VkImageView imageView;
};

class TextureManager {
public:
	void init(Console& console);
	Texture* loadTexture(Renderer& renderer, const char* file);
	std::unordered_map<std::string, Texture> loadedTextures;

protected:
	Console* console;
};
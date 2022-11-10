#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdlib>
#include <vector>
#include <string>
#include <sstream>

struct Material {
    VkDescriptorSet textureSet{ VK_NULL_HANDLE };
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct AllocatedImage {
    VkImage image;
    VmaAllocation allocation;
};

namespace utils {
    inline float clamp_loop(float value, float min, float max) {
        if (value > max) return min;
        if (value < min) return max;
        return value;
    }

    // https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
    inline std::vector<std::string> split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    inline std::string join(const std::vector<std::string> arr, char delimiter, uint32_t clamp) {
        std::string out = "";
        uint32_t index = 0;
        for (std::string token : arr) {
            out += token;
            out += delimiter;
            ++index;
            if (index >= arr.size() - clamp) break;       
        }
        
        return out;
    }
}

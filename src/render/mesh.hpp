#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include "spock/core.hpp"
namespace vkengine {
    struct Vertex {
        glm::vec3 position;
        float     uv_x;
        glm::vec3 normal;
        float     uv_y;
        glm::vec4 color;
    };

    struct GeoSurface {
        uint32_t startIndex;
        uint32_t count;
    };

    struct GPUMeshBuffers {
        spock::Buffer          indexBuffer;
        spock::Buffer          vertexBuffer;
        VkDeviceAddress vertexBufferAddress;

        uint32_t        indexCount;
        uint32_t        startIndex;
    };

    struct Mesh {
        GPUMeshBuffers data;
        spock::Image          diffuse;
        spock::Image          normal;
        spock::Image          specular;
    };

    struct Model {
        std::vector<Mesh> meshes;
    };
    Model load_gltf_model(const char* filePath);
}

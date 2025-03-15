#pragma once
#include "objects.hpp"
#include <unordered_map>
#include <filesystem>

struct GeoSurface {
    uint32_t startIndex;
    uint32_t count;
};

struct MeshAsset {
    std::string name;

    std::vector<GeoSurface> surfaces;
    GPUMeshBuffers meshBuffers;
};

//forward declaration
class VulkanContext;
std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanContext* engine, const char* filePath);

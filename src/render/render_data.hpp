#pragma once
//Contains the internal "user" data for render.cpp.
//user is in quotes cuz i am the user and the actual user is just using the executable.
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
#include "vk/types.hpp"
#include <chrono>
namespace spock {

inline VkDescriptorSetLayout computeImageDescLayout;
inline VkDescriptorSetLayout uniformDescLayout;
inline VkDescriptorSet       computeImageDesc;
inline VkPipeline            computePipeline;
inline VkPipeline            vertexPipeline;
inline VkPipelineLayout      computePipelineLayout;
inline VkPipelineLayout      vertexPipelineLayout;

constexpr int      STORAGE_COUNT = 65536;
constexpr int      SAMPLER_COUNT = 65536;
constexpr int      IMAGE_COUNT   = 65536;

constexpr uint32_t STORAGE_BINDING = 0;
constexpr uint32_t SAMPLER_BINDING = 1;
constexpr uint32_t VERTEX_BINDING  = 2;
constexpr uint32_t INDEX_BINDING   = 3;

inline VkDescriptorSetLayout samplerDescriptorSetLayout;
inline VkDescriptorSet       samplerDescriptorSet;
inline VkSampler             linearSampler;
inline VkSampler             nearestSampler;

struct ComputePushConstants {
    glm::vec4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
};

struct VertexPushConstants {
    int             diffuse;
    int             normal;
    int             specular;
    glm::mat4       worldMatrix;
    VkDeviceAddress vertexBuffer;
};

struct GPUSceneData {
    glm::mat4 view;
    glm::mat4 proj;
};

inline GPUSceneData sceneData = {};

inline Model guitar;

//some settings
inline bool FPS_UNLIMITED = false;
inline int FPS_LIMIT = 240;
inline int TICK_LIMIT = 240;
inline std::chrono::nanoseconds delta(0);

inline std::chrono::nanoseconds tick(0);

}

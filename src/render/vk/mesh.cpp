
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <span>
#include <unordered_map>
#include "core.hpp"
#include "internal.hpp"

using namespace spock;

std::unordered_map<std::string, Image> loadedTextures;

int                                    vertexID  = 0;
int                                    indicesID = 0;


GPUMeshBuffers                         upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices)
{
    const size_t   vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t   indexBufferSize  = indices.size() * sizeof(uint32_t);

    GPUMeshBuffers newSurface;

    //create vertex buffer
    newSurface.vertexBuffer = create_buffer(vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                                                    VMA_MEMORY_USAGE_GPU_ONLY);

    //find the adress of the vertex buffer
    VkBufferDeviceAddressInfo deviceAddressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.vertexBuffer.buffer};
    newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(ctx.device, &deviceAddressInfo);

    //create index buffer
    newSurface.indexBuffer = create_buffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    Buffer staging         = create_buffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void*  data = staging.info.pMappedData;

    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // copy index buffer
    memcpy((char*)data + vertexBufferSize, indices.data(), indexBufferSize);

    begin_immediate_command();
    VkBufferCopy vertexCopy{0};
    vertexCopy.dstOffset = 0;
    vertexCopy.srcOffset = 0;
    vertexCopy.size      = vertexBufferSize;

    vkCmdCopyBuffer(ctx.immCommandBuffer, staging.buffer, newSurface.vertexBuffer.buffer, 1, &vertexCopy);

    VkBufferCopy indexCopy{0};
    indexCopy.dstOffset = 0;
    indexCopy.srcOffset = vertexBufferSize;
    indexCopy.size      = indexBufferSize;

    vkCmdCopyBuffer(ctx.immCommandBuffer, staging.buffer, newSurface.indexBuffer.buffer, 1, &indexCopy);
    end_immediate_command();

    destroy_buffer(staging);
    //we doont want to print here
    destroyQueue.push(newSurface.indexBuffer);
    destroyQueue.push(newSurface.vertexBuffer);

    newSurface.indexCount = indices.size();
    newSurface.startIndex = indices[0];
    return newSurface;
}

Image load_material_texture(aiMaterial* mat, aiTextureType type, const std::string& directory) {
    if (mat->GetTextureCount(type) == 0)
        return {};
    assert(mat->GetTextureCount(type) == 1); //only 1 texture per mesh PLZ
    aiString str;
    mat->GetTexture(type, 0, &str);
    std::string stdstr(directory + str.C_Str());

    if (loadedTextures.contains(stdstr))
        return loadedTextures[stdstr];
    Image texture          = create_image(stdstr.c_str(), VK_IMAGE_USAGE_SAMPLED_BIT);
    loadedTextures[stdstr] = texture;
    printf("Loaded mesh texture %s\n", stdstr.c_str());
    return texture;
}

Mesh processMesh(aiMesh* mesh, const aiScene* scene, const std::string& directory) {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        vertex.position.x               = mesh->mVertices[i].x;
        vertex.position.y               = mesh->mVertices[i].y;
        vertex.position.z               = mesh->mVertices[i].z;
        vertex.normal.x                 = mesh->mNormals[i].x;
        vertex.normal.y                 = mesh->mNormals[i].y;
        vertex.normal.z                 = mesh->mNormals[i].z;
        constexpr bool useNormalAsColor = true;
        if (useNormalAsColor)
            vertex.color = glm::vec4(vertex.normal.x, vertex.normal.y, vertex.normal.z, 1.f);
        else
            vertex.color = *(glm::vec4*)&mesh->mColors[i];

        if (mesh->mTextureCoords[0]) {
            vertex.uv_x = mesh->mTextureCoords[0][i].x;
            vertex.uv_y = mesh->mTextureCoords[0][i].y;
        } else {
            vertex.uv_x = 0.0;
            vertex.uv_y = 0.0;
        }
        vertices.push_back(vertex);
    }

    // process indices
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

        for (uint32_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh newMesh{};
    newMesh.data = upload_mesh(indices, vertices);
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        newMesh.diffuse      = load_material_texture(material, aiTextureType_DIFFUSE, directory);
        newMesh.specular     = load_material_texture(material, aiTextureType_SPECULAR, directory);
        newMesh.normal       = load_material_texture(material, aiTextureType_NORMALS, directory);
    }

    return newMesh;
}

void processNode(aiNode* node, const aiScene* scene, Model& model, const std::string& directory) {
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(processMesh(mesh, scene, directory));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, model, directory);
    }
}

Model spock::load_gltf_model(const char* filePath) {
    Assimp::Importer import;
    const aiScene*   scene = import.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenNormals);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        printf("ERROR::ASSIMP:: %s\n", import.GetErrorString());
        abort();
    }

    std::string directory(filePath);
    directory.erase(directory.begin() + directory.find_last_of('/') + 1, directory.end());
    Model model;
    processNode(scene->mRootNode, scene, model, directory);
    printf("Loaded model %s\n", filePath);
    return model;
}

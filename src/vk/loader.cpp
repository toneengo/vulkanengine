#include "stb_image.h"
#include <iostream>
#include "loader.hpp"
#include <filesystem>

#include "context.hpp"
#include "create_info.hpp"
#include "objects.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

MeshAsset processMesh(aiMesh *mesh, const aiScene *scene, VulkanContext* engine)
{
    MeshAsset newmesh;
    newmesh.name = std::string(mesh->mName.C_Str());
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        vertex.normal.x = mesh->mNormals[i].x;
        vertex.normal.y = mesh->mNormals[i].y;
        vertex.normal.z = mesh->mNormals[i].z;
        constexpr bool useNormalAsColor = true;
        if (useNormalAsColor)
            vertex.color = glm::vec4(vertex.normal.x, vertex.normal.y, vertex.normal.z, 1.f);
        else
            vertex.color = *(glm::vec4*)&mesh->mColors[i];
        
        if (mesh->mTextureCoords[0])
        {
            vertex.uv_x =  mesh->mTextureCoords[0][i].x;
            vertex.uv_y =  mesh->mTextureCoords[0][i].y;
        }
        else
        {
            vertex.uv_x = 0.0;
            vertex.uv_y = 0.0;
        }
        vertices.push_back(vertex);
    }
    // process indices
    for (uint32_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        GeoSurface newSurface;
        newSurface.startIndex = indices.size(); 
        newSurface.count = mesh->mNumVertices; 
        newmesh.surfaces.push_back(newSurface);

        for (uint32_t j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    newmesh.meshBuffers = engine->upload_mesh(indices, vertices);
    /*
    // process material
    if(mesh->mMaterialIndex >= 0)
    {
        [...]
    }
    */

    return newmesh;
}  
void processNode(aiNode *node, const aiScene *scene, std::vector<std::shared_ptr<MeshAsset>>& meshes, VulkanContext* engine)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(std::make_shared<MeshAsset>(std::move(processMesh(mesh, scene, engine))));	
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, meshes, engine);
    }
}

std::optional<std::vector<std::shared_ptr<MeshAsset>>> loadGltfMeshes(VulkanContext* engine, const char* filePath)
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        printf("ERROR::ASSIMP:: %s\n", import.GetErrorString());
        return std::nullopt;
    }

    std::vector<std::shared_ptr<MeshAsset>> meshes;

    processNode(scene->mRootNode, scene, meshes, engine);

    return meshes;
}

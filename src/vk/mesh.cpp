#include "core.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
using namespace engine;

GPUMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
MeshAsset processMesh(aiMesh *mesh, const aiScene *scene)
{
    MeshAsset newmesh;
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
    newmesh.meshBuffers = upload_mesh(indices, vertices);
    /*
    // process material
    if(mesh->mMaterialIndex >= 0)
    {
        [...]
    }
    */

    return newmesh;
}  

void processNode(aiNode *node, const aiScene *scene, std::unordered_map<std::string, MeshAsset>& meshes)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes[mesh->mName.C_Str()] = processMesh(mesh, scene);
        printf("Loaded mesh '%s'\n", mesh->mName.C_Str());
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, meshes);
    }
}

void engine::load_gltf_meshes(const char* filePath, std::unordered_map<std::string, MeshAsset>& meshes)
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        printf("ERROR::ASSIMP:: %s\n", import.GetErrorString());
        return;
    }

    processNode(scene->mRootNode, scene, meshes);

    return;
}

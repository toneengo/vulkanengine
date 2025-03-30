#include "core.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <span>
using namespace engine;

std::unordered_map<std::string, Image> loadedTextures;
GPUMeshBuffers upload_mesh(std::span<uint32_t> indices, std::span<Vertex> vertices);
Image load_material_texture(aiMaterial *mat, aiTextureType type, const std::string& directory)
{
    if (mat->GetTextureCount(type) == 0) return {};
    assert(mat->GetTextureCount(type) == 1); //only 1 texture per mesh PLZ
    aiString str;
    mat->GetTexture(type, 0, &str);
    std::string stdstr(directory + str.C_Str());

    if (loadedTextures.contains(stdstr))
        return loadedTextures[stdstr];
    Image texture = create_texture(stdstr.c_str(), VK_IMAGE_USAGE_SAMPLED_BIT);
    loadedTextures[stdstr] = texture;
    printf("Loaded mesh texture %s\n", stdstr.c_str());
    return texture;
}

Mesh processMesh(aiMesh *mesh, const aiScene *scene, const std::string& directory)
{
    Mesh newmesh;
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
    if(mesh->mMaterialIndex >= 0)
    {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        newmesh.diffuse = load_material_texture(material, aiTextureType_DIFFUSE, directory);
        newmesh.specular = load_material_texture(material, aiTextureType_SPECULAR, directory);
        newmesh.normal = load_material_texture(material, aiTextureType_NORMALS, directory);
    }
    newmesh.meshBuffers = upload_mesh(indices, vertices);

    return newmesh;
}  

void processNode(aiNode *node, const aiScene *scene, Model &model, const std::string& directory)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        model.meshes.push_back(processMesh(mesh, scene, directory));
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene, model, directory);
    }
}

Model engine::load_gltf_model(const char* filePath)
{
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(filePath, aiProcess_Triangulate | aiProcess_GenNormals);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
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

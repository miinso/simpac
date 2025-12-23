#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>

int main() {
    // Initialize Assimp Importer
    Assimp::Importer importer;

    // Path to the OBJ file
    const std::string filePath = "resources/models/crate_model.obj";

    // Read the OBJ file
    const aiScene* scene =
        importer.ReadFile(filePath.c_str(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return -1;
    }

    // Print the number of meshes
    std::cout << "Number of meshes: " << scene->mNumMeshes << std::endl;

    // Iterate through each mesh
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        std::cout << "Mesh " << i + 1 << ":" << std::endl;
        std::cout << "  Number of vertices: " << mesh->mNumVertices << std::endl;
        std::cout << "  Number of faces: " << mesh->mNumFaces << std::endl;
    }

    return 0;
}
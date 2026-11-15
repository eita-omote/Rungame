// ============================================================
// ファイルの役割: FBXモデル・アニメーションの読み込み
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Component.h"
#include "../Engine/GameObject.h"
#include "FBXModel.h"
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <assimp/material.h>
#include <utility>

// クラスの責務: FBXモデル・アニメーションの読み込み
class FBXLoaderComponent : public Component {
public:
    // FBXモデル（メッシュ＆スケルトン）の読み込み
    bool LoadModel(const std::string& filePath) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            filePath,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_LimitBoneWeights
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            OutputDebugStringA("[FBXLoader] Failed to load FBX model.\n");
            return false;
        }

        m_rootNode = NodeData{};
        m_boneCounter = 0;
        m_boneInfoMap.clear();
        m_vertices.clear();
        m_indices.clear();
        m_submeshes.clear();
        m_textureBytes.clear();
        m_textureFile.clear();
        m_materials.clear();
        for (unsigned i = 0; i < scene->mNumMaterials; ++i) {
            MaterialData mat;
            aiColor4D diffuse(1, 1, 1, 1);
            if (AI_SUCCESS == aiGetMaterialColor(scene->mMaterials[i], AI_MATKEY_COLOR_DIFFUSE, &diffuse))
                mat.color = { diffuse.r, diffuse.g, diffuse.b, diffuse.a };
            FindMaterialTexture(scene, scene->mMaterials[i], filePath, mat);
            m_materials.push_back(std::move(mat));
        }

        // 1. ノード階層構造の読み込み
        ReadNodeHierarchy(scene->mRootNode, m_rootNode);

        // 2. メッシュとボーン情報の解析
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
            ProcessMesh(scene->mMeshes[i], scene);
        }

        //頂点ごとに最大4つの重み(weight)を正規化する。重みを設定されてない頂点はバインドポーズのままにする
        for (auto& vertex : m_vertices) {
            float sum = 0.0f;
            for (float weight : vertex.weights) sum += weight;
            if (sum > 0.00001f) for (float& weight : vertex.weights) weight /= sum;
        }
        return true;
    }

    struct MaterialData {
        DirectX::XMFLOAT4 color{ 1,1,1,1 };
        std::vector<unsigned char> embedded;
        std::string file;
    };
    struct SubmeshData { uint32_t startIndex = 0, indexCount = 0, materialIndex = 0; };
    const std::vector<MaterialData>& GetMaterials() const { return m_materials; }
    const std::vector<SubmeshData>& GetSubmeshes() const { return m_submeshes; }
    const std::vector<unsigned char>& GetEmbeddedTextureBytes() const { return m_textureBytes; }
    const std::string& GetTextureFile() const { return m_textureFile; }

    const std::vector<SkinnedVertex>& GetVertices() const { return m_vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    const std::unordered_map<std::string, BoneInfo>& GetBoneInfoMap() const { return m_boneInfoMap; }
    const NodeData& GetRootNode() const { return m_rootNode; }
    int GetBoneCount() const { return m_boneCounter; }

private:
    std::vector<MaterialData> m_materials;
    std::vector<SubmeshData> m_submeshes;
    std::vector<SkinnedVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::unordered_map<std::string, BoneInfo> m_boneInfoMap;
    NodeData m_rootNode;
    int m_boneCounter = 0;
    std::vector<unsigned char> m_textureBytes;
    std::string m_textureFile;

    void FindMaterialTexture(const aiScene* scene, const aiMaterial* material,
        const std::string& modelPath, MaterialData& out) {
        namespace fs = std::filesystem;
        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) != AI_SUCCESS) return;
        const std::string name = path.C_Str();
        const aiTexture* embedded = scene->GetEmbeddedTexture(name.c_str());
        if (embedded && embedded->pcData && embedded->mWidth) {
            if (embedded->mHeight == 0) {
                const auto* bytes = reinterpret_cast<const unsigned char*>(embedded->pcData);
                out.embedded.assign(bytes, bytes + embedded->mWidth);
            }
            else {
                // 生のaiTexelデータはエンコードされたWICデータではないため、マテリアルカラーに切り替える
                OutputDebugStringA("[FBXLoader] Uncompressed embedded texture unsupported.\n");
            }
            return;
        }
        const fs::path dir = fs::path(modelPath).parent_path();
        const fs::path raw(name);
        const fs::path candidates[] = { raw, dir / raw, dir / raw.filename(), dir / "Textures" / raw.filename() };
        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (fs::is_regular_file(candidate, ec)) { out.file = candidate.string(); return; }
        }
    }

    void ReadNodeHierarchy(const aiNode* src, NodeData& dest) {
        dest.name = src->mName.data;
        dest.transformation = AiToXMMATRIX(src->mTransformation);

        for (unsigned int i = 0; i < src->mNumChildren; ++i) {
            NodeData child;
            ReadNodeHierarchy(src->mChildren[i], child);
            dest.children.push_back(child);
        }
    }

    void ProcessMesh(const aiMesh* mesh, const aiScene* scene) {
        uint32_t vertexBase = static_cast<uint32_t>(m_vertices.size());
        SubmeshData sub;
        sub.startIndex = static_cast<uint32_t>(m_indices.size());
        sub.materialIndex = mesh->mMaterialIndex;

        // 頂点属性取得
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            SkinnedVertex vertex;
            vertex.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

            if (mesh->HasNormals()) {
                vertex.normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
            }
            if (mesh->mTextureCoords[0]) {
                vertex.uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
            }
            else {
                vertex.uv = { 0.0f, 0.0f };
            }
            m_vertices.push_back(vertex);
        }

        // インデックス取得
        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; ++j) {
                m_indices.push_back(vertexBase + face.mIndices[j]);
            }
        }

        sub.indexCount = static_cast<uint32_t>(m_indices.size()) - sub.startIndex;
        m_submeshes.push_back(sub);

        // ボーンとウェイト情報の解析
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

            if (m_boneInfoMap.find(boneName) == m_boneInfoMap.end()) {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_boneCounter;
                newBoneInfo.offsetMatrix = AiToXMMATRIX(mesh->mBones[boneIndex]->mOffsetMatrix);
                m_boneInfoMap[boneName] = newBoneInfo;
                boneID = m_boneCounter++;
            }
            else {
                boneID = m_boneInfoMap[boneName].id;
            }

            auto weights = mesh->mBones[boneIndex]->mWeights;
            unsigned int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (unsigned int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
                uint32_t vertexId = vertexBase + weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                m_vertices[vertexId].AddBoneData(boneID, weight);
            }
        }
    }
};
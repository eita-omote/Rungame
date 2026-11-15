// ============================================================
// ファイルの役割: FBXモデルと骨格のデータ構造
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <DirectXMath.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// 1頂点あたりの最大ボーン影響数
constexpr int MAX_BONE_INFLUENCE = 4;

// スキニング用頂点構造体
struct SkinnedVertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
    int boneIDs[MAX_BONE_INFLUENCE] = { -1, -1, -1, -1 };
    float weights[MAX_BONE_INFLUENCE] = { 0.0f, 0.0f, 0.0f, 0.0f };

    void AddBoneData(int boneID, float weight) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (boneIDs[i] < 0) {
                boneIDs[i] = boneID;
                weights[i] = weight;
                return;
            }
        }
    }
};

// ボーンオフセット情報
struct BoneInfo {
    int id;
    DirectX::XMMATRIX offsetMatrix;
};

// ノード階層構造
struct NodeData {
    std::string name;
    DirectX::XMMATRIX transformation;
    std::vector<NodeData> children;
};

// Assimpの行列(aiMatrix4x4)をDirectX::XMMATRIXへ変換する関数
inline DirectX::XMMATRIX AiToXMMATRIX(const aiMatrix4x4& from) {
    return DirectX::XMMATRIX(
        from.a1, from.b1, from.c1, from.d1,
        from.a2, from.b2, from.c2, from.d2,
        from.a3, from.b3, from.c3, from.d3,
        from.a4, from.b4, from.c4, from.d4
    );
}
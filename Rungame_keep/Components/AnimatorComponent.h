// ============================================================
// ファイルの役割: アニメーションの再生・ブレンド・時間管理
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Component.h"
#include "../Engine/GameObject.h"
#include "FBXLoaderComponent.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// クラスの責務: アニメーションの再生・ブレンド・時間管理
class AnimatorComponent : public Component {
public:
    // Each clip owns its Assimp importer, so aiAnimation pointers remain valid.
    bool LoadClip(const std::string& name, const std::string& path) {
        if (name.empty()) return false;
        auto importer = std::make_unique<Assimp::Importer>();
        const aiScene* scene = importer->ReadFile(path, aiProcess_Triangulate);
        if (!scene || !scene->HasAnimations()) {
            OutputDebugStringA("[Animator] Clip load failed.\n");
            return false;
        }
        auto loader = m_owner ? m_owner->GetComponent<FBXLoaderComponent>() : nullptr;
        if (!loader || loader->GetBoneCount() > 256) return false;
        Clip clip;
        clip.animation = scene->mAnimations[0];
        for (unsigned int i = 0; i < clip.animation->mNumChannels; ++i) {
            const aiNodeAnim* ch = clip.animation->mChannels[i];
            clip.channels[ch->mNodeName.C_Str()] = ch;
        }
        clip.importer = std::move(importer);
        m_clips[name] = std::move(clip);
        if (m_final.size() != static_cast<size_t>(loader->GetBoneCount()))
            m_final.assign(loader->GetBoneCount(), DirectX::XMMatrixIdentity());
        if (m_current.empty()) return Play(name);
        return true;
    }

    // Backward-compatible entry point for existing code.
    bool LoadAnimation(const std::string& path) { return LoadClip("Run", path); }

    bool Play(const std::string& name, bool restart = false) {
        auto it = m_clips.find(name);
        if (it == m_clips.end()) return false;
        if (m_current == name && !restart) return true;
        // Snapshot the currently displayed local pose before changing clips.
        // Blend from this snapshot to the new clip (no abrupt T-pose jump).
        auto loader = m_owner ? m_owner->GetComponent<FBXLoaderComponent>() : nullptr;
        if (!m_current.empty() && !restart && loader && m_blendSeconds > 0.0f) {
            m_fromPose.clear();
            CapturePose(loader->GetRootNode(), m_fromPose);
            m_blendElapsed = 0.0f;
            m_blending = true;
        }
        else {
            m_blending = false;
            m_fromPose.clear();
        }
        m_current = name;
        m_animation = it->second.animation;
        m_channels = it->second.channels;
        m_time = 0.0;
        if (loader) Evaluate(*loader);
        return true;
    }

    const std::string& CurrentClip() const { return m_current; }
    void SetBlendDuration(float seconds) { m_blendSeconds = (std::max)(0.0f, seconds); }
    bool IsBlending() const { return m_blending; }

    // 現在のアニメーションの再生速度。1.0が元の速度。
    void SetPlaybackSpeed(float speed) { m_playbackSpeed = (std::max)(0.01f, speed); }
    float GetPlaybackSpeed() const { return m_playbackSpeed; }

    // falseにすると最終フレームで停止する（ジャンプ用）。
    void SetLooping(bool looping) { m_looping = looping; }

    // FBXに記録されたクリップの長さを秒単位で返す。
    float GetClipDurationSeconds(const std::string& name) const {
        auto it = m_clips.find(name);
        if (it == m_clips.end() || !it->second.animation) return 0.0f;
        const aiAnimation* a = it->second.animation;
        const double tps = a->mTicksPerSecond > 0.0 ? a->mTicksPerSecond : 25.0;
        return static_cast<float>(a->mDuration / tps);
    }

    void Update(float dt) override {
        if (!m_animation || !m_owner) return;
        auto loader = m_owner->GetComponent<FBXLoaderComponent>();
        if (!loader) return;
        const double tps = m_animation->mTicksPerSecond > 0.0 ? m_animation->mTicksPerSecond : 25.0;
        const double duration = m_animation->mDuration;
        if (duration <= 0.0) return;
        const float safeDt = (std::max)(0.0f, dt);
        // 速度補正後の再生時刻を更新する。単発再生は最終姿勢を保持する。
        const double nextTime = m_time + safeDt * m_playbackSpeed * tps;
        m_time = m_looping ? std::fmod(nextTime, duration) : (std::min)(nextTime, duration);
        if (m_blending) {
            m_blendElapsed += safeDt;
            if (m_blendElapsed >= m_blendSeconds) {
                m_blending = false;
                m_fromPose.clear();
            }
        }
        Evaluate(*loader);
    }

    const std::vector<DirectX::XMMATRIX>& GetFinalBoneTransforms() const { return m_final; }

private:
    struct Clip {
        std::unique_ptr<Assimp::Importer> importer;
        const aiAnimation* animation = nullptr;
        std::unordered_map<std::string, const aiNodeAnim*> channels;
    };
    std::unordered_map<std::string, Clip> m_clips;
    std::string m_current;
    const aiAnimation* m_animation = nullptr;
    std::unordered_map<std::string, const aiNodeAnim*> m_channels;
    std::vector<DirectX::XMMATRIX> m_final;
    double m_time = 0.0;
    struct Pose {
        DirectX::XMFLOAT3 translation{ 0,0,0 };
        DirectX::XMFLOAT3 scale{ 1,1,1 };
        DirectX::XMFLOAT4 rotation{ 0,0,0,1 };
    };
    std::unordered_map<std::string, Pose> m_fromPose;
    float m_blendSeconds = 0.15f;
    float m_blendElapsed = 0.0f;
    bool m_blending = false;
    float m_playbackSpeed = 1.0f;
    bool m_looping = true;

    static Pose Decompose(DirectX::FXMMATRIX matrix) {
        Pose p;
        DirectX::XMVECTOR s, r, t;
        if (DirectX::XMMatrixDecompose(&s, &r, &t, matrix)) {
            DirectX::XMStoreFloat3(&p.scale, s);
            DirectX::XMStoreFloat4(&p.rotation, DirectX::XMQuaternionNormalize(r));
            DirectX::XMStoreFloat3(&p.translation, t);
        }
        return p;
    }
    static DirectX::XMMATRIX Matrix(const Pose& p) {
        using namespace DirectX;
        return XMMatrixScalingFromVector(XMLoadFloat3(&p.scale)) *
            XMMatrixRotationQuaternion(XMLoadFloat4(&p.rotation)) *
            XMMatrixTranslationFromVector(XMLoadFloat3(&p.translation));
    }
    Pose Sample(const NodeData& node) const {
        Pose p = Decompose(node.transformation);
        auto ch = m_channels.find(node.name);
        if (ch != m_channels.end()) {
            const aiNodeAnim& c = *ch->second;
            if (c.mNumPositionKeys) DirectX::XMStoreFloat3(&p.translation, Position(c, m_time));
            if (c.mNumRotationKeys) DirectX::XMStoreFloat4(&p.rotation, Rotation(c, m_time));
            if (c.mNumScalingKeys) DirectX::XMStoreFloat3(&p.scale, Scale(c, m_time));
        }
        return p;
    }
    Pose DisplayPose(const NodeData& node) const {
        Pose target = Sample(node);
        if (!m_blending || m_blendSeconds <= 0.0f) return target;
        auto it = m_fromPose.find(node.name);
        if (it == m_fromPose.end()) return target;
        const float a = (std::clamp)(m_blendElapsed / m_blendSeconds, 0.0f, 1.0f);
        Pose result;
        using namespace DirectX;
        XMStoreFloat3(&result.translation, XMVectorLerp(XMLoadFloat3(&it->second.translation), XMLoadFloat3(&target.translation), a));
        XMStoreFloat3(&result.scale, XMVectorLerp(XMLoadFloat3(&it->second.scale), XMLoadFloat3(&target.scale), a));
        XMStoreFloat4(&result.rotation, XMQuaternionNormalize(XMQuaternionSlerp(XMLoadFloat4(&it->second.rotation), XMLoadFloat4(&target.rotation), a)));
        return result;
    }
    void CapturePose(const NodeData& node, std::unordered_map<std::string, Pose>& poses) const {
        poses[node.name] = DisplayPose(node);
        for (const auto& child : node.children) CapturePose(child, poses);
    }

    template<typename Key>
    static unsigned int Segment(const Key* keys, unsigned int count, double time) {
        for (unsigned int i = 0; i + 1 < count; ++i)
            if (time < keys[i + 1].mTime) return i;
        return count - 2;
    }
    static float Alpha(double time, double a, double b) {
        if (b <= a) return 0.0f;
        return (std::clamp)(static_cast<float>((time - a) / (b - a)), 0.0f, 1.0f);
    }
    static DirectX::XMVECTOR Position(const aiNodeAnim& c, double t) {
        if (!c.mNumPositionKeys) return DirectX::XMVectorZero();
        if (c.mNumPositionKeys == 1) {
            auto v = c.mPositionKeys[0].mValue;
            return DirectX::XMVectorSet(v.x, v.y, v.z, 0);
        }
        auto i = Segment(c.mPositionKeys, c.mNumPositionKeys, t);
        auto a = c.mPositionKeys[i], b = c.mPositionKeys[i + 1];
        auto v = a.mValue + Alpha(t, a.mTime, b.mTime) * (b.mValue - a.mValue);
        return DirectX::XMVectorSet(v.x, v.y, v.z, 0);
    }
    static DirectX::XMVECTOR Rotation(const aiNodeAnim& c, double t) {
        if (!c.mNumRotationKeys) return DirectX::XMQuaternionIdentity();
        if (c.mNumRotationKeys == 1) {
            auto v = c.mRotationKeys[0].mValue;
            return DirectX::XMVectorSet(v.x, v.y, v.z, v.w);
        }
        auto i = Segment(c.mRotationKeys, c.mNumRotationKeys, t);
        auto a = c.mRotationKeys[i], b = c.mRotationKeys[i + 1];
        aiQuaternion q;
        aiQuaternion::Interpolate(q, a.mValue, b.mValue, Alpha(t, a.mTime, b.mTime));
        q.Normalize();
        return DirectX::XMVectorSet(q.x, q.y, q.z, q.w);
    }
    static DirectX::XMVECTOR Scale(const aiNodeAnim& c, double t) {
        if (!c.mNumScalingKeys) return DirectX::XMVectorSet(1, 1, 1, 0);
        if (c.mNumScalingKeys == 1) {
            auto v = c.mScalingKeys[0].mValue;
            return DirectX::XMVectorSet(v.x, v.y, v.z, 0);
        }
        auto i = Segment(c.mScalingKeys, c.mNumScalingKeys, t);
        auto a = c.mScalingKeys[i], b = c.mScalingKeys[i + 1];
        auto v = a.mValue + Alpha(t, a.mTime, b.mTime) * (b.mValue - a.mValue);
        return DirectX::XMVectorSet(v.x, v.y, v.z, 0);
    }
    void Evaluate(const FBXLoaderComponent& loader) {
        const auto& root = loader.GetRootNode();
        // For row-vector matrices, global inverse comes on the right.
        const auto inverseRoot = DirectX::XMMatrixInverse(nullptr, root.transformation);
        Visit(root, DirectX::XMMatrixIdentity(), inverseRoot, loader.GetBoneInfoMap());
    }
    void Visit(const NodeData& node, DirectX::FXMMATRIX parent,
        DirectX::FXMMATRIX inverseRoot,
        const std::unordered_map<std::string, BoneInfo>& bones) {
        auto local = Matrix(DisplayPose(node));
        auto global = local * parent;
        auto bone = bones.find(node.name);
        if (bone != bones.end()) {
            const int id = bone->second.id;
            if (id >= 0 && static_cast<size_t>(id) < m_final.size())
                m_final[id] = bone->second.offsetMatrix * global * inverseRoot;
        }
        for (const auto& child : node.children) Visit(child, global, inverseRoot, bones);
    }
};

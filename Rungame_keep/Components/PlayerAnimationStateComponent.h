// ============================================================
// ファイルの役割: 移動・ジャンプに応じたアニメーションの切り替え
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include "Component.h"
#include "PlayerControllerComponent.h"
#include "AnimatorComponent.h"
#include "../Engine/GameObject.h"
#include <algorithm>

// Animation states are separate from physics. Add new clips later without
// changing PlayerControllerComponent.
// クラスの責務: 移動・ジャンプに応じたアニメーションの切り替え
class PlayerAnimationStateComponent : public Component {
public:
    enum class State { Run, Jump, Fall, Land };

    void Reset() {
        m_state = State::Run;
        m_landingTime = 0.0f;
        if (m_owner) {
            if (auto* animator = m_owner->GetComponent<AnimatorComponent>()) {
                animator->SetPlaybackSpeed(1.0f);
                animator->SetLooping(true);
                animator->Play("Run", true);
            }
        }
    }

    State GetState() const { return m_state; }
    void SetLandingDuration(float seconds) { m_landingDuration = (std::max)(0.0f, seconds); }

    // Called explicitly AFTER PlayerControllerComponent updates. Do not use
    // Update() here: GameObject stores components in an unordered_map.
    void Tick(float dt) {
        if (!m_owner) return;
        auto* controller = m_owner->GetComponent<PlayerControllerComponent>();
        auto* animator = m_owner->GetComponent<AnimatorComponent>();
        if (!controller || !animator) return;
        dt = (std::max)(0.0f, dt);
        const bool grounded = controller->IsGrounded();
        const float vy = controller->GetVerticalVelocity();

        State next = m_state;
        if (!grounded) {
            m_landingTime = 0.0f;
            next = vy > 0.0f ? State::Jump : State::Fall;
        }
        else if (m_state == State::Jump || m_state == State::Fall) {
            next = State::Land;
            m_landingTime = 0.0f;
        }
        else if (m_state == State::Land) {
            m_landingTime += dt;
            if (m_landingTime >= m_landingDuration) next = State::Run;
        }
        else {
            next = State::Run;
        }

        if (next != m_state) {
            m_state = next;

            // ジャンプは空中にいる時間に合わせて一度だけ再生する。
            // Fallに入っても同じJumpクリップを継続し、途中で再スタートしない。
            if (next == State::Jump) {
                const float airTime = controller->GetExpectedAirTime();
                const float clipTime = animator->GetClipDurationSeconds("Jump");
                animator->SetLooping(false);

                // ジャンプアニメーションの再生速度を設定
                constexpr float jumpPlaybackSpeed = 0.7f;

                animator->SetLooping(false);
                animator->SetPlaybackSpeed(jumpPlaybackSpeed);
                animator->Play("Jump");
                animator->Play("Jump");
            }
            else if (next == State::Land || next == State::Run) {
                animator->SetPlaybackSpeed(1.0f);
                animator->SetLooping(true);
                animator->Play("Run");
            }
        }
    }

private:
    State m_state = State::Run;
    float m_landingTime = 0.0f;
    float m_landingDuration = 0.15f;
};

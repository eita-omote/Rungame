// ============================================================
// ファイルの役割: カメラのビュー・投影行列の保持
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <DirectXMath.h>
// クラスの責務: カメラのビュー・投影行列の保持
class Camera {
public:
    void Initialize(float aspectRatio);
    void UpdateFollow(const DirectX::XMFLOAT3& targetPos);

    DirectX::XMMATRIX GetViewMatrix() const { return DirectX::XMLoadFloat4x4(&m_view); }
    DirectX::XMMATRIX GetProjectionMatrix() const { return DirectX::XMLoadFloat4x4(&m_proj); }

private:
    DirectX::XMFLOAT4X4 m_view;
    DirectX::XMFLOAT4X4 m_proj;
    DirectX::XMFLOAT3 m_offset = { 0.0f, 3.5f, -7.0f }; // プレイヤー後方上部のオフセット
};
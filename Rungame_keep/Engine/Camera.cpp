// ============================================================
// ファイルの役割: カメラの追従と投影行列の更新
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

// Camera.cpp
#include "Camera.h"

using namespace DirectX;

// Camera::Initialize : 必要な初期状態とリソースを準備します。
void Camera::Initialize(float aspectRatio) {
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), aspectRatio, 0.1f, 100.0f);
    XMStoreFloat4x4(&m_proj, proj);
}

// Camera::UpdateFollow : この関数の処理内容と呼び出し順序を確認してください。
void Camera::UpdateFollow(const XMFLOAT3& targetPos) {
    XMVECTOR eye = XMVectorSet(targetPos.x + m_offset.x, targetPos.y + m_offset.y, targetPos.z + m_offset.z, 0.0f);
    XMVECTOR target = XMVectorSet(targetPos.x, targetPos.y + 1.0f, targetPos.z, 0.0f);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
    XMStoreFloat4x4(&m_view, view);
}
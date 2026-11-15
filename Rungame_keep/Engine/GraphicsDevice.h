// ============================================================
// ファイルの役割: DirectX 12描画基盤の公開操作と内部リソース
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

// Engine/GraphicsDevice.h
#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

// クラスの責務: DirectX 12描画基盤の公開操作と内部リソース
class GraphicsDevice {
public:
    bool Initialize(HWND hwnd, UINT width, UINT height);
    // GPU完了を待ち、バックバッファ・深度バッファ・描画領域を再生成する。
    bool Resize(UINT width, UINT height);
    void BeginFrame();
    void EndFrame();

    ID3D12Device* GetDevice() const { return m_device.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

private:
    static const UINT FrameCount = 2;

    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;

    UINT m_rtvDescriptorSize = 0;
    UINT m_frameIndex = 0;
    UINT m_width = 0;
    UINT m_height = 0;

    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue = 0;
    HANDLE m_fenceEvent = nullptr;

    void WaitForPreviousFrame();
};
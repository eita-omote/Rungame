// ============================================================
// ファイルの役割: DirectX 12による全画面暗転演出
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdint>
#include <cstring>
#pragma comment(lib, "d3dcompiler.lib")

// Full-screen alpha-blended triangle, drawn after all 3D objects.
// This class does not create any Win32 windows and needs no resize resources.
// クラスの責務: DirectX 12による全画面暗転演出
class FadeRenderer final {
public:
    bool Initialize(ID3D12Device* device) {
        if (!device) return false;
        static constexpr char shader[] = R"HLSL(
            cbuffer FadeConstants : register(b0) { float opacity; };
            struct VOut { float4 pos : SV_POSITION; };
            VOut VSMain(uint vertexId : SV_VertexID) {
                float2 p = float2((vertexId << 1) & 2, vertexId & 2);
                VOut o;
                o.pos = float4(p * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
                return o;
            }
            float4 PSMain(VOut input) : SV_TARGET {
                return float4(7.0f / 255.0f, 12.0f / 255.0f, 24.0f / 255.0f, opacity);
            }
        )HLSL";
        D3D12_ROOT_PARAMETER param{};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param.Constants.ShaderRegister = 0;
        param.Constants.Num32BitValues = 1;
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC rootDesc{};
        rootDesc.NumParameters = 1;
        rootDesc.pParameters = &param;
        rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        Microsoft::WRL::ComPtr<ID3DBlob> rootBlob, errors;
        if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
            &rootBlob, &errors))) return false;
        if (FAILED(device->CreateRootSignature(0, rootBlob->GetBufferPointer(),
            rootBlob->GetBufferSize(), IID_PPV_ARGS(&m_root)))) return false;

        Microsoft::WRL::ComPtr<ID3DBlob> vs, ps;
        if (FAILED(D3DCompile(shader, sizeof(shader) - 1, nullptr, nullptr, nullptr,
            "VSMain", "vs_5_0", 0, 0, &vs, &errors))) return false;
        errors.Reset();
        if (FAILED(D3DCompile(shader, sizeof(shader) - 1, nullptr, nullptr, nullptr,
            "PSMain", "ps_5_0", 0, 0, &ps, &errors))) return false;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
        desc.pRootSignature = m_root.Get();
        desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
        desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        desc.RasterizerState.DepthClipEnable = TRUE;
        desc.BlendState.AlphaToCoverageEnable = FALSE;
        desc.BlendState.IndependentBlendEnable = FALSE;
        auto& blend = desc.BlendState.RenderTarget[0];
        blend.BlendEnable = TRUE;
        blend.LogicOpEnable = FALSE;
        blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blend.BlendOp = D3D12_BLEND_OP_ADD;
        blend.SrcBlendAlpha = D3D12_BLEND_ONE;
        blend.DestBlendAlpha = D3D12_BLEND_ZERO;
        blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        desc.DepthStencilState.DepthEnable = FALSE;
        desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        return SUCCEEDED(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipeline)));
    }

    void Draw(ID3D12GraphicsCommandList* commandList, float opacity) const {
        if (!commandList || !m_pipeline || opacity <= 0.0f) return;
        if (opacity > 1.0f) opacity = 1.0f;
        std::uint32_t bits = 0;
        static_assert(sizeof(bits) == sizeof(opacity), "float size mismatch");
        std::memcpy(&bits, &opacity, sizeof(bits));
        commandList->SetGraphicsRootSignature(m_root.Get());
        commandList->SetPipelineState(m_pipeline.Get());
        commandList->SetGraphicsRoot32BitConstant(0, bits, 0);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);
    }

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline;
};

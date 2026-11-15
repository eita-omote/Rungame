// ============================================================
// ファイルの役割: 床・障害物用の立方体描画
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

// Engine/CubeRenderer.cpp
#include "CubeRenderer.h"
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

const char* g_cubeShaderSource = R"(
cbuffer SceneCB : register(b0) {
    matrix g_wvp;
    float4 g_color;
};

struct VSInput {
    float3 position : POSITION;
};

struct PSInput {
    float4 position : SV_POSITION;
};

PSInput VSMain(VSInput input) {
    PSInput result;
    result.position = mul(float4(input.position, 1.0f), g_wvp);
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return g_color;
}
)";

// CubeRenderer::Initialize : 必要な初期状態とリソースを準備します。
bool CubeRenderer::Initialize(ID3D12Device* device) {
    // 1. ルートシグネチャの作成（32ビット定数＝20個: 行列16 + カラー4）
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParam.Constants.ShaderRegister = 0;
    rootParam.Constants.RegisterSpace = 0;
    rootParam.Constants.Num32BitValues = sizeof(ConstantBufferData) / 4;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = &rootParam;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature, error;
    D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

    // 2. パイプラインステート (PSO) の作成
    ComPtr<ID3DBlob> vs, ps;
    D3DCompile(g_cubeShaderSource, strlen(g_cubeShaderSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vs, nullptr);
    D3DCompile(g_cubeShaderSource, strlen(g_cubeShaderSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &ps, nullptr);

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングなし
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    // ★1. 深度テストの設定
    psoDesc.DepthStencilState.DepthEnable = TRUE; // 深度有効
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // 手前のものを描画

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;

    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    psoDesc.SampleDesc.Count = 1;

    // パイプラインステートの作成
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr)) {
        return false;
    }
    // 3. 頂点バッファ・インデックスバッファの作成
    XMFLOAT3 vertices[] = {
        { -0.5f, -0.5f, -0.5f }, { -0.5f,  0.5f, -0.5f }, {  0.5f,  0.5f, -0.5f }, {  0.5f, -0.5f, -0.5f },
        { -0.5f, -0.5f,  0.5f }, { -0.5f,  0.5f,  0.5f }, {  0.5f,  0.5f,  0.5f }, {  0.5f, -0.5f,  0.5f },
    };

    uint16_t indices[] = {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7
    };

    D3D12_HEAP_PROPERTIES heapProps = { D3D12_HEAP_TYPE_UPLOAD };

    D3D12_RESOURCE_DESC vBufferDesc = {};
    vBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vBufferDesc.Width = sizeof(vertices);
    vBufferDesc.Height = 1;
    vBufferDesc.DepthOrArraySize = 1;
    vBufferDesc.MipLevels = 1;
    vBufferDesc.SampleDesc.Count = 1;
    vBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &vBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer));
    UINT8* pVertexData = nullptr;
    m_vertexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pVertexData));
    memcpy(pVertexData, vertices, sizeof(vertices));
    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(XMFLOAT3);
    m_vertexBufferView.SizeInBytes = sizeof(vertices);

    D3D12_RESOURCE_DESC iBufferDesc = vBufferDesc;
    iBufferDesc.Width = sizeof(indices);

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &iBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer));
    UINT8* pIndexData = nullptr;
    m_indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pIndexData));
    memcpy(pIndexData, indices, sizeof(indices));
    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = sizeof(indices);

    return true;
}

// CubeRenderer::Draw : 現在の状態を描画します。
void CubeRenderer::Draw(ID3D12GraphicsCommandList* commandList,
    const XMMATRIX& world,
    const XMMATRIX& view,
    const XMMATRIX& proj,
    const XMFLOAT4& color) {
    XMMATRIX wvp = world * view * proj;
    ConstantBufferData cbData;
    XMStoreFloat4x4(&cbData.wvp, XMMatrixTranspose(wvp));
    cbData.color = color;

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    // データの直接転送（コマンドリストに直接書き込むため、複数回呼び出しても上書きされない）
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(ConstantBufferData) / 4, &cbData, 0);

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
}
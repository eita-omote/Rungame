// ============================================================
// ファイルの役割: FBXモデルのGPU描画とスキニング
// 実行コードは変更せず、学習用の説明コメントだけを追加しています。
// ============================================================

#include "FBXRenderer.h"
#include <d3dcompiler.h>
#include <cstring>
#include <limits>
#include <wincodec.h>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <vector>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace {
    const char* kShader = R"(
cbuffer SceneCB : register(b0) { row_major float4x4 g_wvp; float4 g_color; float4 g_light; };
cbuffer BoneCB : register(b1) { row_major float4x4 g_bones[256]; };
Texture2D g_diffuse : register(t0);
SamplerState g_sampler : register(s0);
struct VSInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    int4 boneIDs : BONEIDS;
    float4 weights : WEIGHTS;
};
struct PSInput { float4 position : SV_POSITION; float2 uv : TEXCOORD0; float3 normal : NORMAL; };
PSInput VSMain(VSInput input) {
    float4 bindPos = float4(input.position, 1.0f);
    float4 skinned = float4(0, 0, 0, 0);
    float3 skinnedNormal = float3(0,0,0);
    float total = 0;
    [unroll] for (int i = 0; i < 4; ++i) {
        int id = input.boneIDs[i];
        float w = input.weights[i];
        if (id >= 0 && id < 256 && w > 0.0f) {
            skinned += mul(bindPos, g_bones[id]) * w;
            skinnedNormal += mul(input.normal, (float3x3)g_bones[id]) * w;
            total += w;
        }
    }
    float4 localPos = total > 0.00001f ? skinned / total : bindPos;
    PSInput o;
    o.position = mul(localPos, g_wvp);
    o.uv = input.uv;
    o.normal = normalize(total > 0.00001f ? skinnedNormal / total : input.normal);
    return o;
}
float4 PSMain(PSInput input) : SV_TARGET {
    float3 N = normalize(input.normal);
    float3 L = normalize(g_light.xyz);
    float brightness = saturate(g_light.w + (1.0 - g_light.w) * max(dot(N,L), 0.0));
    float4 base = g_diffuse.Sample(g_sampler, input.uv) * g_color;
    return float4(base.rgb * brightness, base.a);
}
)";

    bool UploadBuffer(ID3D12Device* device, const void* source, UINT64 size,
        ComPtr<ID3D12Resource>& destination) {
        if (!size) return false;
        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleDesc.Count = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&destination)))) return false;
        void* data = nullptr;
        if (FAILED(destination->Map(0, nullptr, &data))) return false;
        std::memcpy(data, source, static_cast<size_t>(size));
        destination->Unmap(0, nullptr);
        return true;
    }
}

bool FBXRenderer::Initialize(ID3D12Device* device,
    const std::vector<SkinnedVertex>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<FBXLoaderComponent::MaterialData>& materials,
    const std::vector<FBXLoaderComponent::SubmeshData>& submeshes) {
    if (!device || vertices.empty() || indices.empty() ||
        indices.size() > (std::numeric_limits<UINT>::max)()) return false;

    // Root constants: 20 DWORDs; skin matrices: separate CBV (not root constants).
    D3D12_ROOT_PARAMETER params[3]{};
    auto& param = params[0];
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.Constants.ShaderRegister = 0;
    param.Constants.Num32BitValues = sizeof(Constants) / 4;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[1].Descriptor.ShaderRegister = 1;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    D3D12_DESCRIPTOR_RANGE srvRange{};
    srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRange.NumDescriptors = 1;
    srvRange.BaseShaderRegister = 0;
    params[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[2].DescriptorTable.NumDescriptorRanges = 1;
    params[2].DescriptorTable.pDescriptorRanges = &srvRange;
    params[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootDesc.NumStaticSamplers = 1;
    rootDesc.pStaticSamplers = &sampler;
    rootDesc.NumParameters = 3;
    rootDesc.pParameters = params;
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> signature, error;
    if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &signature, &error))) return false;
    if (FAILED(device->CreateRootSignature(0, signature->GetBufferPointer(),
        signature->GetBufferSize(), IID_PPV_ARGS(&m_root)))) return false;

    ComPtr<ID3DBlob> vs, ps;
    if (FAILED(D3DCompile(kShader, std::strlen(kShader), nullptr, nullptr, nullptr,
        "VSMain", "vs_5_0", 0, 0, &vs, &error))) return false;
    if (FAILED(D3DCompile(kShader, std::strlen(kShader), nullptr, nullptr, nullptr,
        "PSMain", "ps_5_0", 0, 0, &ps, &error))) return false;

    D3D12_INPUT_ELEMENT_DESC input[] = {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          static_cast<UINT>(offsetof(SkinnedVertex, normal)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
          static_cast<UINT>(offsetof(SkinnedVertex, uv)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
          static_cast<UINT>(offsetof(SkinnedVertex, position)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BONEIDS", 0, DXGI_FORMAT_R32G32B32A32_SINT, 0,
          static_cast<UINT>(offsetof(SkinnedVertex, boneIDs)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
          static_cast<UINT>(offsetof(SkinnedVertex, weights)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.InputLayout = { input, _countof(input) };
    pso.pRootSignature = m_root.Get();
    pso.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    pso.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    pso.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pso.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable = TRUE;
    pso.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pso.DepthStencilState.DepthEnable = TRUE;
    pso.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    pso.SampleMask = UINT_MAX;
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = 1;
    if (FAILED(device->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&m_pso)))) return false;

    const UINT64 vertexBytes = static_cast<UINT64>(vertices.size()) * sizeof(SkinnedVertex);
    const UINT64 indexBytes = static_cast<UINT64>(indices.size()) * sizeof(uint32_t);
    if (vertexBytes > (std::numeric_limits<UINT>::max)() ||
        indexBytes > (std::numeric_limits<UINT>::max)()) return false;
    if (!UploadBuffer(device, vertices.data(), vertexBytes, m_vertices) ||
        !UploadBuffer(device, indices.data(), indexBytes, m_indices)) return false;
    m_vbv.BufferLocation = m_vertices->GetGPUVirtualAddress();
    m_vbv.SizeInBytes = static_cast<UINT>(vertexBytes);
    m_vbv.StrideInBytes = sizeof(SkinnedVertex);
    m_ibv.BufferLocation = m_indices->GetGPUVirtualAddress();
    m_ibv.SizeInBytes = static_cast<UINT>(indexBytes);
    m_ibv.Format = DXGI_FORMAT_R32_UINT;
    m_indexCount = static_cast<UINT>(indices.size());
    // 256 x float4x4 = 16384 bytes (multiple of 256 for CBV alignment).
    const UINT64 boneBytes = MaxBones * sizeof(XMFLOAT4X4);
    std::vector<XMFLOAT4X4> identities(MaxBones);
    for (auto& matrix : identities) XMStoreFloat4x4(&matrix, XMMatrixIdentity());
    if (!UploadBuffer(device, identities.data(), boneBytes, m_boneBuffer)) return false;
    if (FAILED(m_boneBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_boneMapped)))) return false;
    m_submeshes = submeshes;
    if (m_submeshes.empty()) m_submeshes.push_back({ 0, m_indexCount, 0 });
    if (!InitializeTextures(device, materials)) return false;
    return true;
}

void FBXRenderer::Draw(ID3D12GraphicsCommandList* cmd, const XMMATRIX& world,
    const XMMATRIX& view, const XMMATRIX& projection,
    const std::vector<XMMATRIX>& bones) {
    if (!cmd || !m_pso || !m_indexCount) return;
    Constants cb{};
    // Row-vector mul(position, matrix), so do NOT transpose this row_major matrix.
    XMStoreFloat4x4(&cb.wvp, world * view * projection);
    cb.color = { 1,1,1,1 };
    cb.lightDirectionAmbient = { -0.35f, 0.85f, -0.40f, 0.35f };
    if (!m_boneMapped) return;
    // Current GraphicsDevice waits for GPU completion at EndFrame.
    // Do not overwrite this mapped memory while a previous frame still uses it.
    auto* matrices = reinterpret_cast<XMFLOAT4X4*>(m_boneMapped);
    for (UINT i = 0; i < MaxBones; ++i) {
        XMStoreFloat4x4(&matrices[i], i < bones.size() ? bones[i] : XMMatrixIdentity());
    }
    cmd->SetPipelineState(m_pso.Get());
    cmd->SetGraphicsRootSignature(m_root.Get());

    cmd->SetGraphicsRootConstantBufferView(1, m_boneBuffer->GetGPUVirtualAddress());
    ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
    cmd->SetDescriptorHeaps(1, heaps);

    cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd->IASetVertexBuffers(0, 1, &m_vbv);
    cmd->IASetIndexBuffer(&m_ibv);
    const UINT stride = m_srvDescriptorSize;
    for (const auto& sub : m_submeshes) {
        if (sub.startIndex >= m_indexCount || sub.indexCount > m_indexCount - sub.startIndex) continue;
        UINT mat = sub.materialIndex < m_materialColors.size() ? sub.materialIndex : 0;
        cb.color = m_materialColors[mat];
        cmd->SetGraphicsRoot32BitConstants(0, sizeof(Constants) / 4, &cb, 0);
        auto handle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += static_cast<UINT64>(mat) * stride;
        cmd->SetGraphicsRootDescriptorTable(2, handle);
        cmd->DrawIndexedInstanced(sub.indexCount, 1, sub.startIndex, 0, 0);
    }
}

bool FBXRenderer::InitializeTextures(ID3D12Device* device,
    const std::vector<FBXLoaderComponent::MaterialData>& materials) {
    const UINT count = static_cast<UINT>((std::max)(size_t(1), materials.size()));
    m_materialColors.assign(count, XMFLOAT4(1, 1, 1, 1));
    m_textures.resize(count);
    m_srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.NumDescriptors = count;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap)))) return false;
    for (UINT materialIndex = 0; materialIndex < count; ++materialIndex) {
        std::vector<unsigned char> embedded;
        std::string file;
        if (materialIndex < materials.size()) {
            embedded = materials[materialIndex].embedded;
            file = materials[materialIndex].file;
            m_materialColors[materialIndex] = materials[materialIndex].color;
        }
        std::vector<unsigned char> pixels{ 255, 255, 255, 255 };
        UINT width = 1, height = 1;
        std::vector<unsigned char> encoded = embedded;
        if (encoded.empty() && !file.empty()) {
            std::ifstream in(file, std::ios::binary);
            if (in) encoded.assign(std::istreambuf_iterator<char>(in), {});
        }
        if (!encoded.empty() && encoded.size() <= UINT_MAX) {
            HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool uninit = SUCCEEDED(co);
            ComPtr<IWICImagingFactory> factory;
            ComPtr<IWICStream> stream;
            ComPtr<IWICBitmapDecoder> decoder;
            ComPtr<IWICBitmapFrameDecode> frame;
            ComPtr<IWICFormatConverter> converter;
            HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
            if (SUCCEEDED(hr)) hr = factory->CreateStream(&stream);
            if (SUCCEEDED(hr)) hr = stream->InitializeFromMemory(encoded.data(),
                static_cast<DWORD>(encoded.size()));
            if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromStream(stream.Get(), nullptr,
                WICDecodeMetadataCacheOnLoad, &decoder);
            if (SUCCEEDED(hr)) hr = decoder->GetFrame(0, &frame);
            if (SUCCEEDED(hr)) hr = frame->GetSize(&width, &height);
            if (SUCCEEDED(hr)) hr = factory->CreateFormatConverter(&converter);
            if (SUCCEEDED(hr)) hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
            if (SUCCEEDED(hr) && (!width || !height || width > 8192 || height > 8192)) hr = E_INVALIDARG;
            if (SUCCEEDED(hr)) {
                pixels.resize(static_cast<size_t>(width) * height * 4);
                hr = converter->CopyPixels(nullptr, width * 4,
                    static_cast<UINT>(pixels.size()), pixels.data());
            }
            if (FAILED(hr)) {
                width = height = 1;
                pixels = { 255, 255, 255, 255 };
                OutputDebugStringA("[FBXRenderer] Texture decode failed; using white.\n");
            }
            else {
                m_hasTexture = true;
            }
            converter.Reset(); frame.Reset(); decoder.Reset(); stream.Reset(); factory.Reset();
            if (uninit) CoUninitialize();
        }
        else {
            OutputDebugStringA("[FBXRenderer] Diffuse texture not found; using white.\n");
        }

        D3D12_RESOURCE_DESC textureDesc{};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Width = width;
        textureDesc.Height = height;
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        if (FAILED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE,
            &textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(&m_textures[materialIndex])))) return false;

        UINT64 totalBytes = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{};
        UINT numRows = 0;
        UINT64 rowSize = 0;
        device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &layout,
            &numRows, &rowSize, &totalBytes);
        D3D12_RESOURCE_DESC uploadDesc{};
        uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadDesc.Width = totalBytes;
        uploadDesc.Height = 1;
        uploadDesc.DepthOrArraySize = 1;
        uploadDesc.MipLevels = 1;
        uploadDesc.SampleDesc.Count = 1;
        uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES uploadHeap{};
        uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
        ComPtr<ID3D12Resource> upload;
        if (FAILED(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
            &uploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&upload)))) return false;
        unsigned char* mapped = nullptr;
        if (FAILED(upload->Map(0, nullptr, reinterpret_cast<void**>(&mapped)))) return false;
        for (UINT y = 0; y < height; ++y) {
            std::memcpy(mapped + layout.Offset + static_cast<size_t>(y) * layout.Footprint.RowPitch,
                pixels.data() + static_cast<size_t>(y) * width * 4, static_cast<size_t>(width) * 4);
        }
        upload->Unmap(0, nullptr);

        // One-time command queue and fence: no GraphicsDevice interface changes needed.
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> list;
        D3D12_COMMAND_QUEUE_DESC qDesc{};
        qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        if (FAILED(device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&queue))) ||
            FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&allocator))) ||
            FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                allocator.Get(), nullptr, IID_PPV_ARGS(&list)))) return false;
        D3D12_TEXTURE_COPY_LOCATION dst{};
        dst.pResource = m_textures[materialIndex].Get();
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;
        D3D12_TEXTURE_COPY_LOCATION src{};
        src.pResource = upload.Get();
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint = layout;
        list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_textures[materialIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        list->ResourceBarrier(1, &barrier);
        if (FAILED(list->Close())) return false;
        ID3D12CommandList* commands[] = { list.Get() };
        queue->ExecuteCommandLists(1, commands);
        ComPtr<ID3D12Fence> fence;
        if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))) ||
            FAILED(queue->Signal(fence.Get(), 1))) return false;
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!eventHandle) return false;
        HRESULT waitResult = fence->SetEventOnCompletion(1, eventHandle);
        if (SUCCEEDED(waitResult)) WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
        if (FAILED(waitResult)) return false;

        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels = 1;
        auto cpu = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
        cpu.ptr += static_cast<SIZE_T>(materialIndex) * m_srvDescriptorSize;
        device->CreateShaderResourceView(m_textures[materialIndex].Get(), &srv, cpu);
    }
    return true;
}

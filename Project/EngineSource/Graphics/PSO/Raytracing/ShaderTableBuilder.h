#pragma once
#include "ShaderTable.h"
#include <d3d12.h>
#include <wrl.h>
#include <cassert>

namespace GameEngine {

    class ShaderTableBuilder {
    public:

        // 各テーブルへのアクセス
        ShaderTable& RayGen() { return raygenTable_; }
        ShaderTable& Miss() { return missTable_; }
        ShaderTable& HitGroup() { return hitgroupTable_; }

        // テーブルを作成
        void Build(ID3D12Device* device);

        // DispatchRaysDescを生成
        D3D12_DISPATCH_RAYS_DESC CreateDispatchRaysDesc(UINT width, UINT height) const;

        ID3D12Resource* GetBuffer() const { return shaderTable_.Get(); }

    private:
        ShaderTable raygenTable_;
        ShaderTable missTable_;
        ShaderTable hitgroupTable_;

        Microsoft::WRL::ComPtr<ID3D12Resource> shaderTable_;
        D3D12_GPU_VIRTUAL_ADDRESS raygenAddr_ = 0;
        D3D12_GPU_VIRTUAL_ADDRESS missAddr_ = 0;
        D3D12_GPU_VIRTUAL_ADDRESS hitgroupAddr_ = 0;

        static constexpr UINT TABLE_ALIGN =
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

        static UINT Align(UINT size, UINT alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }

        static Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(
            ID3D12Device* device, UINT size)
        {
            D3D12_HEAP_PROPERTIES heapProps{};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

            D3D12_RESOURCE_DESC resDesc{};
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resDesc.Width = size;
            resDesc.Height = 1;
            resDesc.DepthOrArraySize = 1;
            resDesc.MipLevels = 1;
            resDesc.SampleDesc.Count = 1;
            resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

            Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
            HRESULT hr = device->CreateCommittedResource(
                &heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&buffer));

            assert(SUCCEEDED(hr) && "Failed to create ShaderTable buffer");
            return buffer;
        }
    };

}
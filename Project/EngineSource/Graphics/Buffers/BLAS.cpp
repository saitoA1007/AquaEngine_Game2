#include "pch.h"
#include "BLAS.h"
#include "CreateBufferResource.h"
#include <algorithm>
using namespace GameEngine;

void BLAS::Create(ID3D12GraphicsCommandList4* cmdList,
    const D3D12_VERTEX_BUFFER_VIEW& vertexBufView, const D3D12_INDEX_BUFFER_VIEW& indexBufView,
    const uint32_t& totalVertices, const uint32_t& totalIndices, const bool& isUpdate) {

    // ジオメトリ情報を設定
    geometryDesc_.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc_.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    // 頂点データ
    geometryDesc_.Triangles.VertexBuffer.StartAddress = vertexBufView.BufferLocation;
    geometryDesc_.Triangles.VertexBuffer.StrideInBytes = vertexBufView.StrideInBytes;
    geometryDesc_.Triangles.VertexCount = totalVertices;
    geometryDesc_.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    // インデックスデータ
    geometryDesc_.Triangles.IndexBuffer = indexBufView.BufferLocation;
    geometryDesc_.Triangles.IndexCount = totalIndices;
    geometryDesc_.Triangles.IndexFormat = indexBufView.Format;

    // 1つのMeshの入力設定
    inputs_.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL; // BLASを指定
    inputs_.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs_.pGeometryDescs = &geometryDesc_;
    inputs_.NumDescs = 1;
    inputs_.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    // 更新用にする
    if (isUpdate) {
        inputs_.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }

    // 必要なバッファサイズをGPUに計算させる
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
    device_->GetRaytracingAccelerationStructurePrebuildInfo(&inputs_, &prebuildInfo);

    // BLASのデータリソース
    resource_ = CreateBufferResource(
        device_,
        prebuildInfo.ResultDataMaxSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,                                       
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, // DXR専用ステート  
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS                    
    );

    // メモリ不足にならないように、サイズの大きい方を取得する
    size_t scratchSize = (std::max)(prebuildInfo.ScratchDataSizeInBytes, prebuildInfo.UpdateScratchDataSizeInBytes);

    // 構築に使用する作業メモリ
    scratchBuffer_ = CreateBufferResource(
        device_,
        scratchSize,
        D3D12_HEAP_TYPE_DEFAULT,                                    
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // UAVステート              
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS                  
    );

    // ビルド実行
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
    buildDesc.Inputs = inputs_;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer_->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = resource_->GetGPUVirtualAddress();
    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // ビルド完了を待つUAVバリアを張る
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource_.Get();
    cmdList->ResourceBarrier(1, &barrier);
}

void BLAS::Update(ID3D12GraphicsCommandList4* cmdList, const D3D12_VERTEX_BUFFER_VIEW& vertexBufView) {

    // 頂点バッファのアドレスを更新
    geometryDesc_.Triangles.VertexBuffer.StartAddress = vertexBufView.BufferLocation;
    geometryDesc_.Triangles.VertexBuffer.StrideInBytes = vertexBufView.StrideInBytes;

    // 入力設定を更新
    inputs_.pGeometryDescs = &geometryDesc_;
    inputs_.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE
        | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE
        | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

    // ビルド実行
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
    buildDesc.Inputs = inputs_;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer_->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = resource_->GetGPUVirtualAddress();
    buildDesc.SourceAccelerationStructureData = resource_->GetGPUVirtualAddress();
    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // ビルド完了を待つUAVバリアを張る
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.UAV.pResource = resource_.Get();
    cmdList->ResourceBarrier(1, &barrier);
}
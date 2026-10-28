#include "pch.h"
#include "ShaderTableBuilder.h"

using namespace GameEngine;

void ShaderTableBuilder::Build(ID3D12Device* device) {
    // RayGenは必ず1レコードのみ
    if (raygenTable_.RecordCount() != 1) {
        assert(false && "ShaderTableBuilder: RayGen table must have exactly 1 record.");
    }

    // 各テーブルのサイズをD3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENTに揃える
    UINT raygenSize = Align(raygenTable_.TableSize(), TABLE_ALIGN);
    UINT missSize = Align(missTable_.TableSize(), TABLE_ALIGN);
    UINT hitgroupSize = Align(hitgroupTable_.TableSize(), TABLE_ALIGN);
    UINT totalSize = raygenSize + missSize + hitgroupSize;

    // UPLOADヒープにGPU バッファを作成する
    shaderTable_ = CreateUploadBuffer(device, totalSize);

    // バッファにマップして書き込む
    uint8_t* mapped = nullptr;
    shaderTable_->Map(0, nullptr, reinterpret_cast<void**>(&mapped));

    raygenTable_.WriteAll(mapped);
    missTable_.WriteAll(mapped + raygenSize);
    hitgroupTable_.WriteAll(mapped + raygenSize + missSize);

    shaderTable_->Unmap(0, nullptr);

    // DispatchRaysDescのためにGPUアドレスを記録しておく
    D3D12_GPU_VIRTUAL_ADDRESS base = shaderTable_->GetGPUVirtualAddress();
    raygenAddr_ = base;
    missAddr_ = base + raygenSize;
    hitgroupAddr_ = base + raygenSize + missSize;
}

D3D12_DISPATCH_RAYS_DESC ShaderTableBuilder::CreateDispatchRaysDesc(UINT width, UINT height) const {
    D3D12_DISPATCH_RAYS_DESC desc{};

    // RayGen
    desc.RayGenerationShaderRecord.StartAddress = raygenAddr_;
    desc.RayGenerationShaderRecord.SizeInBytes = raygenTable_.RecordStride();

    // Miss
    desc.MissShaderTable.StartAddress = missAddr_;
    desc.MissShaderTable.SizeInBytes = missTable_.TableSize();
    desc.MissShaderTable.StrideInBytes = missTable_.RecordStride();

    // HitGroup
    desc.HitGroupTable.StartAddress = hitgroupAddr_;
    desc.HitGroupTable.SizeInBytes = hitgroupTable_.TableSize();
    desc.HitGroupTable.StrideInBytes = hitgroupTable_.RecordStride();

    // 画面サイズ
    desc.Width = width;
    desc.Height = height;
    desc.Depth = 1;

    return desc;
}
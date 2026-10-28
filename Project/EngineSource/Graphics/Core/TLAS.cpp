#include "pch.h"
#include "TLAS.h"
#include <algorithm>
#include "CreateBufferResource.h"
#include "ResourceGarbageCollector.h"
using namespace GameEngine;

TLAS::~TLAS() {
    if (isCreated_) {
        // GPU側の実行が完了してから破棄させる
        if (resource_) {
            ResourceGarbageCollector::GetInstance().Add(resource_);
        }
        if (instanceBuffer_) {
            instanceBuffer_->Unmap(0, nullptr);
            ResourceGarbageCollector::GetInstance().Add(instanceBuffer_);
        }
        if (scratchBuffer_) {
            ResourceGarbageCollector::GetInstance().Add(scratchBuffer_);
        }
        if (srvManager_) {
            srvManager_->ReleaseIndex(srvIndex_);
        }
    }
}

void TLAS::Create(ID3D12GraphicsCommandList4* cmdList, const uint32_t& initialCapacity) {
    // SRV用のスロットは容量拡張時も使い回すため、ここで一度だけ確保する
    srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::AccessData);
    srvHandleCPU_ = srvManager_->GetCPUHandle(srvIndex_);
    srvHandleGPU_ = srvManager_->GetGPUHandle(srvIndex_);

    AllocateBuffers(initialCapacity == 0 ? 1 : initialCapacity);
    CreateSrv();

    isCreated_ = true;
}

void TLAS::AllocateBuffers(uint32_t capacity) {
    maxInstanceCount_ = capacity;
    uint64_t instanceBufferSize = sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * maxInstanceCount_;

    // GPU側の実行が完了してから破棄
    if (instanceBuffer_) {
        instanceBuffer_->Unmap(0, nullptr);
        ResourceGarbageCollector::GetInstance().Add(instanceBuffer_);
    }
    if (resource_) {
        ResourceGarbageCollector::GetInstance().Add(resource_);
    }
    if (scratchBuffer_) {
        ResourceGarbageCollector::GetInstance().Add(scratchBuffer_);
    }

    // GPUに送るためのリソースを作成
    instanceBuffer_ = CreateBufferResource(
        device_, instanceBufferSize,
        D3D12_HEAP_TYPE_UPLOAD,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_FLAG_NONE
    );

    // インスタンス情報をD3D12の構造体にマッピングして書き込む
    instanceBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs_));

    // ビルドの入力設定
    inputs_.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    inputs_.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    inputs_.InstanceDescs = instanceBuffer_->GetGPUVirtualAddress();
    inputs_.NumDescs = maxInstanceCount_;
    // PERFORM_UPDATEによるリフィットを行うためにはALLOW_UPDATEを立てておく必要がある
    inputs_.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE
        | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

    // バッファを生成
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo{};
    device_->GetRaytracingAccelerationStructurePrebuildInfo(&inputs_, &prebuildInfo);

    // TLASのデータリソースを作成
    resource_ = CreateBufferResource(
        device_, prebuildInfo.ResultDataMaxSizeInBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    // 作業リソースを作成
    uint64_t scratchSize = (std::max)(prebuildInfo.ScratchDataSizeInBytes, prebuildInfo.UpdateScratchDataSizeInBytes);
    scratchBuffer_ = CreateBufferResource(
        device_, scratchSize,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
    );

    // リソースを作り直したので、次のUpdateではリフィットではなく必ずフルビルドさせる
    previousInstanceCount_ = UINT32_MAX;
}

void TLAS::CreateSrv() {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = resource_->GetGPUVirtualAddress();
    device_->CreateShaderResourceView(nullptr, &srvDesc, srvHandleCPU_);
}

void TLAS::EnsureCapacity(uint32_t requiredCount) {
    if (requiredCount <= maxInstanceCount_) { return; }

    // 頻繁な再確保を避けるため、要求数と現容量の倍のうち大きい方まで余裕を持って拡張する
    uint32_t newCapacity = (std::max)(requiredCount, maxInstanceCount_ * 2);

    AllocateBuffers(newCapacity);
    CreateSrv();
}

void TLAS::Update(ID3D12GraphicsCommandList4* cmdList, const std::vector<TLASInstanceData>& instances) {
    uint32_t activeCount = static_cast<uint32_t>(instances.size());

    // 0であれば早期リターン
    if (activeCount == 0) { return; }

    // 容量が足りなければ作り直す
    EnsureCapacity(activeCount);

    // 有効なインスタンスの状態を書き込む
    for (uint32_t i = 0; i < activeCount; ++i) {
        instanceDescs_[i].InstanceID = instances[i].instanceID;
        instanceDescs_[i].InstanceContributionToHitGroupIndex = instances[i].hitGroupIndexOffset;
        instanceDescs_[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

        // Transform行列のコピー
        std::memcpy(instanceDescs_[i].Transform, instances[i].transform, sizeof(float) * 12);

        // 対象となるBLASのGPUアドレスを指定
        instanceDescs_[i].AccelerationStructure = instances[i].blas->GetGpuVirtualAddress();

        // レイキャスト時のフィルタリング
        instanceDescs_[i].InstanceMask = instances[i].instanceMask;
    }

    // 有効数でビルドを構築
    inputs_.NumDescs = activeCount;

    // 前フレームとインスタンス数が同じであればトポロジが変わっていないためPERFORM_UPDATEを行い、異なる場合はフルビルドを行う。
    // 一定回数で必ずフルビルドに戻す
    bool canRefit = (activeCount == previousInstanceCount_) && (consecutiveRefitCount_ < kMaxConsecutiveRefits_);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
    buildDesc.Inputs = inputs_;
    if (canRefit) {
        buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        // インプレース更新
        buildDesc.SourceAccelerationStructureData = resource_->GetGPUVirtualAddress();
        ++consecutiveRefitCount_;
    } else {
        // フルビルドでBVHの質が戻るのでカウントをリセットする
        consecutiveRefitCount_ = 0;
    }
    buildDesc.ScratchAccelerationStructureData = scratchBuffer_->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = resource_->GetGPUVirtualAddress();
    cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // バリア生成
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource_.Get();
    cmdList->ResourceBarrier(1, &barrier);

    previousInstanceCount_ = activeCount;
}
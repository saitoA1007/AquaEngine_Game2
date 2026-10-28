#pragma once
#include "SrvResource.h"
#include "CreateBufferResource.h"
#include "ResourceGarbageCollector.h"

namespace GameEngine {

	/// <summary>
	/// 構造化バッファ用
	/// </summary>
	template <typename T>
	class StructuredBuffer : public SrvResource {
	public:
		~StructuredBuffer() {
			if (isCreated_) {
				// Unmap
				if (data_) {
					resource_->Unmap(0, nullptr);
					data_ = nullptr;
				}
				// srvの解放
				if (srvManager_) {
					srvManager_->ReleaseIndex(srvIndex_);

					if (enableUAV_) {
						srvManager_->ReleaseIndex(uavIndex_);
					}
				}
			}
		}

		/// <summary>
		/// 要素数を指定してバッファを作成
		/// </summary>
		void Create(uint32_t numElements = 1, SrvHeapType type = SrvHeapType::Buffer) {
			if (isCreated_) { return; }

			numElements_ = numElements;

			// リソースを作成
			resource_ = CreateBufferResource(device_, sizeof(T) * numElements_);
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));

			// SRVのインデックスを取得
			srvIndex_ = srvManager_->AllocateSrvIndex(type);
			// SRVの作成
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			srvDesc.Buffer.NumElements = numElements_;
			srvDesc.Buffer.StructureByteStride = sizeof(T);
			srvCpuHandle_ = srvManager_->GetCPUHandle(srvIndex_);
			srvGpuHandle_ = srvManager_->GetGPUHandle(srvIndex_);
			device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCpuHandle_);

			isCreated_ = true;
		}

		void CreateTypeless(uint32_t numElements = 1, SrvHeapType type = SrvHeapType::Buffer) {
			if (isCreated_) { return; }

			numElements_ = numElements;

			// 4バイト境界に切り上げたサイズを計算
			uint32_t bufferSize = ((sizeof(T) * numElements) + 3) & ~3;

			// リソースを作成
			resource_ = CreateBufferResource(device_, bufferSize);
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));

			// SRVのインデックスを取得
			srvIndex_ = srvManager_->AllocateSrvIndex(type);
			// SRVの作成
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.NumElements = bufferSize / 4;
			srvCpuHandle_ = srvManager_->GetCPUHandle(srvIndex_);
			srvGpuHandle_ = srvManager_->GetGPUHandle(srvIndex_);
			device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCpuHandle_);

			isCreated_ = true;
		}

		void Create(ID3D12GraphicsCommandList4* cmdList, const std::vector<T>& data, SrvHeapType type = SrvHeapType::Buffer) {
			if (isCreated_) { return; }

			numElements_ = static_cast<uint32_t>(data.size());
			size_t sizeInBytes = sizeof(T) * numElements_;

			// DEFAULTヒープのリソースを作成
			resource_ = CreateBufferResource(
				device_,
				sizeInBytes,
				D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_STATE_COPY_DEST, // コピーを待つ状態
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			);

			// UPLOADヒープのステージングバッファを作成
			Microsoft::WRL::ComPtr<ID3D12Resource> stagingBuffer = CreateBufferResource(
				device_,
				sizeInBytes,
				D3D12_HEAP_TYPE_UPLOAD,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				D3D12_RESOURCE_FLAG_NONE
			);

			// ステージングバッファにCPUデータをコピー
			void* mappedData = nullptr;
			stagingBuffer->Map(0, nullptr, &mappedData);
			std::memcpy(mappedData, data.data(), sizeInBytes);
			stagingBuffer->Unmap(0, nullptr);

			// GPU側でコピーコマンドを発行
			cmdList->CopyBufferRegion(resource_.Get(), 0, stagingBuffer.Get(), 0, sizeInBytes);

			// リソースの状態を遷移
			TransitionResource(cmdList, resource_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			// リソースの破棄を登録する
			ResourceGarbageCollector::GetInstance().Add(stagingBuffer);

			/// SRVの作成
			{
				srvIndex_ = srvManager_->AllocateSrvIndex(type);

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Buffer.FirstElement = 0;
				srvDesc.Buffer.NumElements = numElements_;
				srvDesc.Buffer.StructureByteStride = sizeof(T);
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

				srvCpuHandle_ = srvManager_->GetCPUHandle(srvIndex_);
				srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
				device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCpuHandle_);
			}

			/// UAVの作成
			{
				uavIndex_ = srvManager_->AllocateSrvIndex(type);

				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = numElements_;
				uavDesc.Buffer.StructureByteStride = sizeof(T);
				uavDesc.Buffer.CounterOffsetInBytes = 0;
				uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

				D3D12_CPU_DESCRIPTOR_HANDLE uavCPU = srvManager_->GetCPUHandle(uavIndex_);
				uavGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(uavIndex_));
				device_->CreateUnorderedAccessView(resource_.Get(), nullptr, &uavDesc, uavCPU);
			}

			isCreated_ = true;
			enableUAV_ = true;
			isSrvState_ = true;
		}

		void CreateForUAV(uint32_t numElements, SrvHeapType type = SrvHeapType::Buffer) {
			if (isCreated_) { return; }

			numElements_ = numElements;
			size_t sizeInBytes = sizeof(T) * numElements_;

			// 最初からuavとして作成
			resource_ = CreateBufferResource(
				device_,
				sizeInBytes,
				D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			);

			/// SRVの作成
			{
				srvIndex_ = srvManager_->AllocateSrvIndex(type);

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Buffer.FirstElement = 0;
				srvDesc.Buffer.NumElements = numElements_;
				srvDesc.Buffer.StructureByteStride = sizeof(T);
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

				srvCpuHandle_ = srvManager_->GetCPUHandle(srvIndex_);
				srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
				device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCpuHandle_);
			}

			/// UAVの作成
			{
				uavIndex_ = srvManager_->AllocateSrvIndex(type);

				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = numElements_;
				uavDesc.Buffer.StructureByteStride = sizeof(T);
				uavDesc.Buffer.CounterOffsetInBytes = 0;
				uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

				D3D12_CPU_DESCRIPTOR_HANDLE uavCPU = srvManager_->GetCPUHandle(uavIndex_);
				uavGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(uavIndex_));
				device_->CreateUnorderedAccessView(resource_.Get(), nullptr, &uavDesc, uavCPU);
			}

			// 状態フラグの設定
			isCreated_ = true;
			enableUAV_ = true;
			isSrvState_ = false;
		}

		// 解放する。
		void Release() {
			if (!isCreated_) { return; }
			// Unmap
			if (data_) {
				resource_->Unmap(0, nullptr);
				data_ = nullptr;
			}
			// srvの解放
			if (srvManager_) {
				srvManager_->ReleaseIndex(srvIndex_);
				if (enableUAV_) {
					srvManager_->ReleaseIndex(uavIndex_);
				}
			}
			resource_.Reset();
			numElements_ = 0;
			enableUAV_ = false;
			isSrvState_ = false;
			isCreated_ = false;
		}

		T* GetData() const { return data_; }
		uint32_t GetNumElements() const { return numElements_; }

		// uav
		uint32_t GetUAVIndex() const { return uavIndex_; }
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetUAVGpuHandle() const { return uavGpuHandle_; }

		// uavにリソース状態を遷移
		void TransitionUAV(ID3D12GraphicsCommandList4* cmdList) {
			if (!enableUAV_ || !isSrvState_) { return; }
			// 遷移
			TransitionResource(cmdList, resource_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			isSrvState_ = false;
		}

		// uavからuavへ
		void BarrierUAVForUAV(ID3D12GraphicsCommandList4* cmdList) {
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = resource_.Get();
			cmdList->ResourceBarrier(1, &barrier);
		}

		// srvにリソースを遷移
		void TransitionSRV(ID3D12GraphicsCommandList4* cmdList) {
			if (!enableUAV_ || isSrvState_) { return; }
			// 遷移
			TransitionResource(cmdList, resource_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			isSrvState_ = true;
		}

	private:
		bool isCreated_ = false;

		T* data_ = nullptr;
		uint32_t numElements_ = 0;

		// uav
		uint32_t uavIndex_ = 0;
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavGpuHandle_{};
		bool enableUAV_ = false;

		bool isSrvState_ = false;
	};
}
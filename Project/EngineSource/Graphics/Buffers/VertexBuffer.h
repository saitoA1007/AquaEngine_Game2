#pragma once
#include <vector>
#include "SrvResource.h"
#include "CreateBufferResource.h"
#include "ResourceGarbageCollector.h"

namespace GameEngine {

	/// <summary>
	/// 頂点データ用クラス
	/// </summary>
	template <typename T>
	class VertexBuffer : public SrvResource {
	public:
		~VertexBuffer() {
			if (isCreated_) {
				// マッピングを解除する
				if (vertexData_) {
					resource_->Unmap(0, nullptr);
					vertexData_ = nullptr;
				}
				// SRV,UAVインデックス解放
				if (srvManager_) {
					srvManager_->ReleaseIndex(srvIndex_);

					if (enableUAV_) {
						srvManager_->ReleaseIndex(uavIndex_);
					}
				}
				isCreated_ = false;
			}
		}

		void Create(const std::vector<T>& vertices) {
			// 頂点数を取得
			totalVertices_ = static_cast<uint32_t>(vertices.size());

			// 頂点バッファを作成
			resource_ = CreateBufferResource(device_, sizeof(T) * totalVertices_);
			// リソースの先頭のアドレスから使う
			vertexBufferView_.BufferLocation = resource_->GetGPUVirtualAddress();
			// 使用するリソースのサイズは頂点3つ分のサイズ
			vertexBufferView_.SizeInBytes = sizeof(T) * totalVertices_;
			// 1頂点あたりのサイズ
			vertexBufferView_.StrideInBytes = sizeof(T);

			// 頂点データを生成
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
			// 頂点データをコピー
			std::memcpy(vertexData_, vertices.data(), sizeof(T) * totalVertices_);

			/// SRVの作成
			srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = totalVertices_;
			srvDesc.Buffer.StructureByteStride = sizeof(T);
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			srvCpuHandle_ = srvManager_->GetCPUHandle(srvIndex_);
			srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
			device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCpuHandle_);

			isCreated_ = true;
		}

		void Create(ID3D12GraphicsCommandList4* cmdList, const std::vector<T>& vertices) {
			// 頂点数を取得
			totalVertices_ = static_cast<uint32_t>(vertices.size());
			size_t sizeInBytes = sizeof(T) * totalVertices_;

			// DEFAULTヒープのリソースを作成
			resource_ = CreateBufferResource(
				device_,
				sizeInBytes,
				D3D12_HEAP_TYPE_DEFAULT,
				D3D12_RESOURCE_STATE_COPY_DEST, // コピーを待つ状態
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
			);

			// リソースの先頭のアドレスから使う
			vertexBufferView_.BufferLocation = resource_->GetGPUVirtualAddress();
			// 使用するリソースのサイズは頂点3つ分のサイズ
			vertexBufferView_.SizeInBytes = sizeof(T) * totalVertices_;
			// 1頂点あたりのサイズ
			vertexBufferView_.StrideInBytes = sizeof(T);

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
			std::memcpy(mappedData, vertices.data(), sizeInBytes);
			stagingBuffer->Unmap(0, nullptr);

			// GPU側でコピーコマンドを発行
			cmdList->CopyBufferRegion(resource_.Get(), 0, stagingBuffer.Get(), 0, sizeInBytes);

			// リソースの状態を遷移
			TransitionResource(cmdList, resource_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			// リソースの破棄を登録する
			ResourceGarbageCollector::GetInstance().Add(stagingBuffer);

			/// SRVの作成
			{
				srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Buffer.FirstElement = 0;
				srvDesc.Buffer.NumElements = totalVertices_;
				srvDesc.Buffer.StructureByteStride = sizeof(T);
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

				srvCpuHandle_ = srvManager_->GetCPUHandle(srvIndex_);
				srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
				device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCpuHandle_);

			}
			
			/// UAVの作成
			{
				uavIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = totalVertices_;
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

		// ビューを取得
		const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return vertexBufferView_; }

		// 頂点データを取得
		T* GetVertexData() { return vertexData_; }

		// 頂点数
		uint32_t GetTotalVertices() const { return totalVertices_; }

		uint32_t GetUAVIndex() const { return uavIndex_; }
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetUAVGpuHandle() const { return uavGpuHandle_; }

		// uavにリソース状態を遷移
		void TransitionUAV(ID3D12GraphicsCommandList4* cmdList) {
			if (!isSrvState_) { return; }
			// 遷移
			TransitionResource(cmdList, resource_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			isSrvState_ = false;
		}

		// srvにリソースを遷移
		void TransitionSRV(ID3D12GraphicsCommandList4* cmdList) {
			if (isSrvState_) { return; }
			// 遷移
			TransitionResource(cmdList, resource_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			isSrvState_ = true;
		}

	private:
		// 頂点バッファビュー
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
		// メッシュデータ
		T* vertexData_ = nullptr;
		// 頂点数
		uint32_t totalVertices_ = 0;

		uint32_t uavIndex_ = 0;
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavGpuHandle_{};
		bool enableUAV_ = false;

		bool isSrvState_ = false;
		bool isCreated_ = false;
	};
}
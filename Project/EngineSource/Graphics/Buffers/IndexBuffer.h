#pragma once
#include <vector>
#include "SrvResource.h"
#include "CreateBufferResource.h"

namespace GameEngine {

	/// <summary>
	/// インデックスデータ用クラス
	/// </summary>
	class IndexBuffer : public SrvResource {
	public:
		~IndexBuffer() {
			if (isCreated_) {
				// SRV インデックス解放
				if (srvManager_) {
					srvManager_->ReleaseIndex(srvIndex_);
				}
				isCreated_ = false;
			}
		}

		void Create(const std::vector<uint32_t>& indices) {
			// インデックス数を取得
			totalIndices_ = static_cast<uint32_t>(indices.size());

			// インデックスバッファを作成
			resource_ = CreateBufferResource(device_, sizeof(uint32_t) * totalIndices_);
			// リソースの先頭のアドレスから使う
			indexBufferView_.BufferLocation = resource_->GetGPUVirtualAddress();
			// 使用するリソースのサイズはインデックス6つ分のサイズ
			indexBufferView_.SizeInBytes = sizeof(uint32_t) * totalIndices_;
			// インデックスはuint32_tとする
			indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

			// インデックスデータを生成
			uint32_t* indexData = nullptr;
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));
			// インデックスデータをコピー
			std::memcpy(indexData, indices.data(), sizeof(uint32_t) * totalIndices_);

			// UnMapする
			resource_->Unmap(0, nullptr);
			indexData = nullptr;

			/// SRVの作成
			srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::Buffer);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Buffer.NumElements = totalIndices_;
			srvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

			srvCpuHandle_ = srvManager_->GetCPUHandle(srvIndex_);
			srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
			device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCpuHandle_);

			isCreated_ = true;
		}

		// ビューを取得
		const D3D12_INDEX_BUFFER_VIEW& GetView() const { return indexBufferView_; }

		// インデックス数
		uint32_t GetTotalIndices() const { return totalIndices_; }

	private:
		// インデックスバッファビュー
		D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
		// 頂点数
		uint32_t totalIndices_ = 0;

		bool isCreated_ = false;
	};
}
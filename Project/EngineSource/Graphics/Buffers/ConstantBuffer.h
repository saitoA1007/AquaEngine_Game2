#pragma once
#include "GpuResource.h"
#include "CreateBufferResource.h"

namespace GameEngine {

	/// <summary>
	/// 定数バッファ用クラス
	/// </summary>
	template <typename T>
	class ConstantBuffer : public GpuResource {
	public:
		~ConstantBuffer() {
			// デストラクタで自動的にUnmapを実行
			if (data_) {
				resource_->Unmap(0, nullptr);
				data_ = nullptr;
			}
		}

		void Create() {
			resource_ = CreateBufferResource(device_, sizeof(T));

			// データを書き込むためのポインタを取得し、保持し続ける
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
		}

		void Create(uint32_t num) {
			resource_ = CreateBufferResource(device_, sizeof(T) * num);

			// データを書き込むためのポインタを取得し、保持し続ける
			resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
		}

		// 解放
		void Release() {
			if (data_) {
				resource_->Unmap(0, nullptr);
				data_ = nullptr;
			}
			resource_.Reset();
		}

		// データを取得する
		T* GetData() const { return data_; }

	private:
		// データのポインタ
		T* data_ = nullptr;
	};
}
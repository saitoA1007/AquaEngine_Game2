#pragma once
#include <memory>
#include <vector>
#include "PackedGeometryBuffer.h"
#include "IndexBuffer.h"
#include "BLAS.h"
#include "RefBuffer.h"

namespace GameEngine {

	/// <summary>
	/// 静的なチャンク群を、内部の切断面を除いた1つのメッシュ、1つのBLASにまとめてレイトレ描画する
	/// </summary>
	class FractureStaticMesh {
	public:
		FractureStaticMesh() = default;
		~FractureStaticMesh() = default;

		// BLASの構築に使うコマンドリストを設定
		static void StaticInitialize(ID3D12GraphicsCommandList4* commandList) { commandList_ = commandList; }

		// 頂点を共有するPackedGeometryBufferを設定
		void Initialize(const PackedGeometryBuffer* geometryBuffer);

		// レイトレで使用するマテリアルとインスタンスマスクを設定
		void SetMaterial(uint32_t materialSrvIndex, uint32_t instanceMask);

		/// <summary>
		/// 静的なチャンクの判定を元にインデックスを作り直す。GPU側への反映はFlushIfDirty()で行う
		/// </summary>
		/// <param name="isStatic">chunkIdが静的なチャンクか</param>
		template <typename IsStaticFunc>
		void Rebuild(IsStaticFunc&& isStatic) {
			if (geometryBuffer_ == nullptr) { return; }
			pendingIndices_ = geometryBuffer_->BuildStaticIndices(isStatic);
			isDirty_ = true;
		}

		// インデックスが作り直されていれば、インデックスバッファとBLASを作り直す
		void FlushIfDirty();

		// 描画する三角形を持っているか
		bool HasGeometry() const { return blas_ != nullptr && indexCount_ > 0; }

		BLAS* GetBLAS() const { return blas_.get(); }
		uint32_t GetRefIndex() const { return refBuffer_.GetRefIndex(); }
		uint32_t GetInstanceMask() const { return refBuffer_.GetInstanceMask(); }

	private:
		// コピー禁止
		FractureStaticMesh(const FractureStaticMesh&) = delete;
		FractureStaticMesh& operator=(const FractureStaticMesh&) = delete;

		static ID3D12GraphicsCommandList4* commandList_;

		// 頂点を共有する破壊グループのバッファ
		const PackedGeometryBuffer* geometryBuffer_ = nullptr;

		// GPUへ反映待ちのインデックス
		std::vector<uint32_t> pendingIndices_;
		bool isDirty_ = false;

		// 静的なチャンクの三角形だけを並べたインデックスバッファ
		std::unique_ptr<IndexBuffer> indexBuffer_;
		std::unique_ptr<BLAS> blas_;
		uint32_t indexCount_ = 0;

		// シェーダー側から頂点、インデックス、マテリアルを引くための参照
		RefBuffer refBuffer_;
	};
}

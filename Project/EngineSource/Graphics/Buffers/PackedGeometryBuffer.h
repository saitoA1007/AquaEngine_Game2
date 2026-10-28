#pragma once
#include <cassert>
#include <unordered_map>
#include <vector>
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexData.h"
#include "BLAS.h"
#include "RefBuffer.h"

namespace GameEngine {

	/// <summary>
	/// 破壊オブジェクト1つ分の全チャンクを、1本の共有頂点、インデックスバッファにまとめて保持する
	/// </summary>
	class PackedGeometryBuffer final {
	public:
		PackedGeometryBuffer() = default;
		~PackedGeometryBuffer() = default;

		/// <summary>
		/// 同じ破壊グループに属するチャンク群を1本のバッファに詰め込む
		/// </summary>
		/// <param name="chunkMeshes">fractureInfoを持つMeshDataの一覧</param>
		void Build(const std::vector<MeshData>& chunkMeshes) {

			std::vector<VertexData> packedVertices;
			std::vector<uint32_t> packedIndices;

			for (const auto& meshData : chunkMeshes) {
				assert(meshData.fractureInfo.has_value() && "PackedGeometryBufferにはfractureInfoを持つMeshDataのみ渡してください");

				GeometryRange range;
				range.vertexOffset = static_cast<uint32_t>(packedVertices.size());
				range.vertexCount = static_cast<uint32_t>(meshData.vertices.size());
				range.indexOffset = static_cast<uint32_t>(packedIndices.size());
				range.indexCount = static_cast<uint32_t>(meshData.indices.size());

				// インデックス値はチャンク内で完結したローカル参照
				packedVertices.insert(packedVertices.end(), meshData.vertices.begin(), meshData.vertices.end());
				packedIndices.insert(packedIndices.end(), meshData.indices.begin(), meshData.indices.end());

				rangesByChunkId_[meshData.fractureInfo->chunkId] = range;

				// 三角形ごとの接している隣接チャンクを、パック後の三角形の並びに合わせて保持する
				const auto& facingIds = meshData.fractureInfo->triangleFacingChunkIds;
				size_t triangleCount = meshData.indices.size() / 3;
				for (size_t t = 0; t < triangleCount; ++t) {
					triangleFacingChunkIds_.push_back(t < facingIds.size() ? facingIds[t] : kFractureExteriorFace);
				}
			}

			// ランタイムカット用に保持しておく
			cpuVertices_ = packedVertices;
			cpuIndices_ = packedIndices;
			
			vertexBuffer_.Create(packedVertices);
			indexBuffer_.Create(packedIndices);
		}

		// 指定チャンクの描画範囲を取得
		const GeometryRange& GetRange(uint32_t chunkId) const {
			auto it = rangesByChunkId_.find(chunkId);
			assert(it != rangesByChunkId_.end() && "指定されたchunkIdのGeometryRangeが見つかりません");
			return it->second;
		}

		// 指定チャンクの頂点を取得
		Fragment ExtractChunk(uint32_t chunkId) const {
			const GeometryRange& range = GetRange(chunkId);

			Fragment frag;
			frag.vertices.assign(
				cpuVertices_.begin() + range.vertexOffset,
				cpuVertices_.begin() + range.vertexOffset + range.vertexCount);

			frag.indices.assign(
				cpuIndices_.begin() + range.indexOffset,
				cpuIndices_.begin() + range.indexOffset + range.indexCount);
			return frag;
		}

		/// <summary>
		/// 静的なチャンクを1つのメッシュとしてまとめて描画するためのインデックスを作る。
		/// 静的なチャンクの三角形のうち、元の外側の表面か、静的でないチャンクと接している切断面だけを残す
		/// </summary>
		/// <param name="isStatic">chunkIdが静的なチャンクか</param>
		/// <returns>パックされた頂点バッファの先頭を基準にしたインデックス</returns>
		template <typename IsStaticFunc>
		std::vector<uint32_t> BuildStaticIndices(IsStaticFunc&& isStatic) const {
			std::vector<uint32_t> result;
			for (const auto& [chunkId, range] : rangesByChunkId_) {
				if (!isStatic(chunkId)) { continue; }

				uint32_t firstTriangle = range.indexOffset / 3;
				uint32_t triangleCount = range.indexCount / 3;
				for (uint32_t t = 0; t < triangleCount; ++t) {
					uint32_t facingId = triangleFacingChunkIds_[firstTriangle + t];
					if (facingId != kFractureExteriorFace && isStatic(facingId)) { continue; }

					// チャンク内のローカル参照を、パック後の頂点バッファ全体の参照に直す
					uint32_t indexBase = (firstTriangle + t) * 3;
					for (uint32_t i = 0; i < 3; ++i) {
						result.push_back(cpuIndices_[indexBase + i] + range.vertexOffset);
					}
				}
			}
			return result;
		}

		// パックされた全頂点数
		uint32_t GetTotalVertices() const { return vertexBuffer_.GetTotalVertices(); }

		// バッファは1本しかないため、ビューは全チャンクで共有する
		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBuffer_.GetView(); }
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBuffer_.GetView(); }

		uint32_t GetVertexBufferSrvIndex() const { return vertexBuffer_.GetSrvIndex(); }
		uint32_t GetIndexBufferSrvIndex() const { return indexBuffer_.GetSrvIndex(); }

		/// <summary>
		/// 全チャンク分のBLASを一括構築する
		/// </summary>
		void BuildBLAS(ID3D12GraphicsCommandList4* cmdList, uint32_t materialSrvIndex) {
			if (blasBuilt_) { return; }

			uint32_t vbSrv = GetVertexBufferSrvIndex();
			uint32_t ibSrv = GetIndexBufferSrvIndex();

			for (const auto& [chunkId, range] : rangesByChunkId_) {
				D3D12_VERTEX_BUFFER_VIEW offsetVB = MakeOffsetVBV(GetVertexBufferView(), range.vertexOffset, range.vertexCount);
				D3D12_INDEX_BUFFER_VIEW offsetIB = MakeOffsetIBV(GetIndexBufferView(), range.indexOffset, range.indexCount);

				auto blas = std::make_unique<BLAS>();
				blas->Create(cmdList, offsetVB, offsetIB, range.vertexCount, range.indexCount, false);
				chunkBLAS_[chunkId] = std::move(blas);

				// RefBufferはデストラクタでインデックスを解放するためムーブができない
				RefBuffer& refBuffer = chunkRefBuffers_[chunkId];
				refBuffer.Create();
				refBuffer.SetModelData(vbSrv, ibSrv, range.vertexOffset, range.indexOffset);
				refBuffer.SetBufferMaterial(static_cast<uint32_t>(BufferType::kDefalutMaterial), materialSrvIndex);
			}

			blasBuilt_ = true;
		}

		bool HasBLAS() const { return blasBuilt_; }

		BLAS* GetChunkBLAS(uint32_t chunkId) const {
			auto it = chunkBLAS_.find(chunkId);
			return it != chunkBLAS_.end() ? it->second.get() : nullptr;
		}

		uint32_t GetChunkRefIndex(uint32_t chunkId) const {
			auto it = chunkRefBuffers_.find(chunkId);
			assert(it != chunkRefBuffers_.end() && "指定chunkIdのRefBufferが見つかりません");
			return it->second.GetRefIndex();
		}

		// マテリアルを設定
		void SetBufferMaterial(uint32_t materialSrvIndex, uint32_t mask) {
			for (auto& [id, refBuffer] : chunkRefBuffers_) {
				refBuffer.SetBufferMaterial(0, materialSrvIndex);
				refBuffer.SetInstanceMask(mask);
			}
		}

	private:
		VertexBuffer<VertexData> vertexBuffer_;
		IndexBuffer indexBuffer_;

		// このバッファ内での描画範囲
		std::unordered_map<uint32_t, GeometryRange> rangesByChunkId_;

		// ランタイムカット用のCPU側コピー
		std::vector<VertexData> cpuVertices_;
		std::vector<uint32_t> cpuIndices_;

		// パック後の三角形ごとの、接している隣接チャンクのID。元の外側の表面ならkFractureExteriorFace
		std::vector<uint32_t> triangleFacingChunkIds_;

		std::unordered_map<uint32_t, std::unique_ptr<BLAS>> chunkBLAS_;
		std::unordered_map<uint32_t, RefBuffer> chunkRefBuffers_;
		bool blasBuilt_ = false;

	private:

		static D3D12_VERTEX_BUFFER_VIEW MakeOffsetVBV(const D3D12_VERTEX_BUFFER_VIEW& base, uint32_t vertexOffset, uint32_t vertexCount) {
			D3D12_VERTEX_BUFFER_VIEW view = base;
			view.BufferLocation = base.BufferLocation + static_cast<UINT64>(vertexOffset) * base.StrideInBytes;
			view.SizeInBytes = vertexCount * base.StrideInBytes;
			return view;
		}

		static D3D12_INDEX_BUFFER_VIEW MakeOffsetIBV(const D3D12_INDEX_BUFFER_VIEW& base, uint32_t indexOffset, uint32_t indexCount) {
			uint32_t strideBytes = (base.Format == DXGI_FORMAT_R32_UINT) ? 4 : 2;
			D3D12_INDEX_BUFFER_VIEW view = base;
			view.BufferLocation = base.BufferLocation + static_cast<UINT64>(indexOffset) * strideBytes;
			view.SizeInBytes = indexCount * strideBytes;
			return view;
		}
	};
}
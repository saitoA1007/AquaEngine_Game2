#pragma once
#include "Matrix4x4.h"
#include "Transform.h"
#include "ConstantBuffer.h"
#include "StructuredBuffer.h"
#include "PackedGeometryBuffer.h"
#include "RuntimeFractureBuffer.h"

namespace GameEngine {

	// CPU側で各破片の状態を管理
	struct FractureChunkState {
		Transform transform;
		Vector3 velocity;

		Vector3 crackVelocity;         // 位置ばねの速度
		Vector3 crackAngularVelocity;  // 回転ばねの速度
		Vector3 crackRestOffset;	   // ばねの収束目標位置

		Vector3 pivot;                 // 破片の中心のローカル座標で回転の中心
		float bottomOffset;            // 中心から破片の底までの高さ
		Vector3 angularVelocity;       // 落下中の回転速度
		bool isGroundActive;           // 地面との判定を行うか
		bool isSleeping;               // 地面の上で止まったか
	};

	struct FractureForGPU {
		Matrix4x4 world;
		Matrix4x4 worldInverseTranspose;

		uint32_t vertexOffset; // PackedGeometryBuffer内でのチャンクの頂点開始位置
		uint32_t indexOffset; // PackedGeometryBuffer内でのチャンクのインデックス開始位置
		uint32_t indexCount;
		uint32_t chunkId; // シェーダー側でgParticleを引くためのID
	};

	struct FractureIndirectCommand {
		uint32_t instanceIndex;
		D3D12_DRAW_INDEXED_ARGUMENTS drawArguments;
	};

	/// <summary>
	/// 複数描画用のワールド行列
	/// </summary>
	class FractureInstance {
	public:
		FractureInstance() = default;
		~FractureInstance();

		/// <summary>
		/// 初期化
		/// </summary>
		/// <param name="transform"></param>
		void Initialize(const std::vector<uint32_t>& chunkIds, const PackedGeometryBuffer& geometryBuffer);

		/// <summary>
		/// SRTを適応
		/// </summary>
		void Update();

	public:

		// GeometryRangeを直接渡して初期化
		void InitializeFromRanges(const std::vector<GeometryRange>& ranges);

		// 指定メッシュを衝撃点周りでランタイムカットし、その結果でこのインスタンスを構築する
		void ApplyRuntimeCut(const Fragment& source, const Vector3& impactPos, float craterRadius, int numSites,int planeCount);

		const CD3DX12_GPU_DESCRIPTOR_HANDLE& GetInstancingSrvGPU() const { return buffer_.GetSrvGpuHandle(); }

		/// <summary>
		/// 描画するモデルの数
		/// </summary>
		/// <returns></returns>
		const uint32_t GetNumInstance() { return numInstance_; }

		// 描画すべきインスタンスを1つ以上持っているか
		bool HasInstances() const { return numInstance_ > 0; }

		// インスタンスを空にする
		void Clear() { numInstance_ = 0; }

		// ラスタライズ描画で使用する親行列
		void SetParentWorldMatrix(const Matrix4x4& worldMatrix) { parentWorldMatrix_ = worldMatrix; }

		// トランスフォーム
		std::vector<FractureChunkState>& GetTransformDatas() { return transformData_; }

		// 回転の中心と、中心から底までの高さを設定する
		void SetPivot(uint32_t index, const Vector3& pivot, float bottomOffset) {
			transformData_[index].pivot = pivot;
			transformData_[index].bottomOffset = bottomOffset;
		}

		StructuredBuffer<FractureForGPU>& GetBuffer() { return buffer_; }

		ConstantBuffer<FractureIndirectCommand>& GetArgumentBuffer() { return argumentBuffer_; }

		// ランタイムカットで構築された場合のみ有効
		bool HasRuntimeGeometry() const { return runtimeBuffer_ != nullptr; }
		const D3D12_VERTEX_BUFFER_VIEW& GetRuntimeVertexBufferView() const { return runtimeBuffer_->GetVertexBufferView(); }
		const D3D12_INDEX_BUFFER_VIEW& GetRuntimeIndexBufferView() const { return runtimeBuffer_->GetIndexBufferView(); }

		// レイトレのTLASインスタンス行列に使う、チャンク単体のワールド行列
		const Matrix4x4& GetChunkWorldMatrix(uint32_t index) const { return instancingData_[index].world; }

		uint32_t GetChunkId(uint32_t index) const { return instancingData_[index].chunkId; }

	private:
		// コピー禁止
		FractureInstance(const FractureInstance&) = delete;
		FractureInstance& operator=(const FractureInstance&) = delete;

		// インスタンスが持つsrvインデックス
		uint32_t srvIndex_ = 0;

		// transformData配列数
		uint32_t numInstance_ = 0;

		// リソース
		StructuredBuffer<FractureForGPU> buffer_;
		FractureForGPU* instancingData_ = nullptr;

		std::vector<FractureChunkState> transformData_;

		// SetParentWorldMatrixで設定される、生成元オブジェクトのワールド行列
		Matrix4x4 parentWorldMatrix_ = Matrix4x4::MakeIdentity();

		// ExecuteIndirect用の間接描画引数バッファ
		ConstantBuffer<FractureIndirectCommand> argumentBuffer_;
		FractureIndirectCommand* argumentData_ = nullptr;

		// これ未満の三角形数になった破片はそれ以上分割しない
		size_t kMinTriangleCount = 12;

		// ランタイム分割
		std::unique_ptr<RuntimeFractureBuffer> runtimeBuffer_;

	private:

		void AllocateBuffers(uint32_t count);
		// 破片の状態を初期値に戻す
		void ResetState(FractureChunkState& state);
		void WriteInstance(uint32_t index, const GeometryRange& range, uint32_t chunkId);
	};
}


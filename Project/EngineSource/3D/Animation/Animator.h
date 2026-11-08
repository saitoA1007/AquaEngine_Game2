#pragma once
#include "VertexData.h"
#include "AnimationData.h"
#include <memory>
#include "VertexBuffer.h"
#include "BLAS.h"
#include "RefBuffer.h"

namespace GameEngine {

	// モデル
	class Model;
	class PSOManager;
	class DebugRenderer;

	/// <summary>
	/// アニメーションの再生クラス
	/// 骨の姿勢・行列パレット・スキニング後の頂点・BLASを個体ごとに保持するため、
	/// 同じModelを複数のAnimatorで再生してもそれぞれ独立した動きになる
	/// </summary>
	class Animator {
	public:
		Animator() = default;
		~Animator() = default;
		Animator(const Animator&) = delete;
		Animator& operator=(const Animator&) = delete;

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="commandList"></param>
		/// <param name="psoManager"></param>
		static void StaticInitialize(ID3D12GraphicsCommandList4* commandList, PSOManager* psoManager);

		/// <summary>
		/// 初期化処理
		/// </summary>
		/// <param name="model"></param>
		/// <param name="animationData"></param>
		void Initialize(Model* model, const AnimationData* animationData);

		/// <summary>
		/// 更新処理(再生を自動管理)
		/// </summary>
		void Update();

		/// <summary>
		/// 更新処理(再生を手動管理)
		/// </summary>
		/// <param name="time"></param>
		void Update(const float& time);

		/// <summary>
		/// コンピュートシェーダーを使用した更新処理(再生を自動管理)
		/// </summary>
		void ComputeUpdate();

		/// <summary>
		/// コンピュートシェーダーを使用した更新処理(再生を手動管理)
		/// </summary>
		/// <param name="time"></param>
		void ComputeUpdate(const float& time);

		/// <summary>
		///  ボーンのデバック描画
		/// </summary>
		/// <param name="debugRenderer"></param>
		/// <param name="sphereRadius"></param>
		/// <param name="color"></param>
		void DebugDraw(DebugRenderer* debugRenderer, float sphereRadius = 0.05f, const Vector4& color = {0.0f,1.0f,1.0f,1.0f});

	public:

		/// <summary>
		/// 最大再生時間を取得
		/// </summary>
		/// <returns></returns>
		float GetMaxTime() const { return animationData_->duration; }

		/// <summary>
		/// 現在の再生時間を取得
		/// </summary>
		/// <returns></returns>
		float GetTimer() const { return timer_; }

		/// <summary>
		/// ループ状態を設定
		/// </summary>
		/// <param name="isLoop"></param>
		void SetIsLoop(const bool& isLoop) { isLoop_ = isLoop; }

		/// <summary>
		/// アニメーションデータを設定
		/// </summary>
		/// <param name="animationData"></param>
		void SetAnimationData(const AnimationData* animationData) { animationData_ = animationData; }

		/// <summary>
		/// アニメーションに付属するモデルデータを設定
		/// </summary>
		/// <param name="model"></param>
		void SetModelData(Model* model);

		/// <summary>
		/// 指定したJointのワールド座標を取得
		/// </summary>
		Vector3 GetJointWorldPosition(const std::string& jointName, const Matrix4x4& worldMatrix) const;

		/// <summary>
		/// 指定したNodeのワールド座標を取得
		/// </summary>
		Vector3 GetNodeWorldPosition(const std::string& nodeName, const Matrix4x4& worldMatrix) const;

		/// <summary>
		/// 再生対象のモデルを取得
		/// </summary>
		const Model* GetModel() const { return model_; }

		/// <summary>
		/// この個体の骨行列パレットを取得
		/// </summary>
		const SkinCluster* GetSkinCluster() const { return skinCluster_.get(); }

		/// <summary>
		/// この個体のルートノードのローカル行列を取得
		/// </summary>
		const Matrix4x4& GetRootLocalMatrix() const { return rootNode_.localMatrix; }

		/// <summary>
		/// レイトレーシングで描画できる状態か
		/// </summary>
		bool IsRaytracingReady() const { return isBlasBuilt_; }

		/// <summary>
		/// この個体のBLASを取得
		/// </summary>
		BLAS* GetBLAS(uint32_t meshIndex) const { return blases_[meshIndex].get(); }

		/// <summary>
		/// この個体のスキニング後頂点バッファのSRVインデックスを取得
		/// </summary>
		uint32_t GetOutputVertexBufferSrvIndex(uint32_t meshIndex) const { return outputVertexBuffers_[meshIndex]->GetSrvIndex(); }

		/// <summary>
		/// この個体のレイトレ参照用バッファを取得
		/// </summary>
		RefBuffer* GetRefBuffer(uint32_t meshIndex) const { return refBuffers_[meshIndex].get(); }

	private:
		static ID3D12GraphicsCommandList4* commandList_;
		static ID3D12RootSignature* rootSignature_;
		static ID3D12PipelineState* pipelineState_;

		// ループの管理
		bool isLoop_ = true;

		// 再生するアニメーションのデータ
		const AnimationData* animationData_ = nullptr;

		Model* model_ = nullptr;

		// 骨の姿勢
		SkeletonData skeleton_;
		// 骨行列パレット
		std::unique_ptr<SkinCluster> skinCluster_;
		// ノードの姿勢
		Node rootNode_;

		// コンピュートシェーダーでスキニングした結果を保持する、個体ごとのリソース
		std::vector<std::unique_ptr<VertexBuffer<VertexData>>> outputVertexBuffers_;
		std::vector<std::unique_ptr<BLAS>> blases_;
		std::vector<std::unique_ptr<RefBuffer>> refBuffers_;
		bool isBlasBuilt_ = false;

		// 時間
		float timer_ = 0.0f;

	private:

		/// <summary>
		/// アニメーション用の値を取得
		/// </summary>
		/// <param name="keyframes"></param>
		/// <param name="time"></param>
		/// <returns></returns>
		static Vector3 CalculateValue(const std::vector<KeyframeVector3>& keyframes, float time);
		static Quaternion CalculateValue(const std::vector<KeyframeQuaternion>& keyframes, float time);

		/// <summary>
		/// ノードに対してアニメーションを適応する
		/// </summary>
		/// <param name="model"></param>
		/// <param name="animation"></param>
		/// <param name="animationTime"></param>
		void ApplyNodeAnimation(Node& node, const AnimationData& animation, float animationTime);

		/// <summary>
		/// ノードを更新
		/// </summary>
		/// <param name="rootNode"></param>
		void NodeHierarchyUpdate(Node& rootNode);

		/// <summary>
		/// 再帰処理用のノード更新
		/// </summary>
		/// <param name="node"></param>
		/// <param name="parentMatrix"></param>
		void NodeHierarchyUpdate(Node& node, const Matrix4x4& parentMatrix);

		/// <summary>
		/// skeletonに対してアニメーションを適応する
		/// </summary>
		/// <param name="skeleton"></param>
		/// <param name="animation"></param>
		/// <param name="animationTime"></param>
		static void ApplyAnimation(SkeletonData& skeleton, const AnimationData& animation, float animationTime);

		void SkeletonUpdate(SkeletonData& skeleton);

		void SkinClusterUpdate(SkinCluster& skinCluster, const SkeletonData& skeleton);

		// コンピュートシェーダーを更新
		void UpdateCompute();

		// コンピュートシェーダー用の個体ごとのリソースを作成
		void CreateComputeResources();

		static const Node* FindNode(const Node& node, const std::string& name);
	};
}

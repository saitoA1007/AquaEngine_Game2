#pragma once
#include "VertexData.h"
#include "AnimationData.h"

namespace GameEngine {

	// モデル
	class Model;
	class PSOManager;
	class DebugRenderer;

	class Animator {
	public:

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

	private:
		static ID3D12GraphicsCommandList4* commandList_;
		static ID3D12RootSignature* rootSignature_;
		static ID3D12PipelineState* pipelineState_;

		// ループの管理
		bool isLoop_ = true;

		// 再生するアニメーションのデータ
		const AnimationData* animationData_ = nullptr;

		// 使用するモデルデータ
		SkinCluster* skinCluster_ = nullptr;
		SkeletonData* skeleton_ = nullptr;

		Model* model_ = nullptr;

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
		/// <param name="model"></param>
		void NodeHierarchyUpdate(Model* model);

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

		static const Node* FindNode(const Node& node, const std::string& name);
	};
}

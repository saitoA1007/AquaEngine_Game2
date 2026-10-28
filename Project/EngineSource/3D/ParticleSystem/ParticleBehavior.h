#pragma once
#include <vector>
#include "ParticleData.h"
#include "Matrix4x4.h"
#include "ParticleModules.h"
#include "DebugParameter.h"
#include "ModulesControl.h"
#include "IGameObject.h"
#include "Camera.h"
#include "WorldTransforms.h"
#include "ParticleUpdateModules.h"
#include "ParticleEmitModules.h"

namespace GameEngine{

	// 前方宣言
	class TextureManager;

	/// <summary>
	/// パーティクルのシミュレーション空間
	/// </summary>
	enum class ParticleSimulationSpace {
		kLocal, // ローカル空間：発生後も常に親に追従する
		kWorld, // ワールド空間：発生時だけ親の影響を受け、その後は世界に置き去りになる
	};

	class ParticleBehavior : public IGameObject {
	public:
		ParticleBehavior(const std::string& name, uint32_t maxNum, TextureManager* textureManager, Model* model);
		~ParticleBehavior() = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		/// <summary>
		/// 経過時間を指定して更新する
		/// </summary>
		/// <param name="deltaTime">経過時間</param>
		void Update(float deltaTime);

		// 描画処理
		void Draw() override;

	public:

		/// <summary>
		/// パーティクルの生成
		/// </summary>
		/// <param name="pos"></param>
		void Emit(const Vector3& pos);

		/// <summary>
		/// 行列のデータを取得
		/// </summary>
		/// <returns></returns>
		WorldTransforms* GetWorldTransforms() const { return worldTransforms_.get(); }

		/// <summary>
		/// 現在の数
		/// </summary>
		/// <returns></returns>
		uint32_t GetCurrentNumInstance() const { return currentNumInstance_; }

		/// <summary>
		/// 発生位置を設定
		/// </summary>
		/// <param name="pos"></param>
		void SetEmitterPos(const Vector3& pos) {
			emitterPos_ = pos; 
			isSetEmitPos_ = true;
		}

		// ターゲット位置を取得
		void SetAttractionTarget(const Vector3& targetPos) {
			if (auto* attraction = modulesControl_->GetModule<AttractionModule>("Attraction")) {
				attraction->SetTargetPosition(targetPos);
			}

			if (auto* Vortex = modulesControl_->GetModule<VortexModule>("Vortex")) {
				Vortex->SetCenterPosition(targetPos);
			}
		}

		// 重力場の方向を設定
		void SetGravityFieldDirection(const Vector3& direction) {
			if (auto* gravityField = modulesControl_->GetModule<GravityFieldModule>("GravityField")) {
				gravityField->SetDirection(direction);
			}
		}

		// 発射形状の半径の範囲を設定
		void SetShapeEmit(float radius) {
			if (auto* ShapeEmit = modulesControl_->GetModule<ShapeEmitModule>("ShapeEmit")) {
				ShapeEmit->SetRadius(radius);
			}
		}

		// 最終的な大きさ
		void SetSizeOverLifeTime(float endScale) {
			if (auto* ShapeEmit = modulesControl_->GetModule<SizeOverLifeTimeModule>("SizeOverLifeTime")) {
				ShapeEmit->SetEndScale(endScale);
			}
		}

		void SetIsLoop(bool isLoop) {
			main_.isLoop = isLoop;
		}

		// パーティクルの移動方向を設定
		void SetDirection(const Vector3& direction) {
			if (auto* directionModule = modulesControl_->GetModule<DirectionEmitModule>("DirectionEmit")) {
				directionModule->SetDirection(direction);
			}
		}

		// パーティクルの生存時間を設定
		void SetLifeTime(float minLifeTime, float maxLifeTime) {
			if (auto* lifeTimeModule = modulesControl_->GetModule<LifeTimeEmitModule>("LifeTimeEmit")) {
				lifeTimeModule->SetLifeTime(minLifeTime, maxLifeTime);
			}
		}

		bool IsLoop() const { return main_.isLoop; }

		/// <summary>
		/// ループ時の連続発生を設定する
		/// </summary>
		/// <param name="isEmitting"></param>
		void SetEmitting(bool isEmitting) { isEmitting_ = isEmitting; }

		// 連続発生が有効か
		bool IsEmitting() const { return isEmitting_; }

		/// <summary>
		/// 生きているパーティクルが残っているか
		/// </summary>
		/// <returns></returns>
		bool HasAliveParticles() const;

		/// <summary>
		/// 全てのパーティクルを消去し、発生タイマーをリセットする
		/// </summary>
		void Clear();

		// 色を設定
		void SetColor(Vector4 color) {
			main_.color = color;
		}

		// サイズを設定
		void SetScale(Vector3 scale) {
			main_.scale = scale;
		}

		/// <summary>
		/// 親を設定する
		/// </summary>
		/// <param name="parentMatrix">親のワールド行列</param>
		/// <param name="space">
		/// kLocal : 発生後も常に親に追従する
		/// kWorld : 発生時の親の姿勢だけを反映し、その後は追従しない
		/// </param>
		void SetParent(const Matrix4x4* parentMatrix, ParticleSimulationSpace space = ParticleSimulationSpace::kLocal) {
			parentMatrix_ = parentMatrix;
			simulationSpace_ = space;
		}

		/// <summary>
		/// 親子付けを解除する
		/// </summary>
		/// <param name="keepWorldPosition">
		/// true にすると、現在生きているパーティクルを解除時のワールド座標に焼き込んでその場に残す
		/// </param>
		void ClearParent(bool keepWorldPosition = true);

		/// <summary>
		/// シミュレーション空間を切り替える
		/// </summary>
		void SetSimulationSpace(ParticleSimulationSpace space) { simulationSpace_ = space; }

		// 親の行列を取得
		const Matrix4x4* GetParent() const { return parentMatrix_; }

		// 親が設定されているか
		bool HasParent() const { return parentMatrix_ != nullptr; }
		// シミュレーション空間を取得
		ParticleSimulationSpace GetSimulationSpace() const { return simulationSpace_; }

		// 発生位置の履歴を削除
		void ResetEmitHistory() { 
			hasPrevEmitPos_ = false;
			distanceAccum_ = 0.0f;
		}

	private:
		// パラメータ機能
		std::unique_ptr<DebugParameter> debugParame_;

		// 描画用のトランスフォーム
		std::unique_ptr<WorldTransforms> worldTransforms_;

		// トレイル描画用のトランスフォーム
		std::unique_ptr<WorldTransforms> trailTransforms_;
		// 現在のトレイルの描画数
		uint32_t trailNumInstance_ = 0;

		// モジュールの管理
		std::unique_ptr<ModulesControl> modulesControl_;

		// カメラ
		Camera* camera_ = nullptr;

		// モデル
		Model* model_ = nullptr;

		// パーティクルの配列
		std::vector<ParticleData> particles_;           
		uint32_t activeCount_ = 0;
		// 最大パーティクル数
		uint32_t maxNumInstance_ = 0;                 
		// 現在のパーティクルの数
		uint32_t currentNumInstance_ = 0;

		// 発生位置
		Vector3 emitterPos_ = { 0.0f,0.0f,0.0f };

		// 前フレームのエミッターのワールド座標
		Vector3 prevEmitWorldPos_ = {}; 
		bool hasPrevEmitPos_ = false;
		// 次の発生までに溜まっている移動距離
		float distanceAccum_ = 0.0f;     

		// ループ時に連続発生させるか
		bool isEmitting_ = true;

		bool isSetEmitPos_ = false;

		// 発生する時間
		float spawnTimer_ = 0.0f;

		// パーティクルの名前
		std::string name_;

		// メインモジュール
		MainModule main_;

		// 親行列
		const Matrix4x4* parentMatrix_ = nullptr;

		// シミュレーション空間
		ParticleSimulationSpace simulationSpace_ = ParticleSimulationSpace::kLocal;

	private:

		/// <summary>
		/// パーティクルを生成する
		/// </summary>
		/// <returns></returns>
		ParticleData MakeNewParticle(const Vector3& offset = Vector3{});
		Vector3 GetEmitterWorldPos() const;
		void UpdateDistanceEmit(bool canSpawn);
		uint32_t SpawnParticles(uint32_t count, const Vector3& offset);

		/// <summary>
		/// パーティクルの発生管理
		/// </summary>
		void Create(float deltaTime);

		/// <summary>
		/// 移動処理
		/// </summary>
		void Move(const Matrix4x4& cameraMatrix, float deltaTime);

		/// <summary>
		/// トレイルの描画データを追加する
		/// </summary>
		void AddTrail(const ParticleData& particle, const TrailModule& trailModule, const Matrix4x4& cameraMatrix, bool isRotateVelocity, bool isFollowParent);

		/// <summary>
		/// 常に親に追従するか
		/// </summary>
		bool IsFollowParent() const {
			return parentMatrix_ != nullptr && simulationSpace_ == ParticleSimulationSpace::kLocal;
		}
	};
}


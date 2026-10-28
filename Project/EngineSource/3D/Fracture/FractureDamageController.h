#pragma once
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>
#include "Model.h"
#include "FractureBreakState.h"
#include "FractureStaticMesh.h"

namespace GameEngine {

	/// <summary>
	/// 破壊オブジェクトの状態を管理する
	/// </summary>
	class FractureDamageController {
	public:
		// モデルの破壊チャンク情報から初期化
		void Initialize(Model* model);

		// 落下シミュレーションとひび割れバネ物理を更新
		void Update(float deltaTime);

		// 指定位置にダメージを蓄積させる
		void ApplyChipDamage(const Vector3& impactPos, float damageAmount, float craterRadius, int craterPlaneCount,
			const Vector3& impactDirection, float penetrationDepth);

		// 静的な破片を元の位置へ戻し、無傷の状態へ戻すアニメーションを開始する。
		void BeginReassembly();
		bool IsReassembling() const { return isReassembling_; }

		// チャンクの破壊された割合を取得
		float GetDestroyedRatio() const {
			if (chunksById_.empty()) { return 0.0f; }
			return static_cast<float>(destroyedChunkIds_.size()) / static_cast<float>(chunksById_.size());
		}

		void SetWorldMatrix(const Matrix4x4& worldMatrix) { worldMatrix_ = worldMatrix; }

		FractureBreakState& GetBreakState() { return breakState_; }
		const FractureBreakState& GetBreakState() const { return breakState_; }

		// 静的なチャンクを1つにまとめたメッシュ
		const FractureStaticMesh& GetStaticMesh() const { return staticMesh_; }
		// 静的なメッシュのレイトレで使用するマテリアルを設定
		void SetStaticMeshMaterial(uint32_t materialSrvIndex, uint32_t instanceMask) { staticMesh_.SetMaterial(materialSrvIndex, instanceMask); }
		// 静的なメッシュの変更をGPU側へ反映する。描画の登録前に呼ぶ
		void FlushStaticMesh() { staticMesh_.FlushIfDirty(); }

	public:

		float kBreakThreshold_ = 3.0f;
		// チャンクが完全破壊された時、命中点から半径このくらいの範囲にある隣接チャンクも巻き込んで切り離す
		float kBreakDetachRadius_ = 3.0f;
		// 閾値到達寸前の最大ズレ量
		float kMaxCrackOffset_ = 0.04f;
		// 閾値到達寸前の最大ランダム回転
		float kMaxCrackRotate_ = 0.15f;
		// 隣接チャンクへ波及させる強さ
		float kNeighborCrackFactor_ = 0.35f;

		// ばね物理のパラメータ
		float kCrackSpringStiffness_ = 400.0f;         // バネ定数
		float kCrackDamping_ = 30.0f;                  // 減衰係数
		float kCrackAngularSpringStiffness_ = 250.0f;
		float kCrackAngularDamping_ = 15.0f;
		float kCrackImpulseStrength_ = 1.8f;

		// ランタイムカットの球の中心を、衝突方向と逆側へ半径のこの割合だけ押し込む。
		float kDentInwardBiasRatio_ = 0.5f;

		// 付着したまま凹ませる際、初回ヒットから最低このくらいの大きさで凹ませる
		float kMinDentRatio_ = 0.15f;

		// 付着したまま凹ませる球の半径は、チャンク自身のAABB対角線半分に対してこの割合を超えないようクランプする。
		float kMaxDentRadiusToChunkRatio_ = 0.6f;

		// 元に戻る演出で使うバネ物理のパラメータ。
		float kReassemblySpringStiffness_ = 8.0f;
		float kReassemblyDamping_ = 6.0f;
		float kReassemblyAngularSpringStiffness_ = 8.0f;
		float kReassemblyAngularDamping_ = 6.0f;
		// これ未満なら元の位置・回転へ収束したとみなす
		float kReassemblySettleThreshold_ = 0.02f;

	private:
		// モデル
		Model* model_ = nullptr;

		// このオブジェクトが持つ破壊グループ名
		std::string groupName_;
		// chunkIdから素早く引くためのマップ
		std::unordered_map<uint32_t, const FractureChunkEntry*> chunksById_;

		// 破片の状態
		FractureBreakState breakState_;

		// 静的なチャンクを内部の切断面を除いて1つにまとめたメッシュ
		FractureStaticMesh staticMesh_;

		// ひびで揺れている、ずれているため、静的なメッシュから外してbreakState_.Intact()で個別に描画しているチャンクのID
		std::unordered_set<uint32_t> looseChunkIds_;

		// マクロ・マイクロ破片それぞれについて、保持しておくバッチ数の上限
		size_t kMaxDebrisBatchesPerType_ = 16;

		// 既に切り離し済みのチャンクID
		std::unordered_set<uint32_t> destroyedChunkIds_;

		// チャンクごとの蓄積ダメージ
		std::unordered_map<uint32_t, float> chunkDamage_;

		// chunkIdがbreakState_.Intact()の何番目のインスタンスか。looseChunkIds_のチャンクだけを持つ
		std::unordered_map<uint32_t, uint32_t> chunkIndexInIntact_;

		// モデル全体のおおよその中心。ひびを入れるのに使用
		Vector3 modelCenter_ = { 0.0f, 0.0f, 0.0f };

		// 現在バネが揺れているチャンクのID
		std::unordered_set<uint32_t> crackActiveChunkIds_;

		// 元の姿へ戻るアニメーションを再生中か
		bool isReassembling_ = false;

		// 生成元オブジェクトの現在のワールド行列
		Matrix4x4 worldMatrix_ = Matrix4x4::MakeIdentity();

	private:
		// チャンクを切り離して破片化する
		void ApplyDamage(const Vector3& impactPos, float damageRadius, float craterRadius, int craterPlaneCount,
			const Vector3& impactDirection, float penetrationDepth);

		// まだ切り離すほどではないチャンクを、付着させたまま動的にカットして凹ませる
		void CarveAttachedDent(uint32_t chunkId, const Vector3& impactPos, float damageRatio,
			float craterRadius, int craterPlaneCount, const Vector3& impactDirection, float penetrationDepth);

		// 衝突方向・めり込み量から、ランタイムカットする球の中心位置を計算する
		Vector3 ComputeDentCenter(const Vector3& impactPos, const Vector3& impactDirection, float dentRadius) const;

		// 個別に描画するチャンクのインスタンスと、静的なメッシュを作り直す
		void RebuildStaticState();

		// 壊れておらず、凹んでもおらず、個別に描画もしていない、静的なメッシュに含めるチャンクか
		bool IsStaticChunk(uint32_t chunkId) const;

		// 隣接グラフをフラッドフィルして、切り離すチャンク群を選ぶ
		std::vector<uint32_t> SelectDetachedChunks(uint32_t seedChunkId, float damageRadius, const Vector3& impactPos) const;

		// アンカーから隣接グラフを辿れず、支えを失ったチャンクを集めて崩落させる
		void CollapseUnsupportedChunks();

		// impactPosに一番近いまだ壊れていないチャンクを探す
		std::optional<uint32_t> FindNearestChunk(const Vector3& impactPos) const;

		// 破片に落下運動を与える
		void SimulateFallingDebris(FractureInstance& instance, float deltaTime);

		// 破片の爆発
		void ApplyExplosionImpulse(FractureInstance& instance,
			const std::vector<uint32_t>& chunkIds, const Vector3& impactPos, float damageRadius);

		void ApplyExplosionImpulseUniform(FractureInstance& instance,
			const Vector3& impactPos, float strength);

		// 崩落する破片に、爆発ではなく重力で崩れ落ちるような弱い初速を与える
		void ApplyCollapseImpulse(FractureInstance& instance);

		void UpdateCrackVisual(uint32_t chunkId, float ratio, float damageDelta);
		void RebuildIntactIndexMap(const std::vector<uint32_t>& ids);

		void SimulateCrackPhysics();

		// 再構築中のマクロ破片を全て原点へバネで収束させ、全チャンクが収束済みなら無傷状態へ復元する
		void UpdateReassembly(float deltaTime);

		// instance内の全チャンクの位置、回転を原点へバネで収束させる。全チャンクが収束済みならtrueを返す
		bool SimulateReassemblySpring(FractureInstance& instance, float deltaTime);
	};
}

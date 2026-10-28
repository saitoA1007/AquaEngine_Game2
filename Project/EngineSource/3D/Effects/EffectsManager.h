#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "IGameObject.h"
#include "EffectObject.h"

namespace GameEngine {

	/// <summary>
	/// エフェクトの定義データと再生インスタンスを管理するクラス
	/// </summary>
	class EffectsManager : public IGameObject {
	public:
		EffectsManager(TextureManager* textureManager, ModelManager* modelManager);
		~EffectsManager() override = default;

		// 初期化処理
		void Initialize() override;

		// 更新処理
		void Update() override;

		// 描画処理
		void Draw() override;

	public:

		/// <summary>
		/// kDirectoryPath内のエフェクトを全て読み込む
		/// </summary>
		void LoadAll();

		/// <summary>
		/// エフェクトを登録する。同じ名前があれば上書きする
		/// </summary>
		/// <param name="asset">登録するエフェクト</param>
		void RegisterEffect(const EffectAsset& asset);

		/// <summary>
		/// 登録したエフェクトのデータからインスタンスを作成する
		/// </summary>
		/// <param name="name">エフェクトの名前</param>
		/// <returns>作成したインスタンス。エフェクトが見つからなければnullptr</returns>
		std::unique_ptr<EffectObject> GetEffect(const std::string& name) const;

		/// <summary>
		/// 名前からエフェクトの定義データを取得
		/// </summary>
		/// <param name="name">エフェクトの名前</param>
		/// <returns>見つからなければnullptr</returns>
		const EffectAsset* GetAsset(const std::string& name) const;

		/// <summary>
		/// エフェクトを再生する
		/// </summary>
		/// <param name="name">エフェクトの名前</param>
		/// <param name="pos">再生する位置</param>
		/// <returns>再生したインスタンス。エフェクトが見つからなければnullptr</returns>
		EffectObject* Play(const std::string& name, const Vector3& pos);

		/// <summary>
		/// 全てのエフェクトを即座に停止する
		/// </summary>
		void StopAll();

		// 登録されている全ての定義データ
		const std::unordered_map<std::string, EffectAsset>& GetAssets() const { return assets_; }

	private:
		TextureManager* textureManager_ = nullptr;
		ModelManager* modelManager_ = nullptr;

		// エフェクトの定義データ
		std::unordered_map<std::string, EffectAsset> assets_;

		// エフェクトごとの再生インスタンス
		std::unordered_map<std::string, std::vector<std::unique_ptr<EffectObject>>> instances_;

		// 古い定義で作られたインスタンス
		std::vector<std::unique_ptr<EffectObject>> retiredInstances_;
	};
}

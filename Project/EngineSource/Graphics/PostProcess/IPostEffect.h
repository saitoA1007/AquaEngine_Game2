#pragma once
#include <string>
#include "SrvManager.h"
#include "ConstantBuffer.h"
#include "JsonSerializer.h"

namespace GameEngine {

	class IPostEffect {
	public:
		virtual ~IPostEffect() = default;

		// 更新処理
		virtual void Update() {}

		// 描画処理
		virtual void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) = 0;

	public:

		// ポストエフェクトを掛ける描画パスを取得
		virtual void SetPassIndex(const uint32_t& index) = 0;

		// 登録
		void Register(const uint32_t& passIndex, const std::string& psoName) {
			passIndex_ = passIndex;
			psoName_ = psoName;
		}

		// 使用する描画パス
		void SetPersistentPass(const std::string& passName) { persistentPass_ = passName; }
		const std::string& GetPersistentPass() const { return persistentPass_; }
		bool IsPersistent() const { return !persistentPass_.empty(); }

		// グラフ用
		// 入力ピンの数
		virtual uint32_t GetInputCount() const { return 1; }
		// 入力ピンの名前
		virtual const char* GetInputName(uint32_t slot) const { return "Input"; }
		// スロット指定で入力SRVを設定。slot0は既存のSetPassIndexに流す
		virtual void SetInputIndex(uint32_t slot, uint32_t srvIndex) {
			if (slot == 0) { SetPassIndex(srvIndex); }
		}
		// 無効時にどの入力スロットを素通しするか
		virtual uint32_t GetPassThroughSlot() const { return 0; }
		// ノードのタイトル
		virtual const char* GetDisplayName() const { return psoName_.c_str(); }
		// ノード内に出すパラメータUI
		virtual void DrawParamUI() {}

		// パラメータの保存/読み込み
		virtual void SaveParams(nlohmann::json& j) {}
		virtual void LoadParams(const nlohmann::json& j) {}

		// 使用するpassのインデックス
		const uint32_t& GetPassIndex() const { return passIndex_; }
		// 使用するPSOの名前
		const std::string& GetPsoName() const { return psoName_; }
		// 有効状態
		void SetIsActive(const bool& isActive) { isActive_ = isActive; }
		const bool& IsActive()const { return isActive_; }
	protected:
		// 使用するpass名前
		uint32_t passIndex_ = 0;
		// 使用するpso
		std::string psoName_;
		// 使用する描画パス
		std::string persistentPass_ = "";

		// 有効状態
		bool isActive_ = false;
	};
}


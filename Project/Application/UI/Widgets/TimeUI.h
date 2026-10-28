#pragma once
#include <array>
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Sprite.h"

namespace GameEngine {
	// 前方宣言
	class TextureManager;
}

class TimeUI : public GameEngine::IGameObject {
public:

	// 数の構造体
	struct NumInfo {
		std::unique_ptr<GameEngine::Sprite> sprite_;
		int32_t num_ = 0;
	};

public:
	TimeUI(std::string name, GameEngine::TextureManager* textureManager);
	~TimeUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 時間を設定
	void SetTime(float time) {
		time_ = time;
		CalculateTotalCount();
	}

	// 基準点に親を設定
	void SetParent(GameEngine::WorldTransform* parent) {
		baseWorld_.SetParent(parent);
	}

	// 色の設定
	void SetColor(Vector3 color) {
		for (size_t i = 0; i < numSprite_.size(); ++i) {
			numSprite_[i].sprite_->color_.x = color.x;
			numSprite_[i].sprite_->color_.y = color.y;
			numSprite_[i].sprite_->color_.z = color.z;
		}
		dotSprite_->color_.x = color.x;
		dotSprite_->color_.y = color.y;
		dotSprite_->color_.z = color.z;
	}

	// 色の設定
	void SetColor(Vector4 color) {
		for (size_t i = 0; i < numSprite_.size(); ++i) {
			numSprite_[i].sprite_->color_ = color;
			dotSprite_->color_ = color;
		}
	}

	// 透明度の設定
	void SetAlpha(float alpha) {
		for (size_t i = 0; i < numSprite_.size(); ++i) {
			numSprite_[i].sprite_->color_.w = alpha;
			dotSprite_->color_.w = alpha;
		}
	}

private:

	// 使用テクスチャ
	const std::array<std::string, 10> numTexture_ = {
		"0.png",
		"1.png",
		"2.png",
		"3.png",
		"4.png",
		"5.png",
		"6.png",
		"7.png",
		"8.png",
		"9.png",
	};

private:

	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 基準の位置
	GameEngine::WorldTransform baseWorld_;

	// テクスチャハンドル
	uint32_t numGH_[10];

	// 画像
	std::array<NumInfo, 4> numSprite_;

	// 中間の点画像
	std::unique_ptr<GameEngine::Sprite> dotSprite_;

	// 時間
	float time_ = 0.0f;

private:

	// 数を適応する
	void CalculateTotalCount();
};
#pragma once
#include "IGameObject.h"
#include "Sprite.h"
#include "DebugParameter.h"

class DimmerUI : public GameEngine::IGameObject {
public:
	DimmerUI(std::string name, GameEngine::DebugParameter* debugParame, bool isFrameActive = false);
	~DimmerUI() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 基準点に親を設定
	void SetParent(GameEngine::WorldTransform* parent) {
		world_.SetParent(parent);
	}

	// 色を設定
	void SetColor(Vector3 color) {
		sprite_.color_.x = color.x;
		sprite_.color_.y = color.y;
		sprite_.color_.z = color.z;
	}
	void SetColor(Vector4 color) {
		sprite_.color_ = color;
	}

	void SetAlpha(float alpha) {
		sprite_.color_.w = alpha;
	}

	// テクスチャを設定
	void SetTexture(uint32_t texture) {
		sprite_.textureHandle_ = texture;
	}

	// アニメーションさせる
	void Play() {
		if (!isPlay_) {
			isPlay_ = true;
		}
	}

public:
	// 基準
	GameEngine::WorldTransform world_;

private:

	// 時間
	float maxTime_ = 1.0f;

	// 動く倍率
	float scaleRatio_ = 0.75f;

private:

	// 名前
	std::string name_;

	bool isPlay_ = false;

	bool isFrameActive_ = false;

	float timer_ = 0.0f;

	Vector3 startScale_;
	Vector3 endScale_;

	// 画像
	GameEngine::Sprite sprite_;
	
	// 背景のフレーム
	std::unique_ptr<GameEngine::Sprite> frameSprite_;
};
#include "pch.h"
#include "HpContainer.h"

using namespace GameEngine;

HpContainer::HpContainer(std::string name, uint32_t texture) {

	// テクスチャを取得
	texture_ = texture;

	// パラメータ機能
	debugParame_ = std::make_unique<DebugParameter>(name);
	debugParame_->RegisterWorld("Base", baseWorld_);
	debugParame_->Register("Spacing", spacing_);
	debugParame_->Register("IconSize", iconSize_);

	// 作成
	hpSprites_.resize(maxHp_);
	for (int i = 0; i < maxHp_; ++i) {
		hpSprites_[i] = std::make_unique<Sprite>();
		hpSprites_[i]->SetParent(&baseWorld_);
		hpSprites_[i]->position_.x = i * (spacing_ + iconSize_.x);
		hpSprites_[i]->textureHandle_ = texture_;
	}
}

void HpContainer::Initialize() {
	debugParame_->Apply();
}

void HpContainer::Update() {
	debugParame_->ApplyIfDirty();

	for (int32_t i = 0; i < maxHp_; ++i) {
		auto& sprite = hpSprites_[static_cast<size_t>(i)];
		if (i < currentHp_) {
			// 残りhpは白
			sprite->color_ = { 1.0f,1.0f,1.0f,1.0f };
		} else {
			// 減っている分は黒
			sprite->color_ = { 0.0f,0.0f,0.0f,1.0f };
		}

		sprite->position_.x = i * (spacing_ + iconSize_.x);
		sprite->size_ = iconSize_;

		// スプライトの更新処理
		sprite->Update();
	}

	// 更新
	baseWorld_.UpdateTransformMatrix();
}

void HpContainer::Draw() {
	// 描画
	for (auto& sprite : hpSprites_) {
		renderQueue_->SubmitSprite(sprite.get());
	}
}

void HpContainer::SetMaxHp(int32_t hp) {
	if (hpSprites_.size() == hp) { return; }

	int32_t currentNum = static_cast<int32_t>(hpSprites_.size());
	maxHp_ = hp;

	hpSprites_.resize(maxHp_);

	for (int32_t i = currentNum; i < maxHp_; ++i) {
		hpSprites_[i] = std::make_unique<Sprite>();
		hpSprites_[i]->SetParent(&baseWorld_);
		hpSprites_[i]->position_.x = i * (spacing_ + iconSize_.x);
		hpSprites_[i]->textureHandle_ = texture_;
	}
}
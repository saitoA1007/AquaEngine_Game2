#include "pch.h"
#include"Fade.h"
#include"EasingManager.h"
#include"SpriteRenderer.h"
using namespace GameEngine;

void Fade::Initialize() {
	// 初期化
	//sprite_ = Sprite::Create({ 0.0f,0.0f }, { 1280.0f,720.0f }, { 0.0f,0.0f }, { 0.0f,0.0f,0.0f,0.0f });
}

void Fade::Update(float timer) {

	//if (timer <= 0.5f) {
	//	float localT = timer / 0.5f;
	//	sprite_->color_.w = Lerp(0.0f, 1.0f, localT);
	//} else {
	//	float localT = (timer - 0.5f) / 0.5f;
	//	sprite_->color_.w = Lerp(1.0f, 0.0f, localT);
	//}

	// 更新処理
	//sprite_->Update();
}

void Fade::Draw() {
	// 画像の描画前処理
	//SpriteRenderer::PreDraw(RenderMode2D::Normal);

	// 遷移画像を表示
	//SpriteRenderer::Draw(sprite_.get(), 0);
}

bool Fade::IsMidTransition(float timer) const {
	return timer >= 0.25f;
}
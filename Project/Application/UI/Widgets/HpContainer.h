#pragma once
#include <vector>
#include "IGameObject.h"
#include "Sprite.h"
#include "WorldTransform.h"
#include "DebugParameter.h"

/// <summary>
/// 個数タイプのHP表示
/// </summary>
class HpContainer : public GameEngine::IGameObject {
public:
	HpContainer(std::string name, uint32_t texture);
	~HpContainer() = default;

	// 初期化処理
	void Initialize() override;

	// 更新処理
	void Update() override;

	// 描画処理
	void Draw() override;

public:

	// 現在のhpを設定
	void SetCurrentHp(int32_t hp) { currentHp_ = hp; }

	// 最大Hpを設定
	void SetMaxHp(int32_t hp);

	// 基準点に親を設定
	void SetParent(GameEngine::WorldTransform* parent) {
		baseWorld_.SetParent(parent);
	}
	
private:
	// パラメータ機能
	std::unique_ptr<GameEngine::DebugParameter> debugParame_;

	// 基準の位置
	GameEngine::WorldTransform baseWorld_;

	// hp画像
	std::vector<std::unique_ptr<GameEngine::Sprite>> hpSprites_;

	int32_t maxHp_ = 3;
	int32_t currentHp_ = 3;

	// 間隔
	float spacing_ = 4.0f;

	// 画像
	uint32_t texture_ = 0;

	// 大きさ
	Vector2 iconSize_ = { 64.0f,64.0f };
};
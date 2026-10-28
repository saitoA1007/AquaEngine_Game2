#pragma once
#include"Sprite.h"
#include"ITransitionEffect.h"

class Fade : public ITransitionEffect {
public:

    // 初期化処理
    void Initialize() override;

    // 更新処理
    void Update(float timer) override;

    // 描画処理
    void Draw() override;

    // 中間を追加
    bool IsMidTransition(float timer) const override;

    // 遷移する時間
    float GetMaxTime() override { return 0.5f; }

private:
    // 画像
    std::unique_ptr<GameEngine::Sprite> sprite_;

};
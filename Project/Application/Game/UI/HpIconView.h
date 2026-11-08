#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "RenderQueue.h"
#include "Sprite.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector4.h"

/// <summary>
/// HPアイコンの並びと被弾の演出。残りのアイコンは水色、失ったアイコンは暗い色にする。
/// 失ったアイコンは一瞬大きく弾けてから暗くなる。演出は止まらない時間で進める。見た目は仮（丸の画像を塗る）。
/// </summary>
class HpIconView {
public:
    /// <summary>
    /// 調整値を登録する。
    /// </summary>
    /// <param name="textureHandle">アイコンの画像のハンドル（仮素材。正式な画像へ差し替える）</param>
    explicit HpIconView(uint32_t textureHandle);
    ~HpIconView() = default;

    /// <summary>
    /// 表示するHPを設定する。PlayDamageの直後なら、減った分のアイコンの演出を始める。毎フレーム呼ぶ。
    /// </summary>
    /// <param name="hp">現在のHP</param>
    /// <param name="maxHp">最大HP</param>
    void SetHp(int32_t hp, int32_t maxHp);

    /// <summary>
    /// 被弾したことを伝える。次のSetHpで、減ったアイコンが弾ける。
    /// </summary>
    void PlayDamage() { isDamagePending_ = true; }

    /// <summary>
    /// 演出の時間を進める。
    /// </summary>
    /// <param name="deltaTime">止まらない時間の経過（秒）</param>
    void Update(float deltaTime);

    /// <summary>
    /// アイコンの位置・大きさ・色を決めて、描画キューへ登録する。
    /// </summary>
    /// <param name="renderQueue">登録先。呼び出し元が所有する。</param>
    void Draw(GameEngine::RenderQueue* renderQueue);

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// アイコンの数を最大HPに合わせる。
    /// </summary>
    /// <param name="count">アイコンの数</param>
    void ResizeIcons(int32_t count);

    /// <summary>
    /// 1つのアイコンの大きさの倍率と色を、状態と演出の進み具合から求める。
    /// </summary>
    /// <param name="index">アイコンの番号</param>
    /// <param name="scale">大きさの倍率の出力</param>
    /// <param name="color">色の出力</param>
    void GetIconLook(int32_t index, float& scale, Vector4& color) const;

private:
    // 調整値
    Vector2 origin_ = { 40.0f, 40.0f };                 // 先頭のアイコンの左上の位置（画面座標）
    float iconSize_ = 56.0f;                            // アイコン1つの大きさ（px）
    float spacing_ = 8.0f;                              // アイコンの間隔（px）
    Vector4 aliveColor_ = { 0.4f, 0.8f, 1.0f, 1.0f };   // 残りのアイコンの色
    Vector4 lostColor_ = { 0.1f, 0.12f, 0.16f, 0.9f };  // 失ったアイコンの色
    Vector4 flashColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };   // 弾ける瞬間の色
    float popScale_ = 1.9f;                             // 弾ける最大の大きさの倍率
    float popTime_ = 0.4f;                              // 弾けてから暗くなるまでの時間（秒）
    float popPeakRatio_ = 0.25f;                        // 演出のうち最大の大きさになる位置（0〜1）

    // 定数
    inline static const char* kDebugName = "HpIconView";

    // 状態
    int32_t hp_ = 0;                  // 表示するHP
    int32_t shownHp_ = -1;            // 前回のSetHpで受け取ったHP（-1は未設定）
    bool isDamagePending_ = false;    // 被弾の通知を受け、次のSetHpで演出を始めるか
    int32_t popFrom_ = 0;             // 演出中のアイコンの範囲の始まり（含む）
    int32_t popTo_ = 0;               // 演出中のアイコンの範囲の終わり（含まない）
    float popTimer_ = 0.0f;           // 演出の経過時間（秒）。popTime_以上なら演出なし
    uint32_t textureHandle_ = 0;      // アイコンの画像

    // 描画
    std::vector<std::unique_ptr<GameEngine::Sprite>> icons_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

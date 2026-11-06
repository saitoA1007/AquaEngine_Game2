#pragma once
#include <algorithm>
#include "Vector3.h"
#include "Application/Game/Enemy/EnemyDefinition.h"

/// <summary>
/// 敵の状態。
/// </summary>
enum class EnemyState {
    kNormal, // 通常（動く）
    kFrozen, // 凍結（完全に停止し、壁になる）
};

/// <summary>
/// 個体の更新に必要な、フレームごとの値。EnemyManagerが作る。
/// </summary>
struct EnemyContext {
    float deltaTime = 0.0f;                          // 経過時間（秒）
    Vector3 playerPosition = { 0.0f, 0.0f, 0.0f };   // プレイヤーの位置
    float iceAreaRadius = 0.0f;                      // 氷エリアの半径
    float freezeRate = 1.0f;                         // 氷エリア内で氷ゲージが進む速さ（ゲージ/秒）
    float decayRate = 0.5f;                          // 氷エリア外で氷ゲージが減る速さ（ゲージ/秒）
    float frozenDuration = 10.0f;                    // 凍ってから解放までの時間（秒）
};

/// <summary>
/// 敵の基底クラス。共通のデータと凍結の進行を持ち、行動は派生クラスが決める。
/// IGameObjectではなく、EnemyManagerが配列で持つ。
/// </summary>
class Enemy {
public:
    virtual ~Enemy() = default;

    /// <summary>
    /// 個体を使い始める。プールから取り出したときに呼ぶ。
    /// </summary>
    /// <param name="definition">この個体の種類。EnemyManagerが所有し、個体より長く生存する。</param>
    /// <param name="position">出現位置（XZ平面。Yは使わない）</param>
    void Spawn(const EnemyDefinition* definition, const Vector3& position);

    /// <summary>
    /// 1フレーム分更新する。凍っていれば解放までの時間だけ進め、凍っていなければ氷ゲージと行動を更新する。
    /// </summary>
    /// <param name="context">フレームごとの値</param>
    void Update(const EnemyContext& context);

    /// <summary>
    /// 位置を押し出し量だけ動かす。壁との重なりの解消に使う。
    /// </summary>
    void AddPosition(const Vector3& offset) {
        position_.x += offset.x;
        position_.z += offset.z;
    }

    /// <summary>
    /// 位置を設定する。
    /// </summary>
    void SetPosition(const Vector3& position) {
        position_.x = position.x;
        position_.z = position.z;
    }

    /// <summary>
    /// 氷霧による氷ダメージを与える。凍っていれば何もしない。
    /// 閾値に達して凍ったときは、その氷霧のIDを覚える（同じ氷霧では砕かないための判定に使う）。
    /// </summary>
    /// <param name="amount">氷ゲージへ足す量</param>
    /// <param name="mistId">氷霧のID</param>
    /// <param name="frozenDuration">凍ってから解放までの時間（秒）</param>
    void ApplyIceDamage(float amount, uint32_t mistId, float frozenDuration);

    /// <summary>
    /// 氷霧で砕けるかを返す。凍っていて、最後に凍った氷霧と別の氷霧のときだけtrue。
    /// </summary>
    /// <param name="mistId">当たった氷霧のID</param>
    bool CanShatter(uint32_t mistId) const { return state_ == EnemyState::kFrozen && lastMistId_ != mistId; }

    /// <summary>
    /// 氷を砕く。砕かれた回数が必要数に達したら撃破として使用を終え、未満なら氷だけ砕いて解放する。
    /// 解放した場合は解放のイベントが残り、EnemyManagerが氷の破片を飛ばす。
    /// </summary>
    /// <returns>撃破したらtrue</returns>
    bool Shatter();

    /// <summary>
    /// 凍結の残り時間を取得する（秒。凍っていなければ0）。解放前の溶ける表現に使う。
    /// </summary>
    float GetFrozenTimeRemaining() const { return state_ == EnemyState::kFrozen ? frozenTimer_ : 0.0f; }

    /// <summary>
    /// 氷霧に砕かれて解放されたイベントを取り出す。取り出すと消える。自然解放では立たない。
    /// </summary>
    /// <returns>前回の取り出し以降に解放されていたらtrue</returns>
    bool ConsumeReleasedEvent() {
        bool released = justReleased_;
        justReleased_ = false;
        return released;
    }

    const Vector3& GetPosition() const { return position_; }
    float GetRadius() const { return definition_->radius; }
    float GetVisualScale() const { return definition_->visualScale; }
    const EnemyDefinition* GetDefinition() const { return definition_; }
    bool IsActive() const { return isActive_; }
    bool IsFrozen() const { return state_ == EnemyState::kFrozen; }

    /// <summary>
    /// 氷ゲージの割合（0から1。閾値が0以下の種類は1）。表示に使う。
    /// </summary>
    float GetIceRatio() const {
        if (definition_->iceThreshold <= 0.0f) {
            return 1.0f;
        }
        return (std::min)(iceGauge_ / definition_->iceThreshold, 1.0f);
    }

    /// <summary>
    /// 直近の移動の向き（長さ1のXZ方向。動いていなければ0）。壁を避ける動きに使う。
    /// </summary>
    const Vector3& GetMoveDirection() const { return moveDirection_; }

    /// <summary>
    /// 壁を回り込む向き（+1か-1。まだ決まっていなければ0）。一度決めたら個体ごとに固定する。
    /// </summary>
    int32_t GetWallSense() const { return wallSense_; }
    void SetWallSense(int32_t sense) { wallSense_ = sense; }

protected:
    /// <summary>
    /// 出現したときの処理。
    /// </summary>
    virtual void OnSpawn() {}

    /// <summary>
    /// 凍っていない間、毎フレームの行動を行う。
    /// </summary>
    virtual void UpdateBehavior(const EnemyContext& context) = 0;

    /// <summary>
    /// 凍った瞬間の処理。
    /// </summary>
    virtual void OnFrozen() {}

    /// <summary>
    /// 氷が砕けて解放された瞬間の処理。
    /// </summary>
    virtual void OnReleased() {}

private:
    /// <summary>
    /// 氷エリア内なら氷ゲージを進め、外なら減らす。閾値に達したら凍らせる。
    /// </summary>
    void UpdateIceGauge(const EnemyContext& context);

    /// <summary>
    /// 氷ゲージの閾値に達して凍る。
    /// </summary>
    /// <param name="frozenDuration">凍ってから解放までの時間（秒）</param>
    void Freeze(float frozenDuration);

    /// <summary>
    /// 凍結の残り時間が尽きて解放される。氷ゲージは0に戻す。
    /// </summary>
    void Release();

protected:
    // 状態
    Vector3 position_ = { 0.0f, 0.0f, 0.0f };       // XZ平面上の位置
    Vector3 moveDirection_ = { 0.0f, 0.0f, 0.0f };  // 直近の移動の向き

    // 参照（所有しない）
    const EnemyDefinition* definition_ = nullptr;

private:
    // 状態
    bool isActive_ = false;                // プールから取り出されているか
    EnemyState state_ = EnemyState::kNormal;
    float iceGauge_ = 0.0f;                // 氷ゲージ（0から閾値まで）
    float frozenTimer_ = 0.0f;             // 凍結の残り時間（秒）
    int32_t wallSense_ = 0;                // 壁を回り込む向き
    uint32_t lastMistId_ = 0;              // 最後に凍った氷霧のID（0は氷霧以外で凍った、または凍っていない）
    int32_t brokenCount_ = 0;              // 砕かれた回数
    bool justReleased_ = false;            // 解放されたイベントが未処理か
};

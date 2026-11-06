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
    void Freeze(const EnemyContext& context);

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
};

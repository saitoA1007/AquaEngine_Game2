#pragma once
#include <cstdint>
#include <vector>
#include "Vector3.h"

/// <summary>
/// HPアイコン1つ分の、砕けた核の欠片の動き。六角形の枠の中だけで、重力で落ちて辺と欠片どうしで跳ね返り、底に積もって止まる。
/// 計算は枠の面の上の2D（核のモデル座標のX・Y）で行い、奥行き（Z）は枠の面へ寄せていく。欠片は円として当てる。
/// 位置と回転は核のモデル座標（単位はモデル単位）で持ち、描画への渡し方はHpHudViewが決める。
/// </summary>
class HpShardPile {
public:
    /// <summary>
    /// 動きの調整値。HpHudViewが持ち、DebugParameterへ公開する。
    /// </summary>
    struct Settings {
        float gravity = 9.0f;                 // 重力の強さ（モデル単位/秒^2。画面の下向き）
        float burstSpeed = 2.0f;              // 割れた瞬間に外へ飛ぶ速さ（モデル単位/秒）
        float burstUp = 1.5f;                 // 割れた瞬間に上へ跳ねる速さ（モデル単位/秒）
        float maxAngularSpeed = 8.0f;         // 割れた瞬間の回転の速さの最大（ラジアン/秒）
        float angularDamping = 2.0f;          // 回転の減り方（1秒あたり）
        float restitution = 0.3f;             // 跳ね返りの強さ（0で跳ねない、1で同じ速さで跳ねる）
        float friction = 0.15f;               // 辺に当たったときに、辺に沿った速さを減らす割合（1回の当たりあたり）
        float radiusScale = 0.6f;             // 欠片の当たりの円の半径の倍率（欠片の大きさに掛ける）
        float depthSettle = 6.0f;             // 奥行きを枠の面へ寄せる速さ（1秒あたり）
        float sleepSpeed = 0.05f;             // これより遅く、何かに触れている欠片は止める（モデル単位/秒）
    };

    /// <summary>
    /// 欠片1個の形の情報。破片モデルの読み込み結果から作る。
    /// </summary>
    struct ShardShape {
        Vector3 restCenter = { 0.0f, 0.0f, 0.0f };  // 割れる前の中心の位置（核のモデル座標）
        float size = 0.1f;                          // 欠片の大きさ（中心から端までの最大の距離。モデル単位）
    };

    /// <summary>
    /// 欠片1個の今の状態。描画に渡す。
    /// </summary>
    struct ShardPose {
        Vector3 position = { 0.0f, 0.0f, 0.0f };    // 中心の位置（核のモデル座標）
        Vector3 rotate = { 0.0f, 0.0f, 0.0f };      // 回転（オイラー角、ラジアン。MakeAffineMatrixと同じ順）
    };

    /// <summary>
    /// 欠片を割れる前の位置に置き、外向きと上向きに飛ばす。
    /// </summary>
    /// <param name="shapes">欠片の形の情報（欠片の数だけ）</param>
    /// <param name="spinAngle">割れた瞬間の核の回転（Y軸まわり、ラジアン）。欠片を核と同じ向きから始める。</param>
    /// <param name="innerApothem">枠の内側の、中心から辺までの距離（モデル単位）</param>
    /// <param name="settings">動きの調整値</param>
    void Start(const std::vector<ShardShape>& shapes, float spinAngle, float innerApothem, const Settings& settings);

    /// <summary>
    /// 欠片を消す（HPが戻ったときなど）。
    /// </summary>
    void Clear() { isActive_ = false; }

    /// <summary>
    /// 欠片を動かす。Startしていなければ何もしない。
    /// </summary>
    /// <param name="deltaTime">経過時間（秒）。止まらない時間を渡す。</param>
    /// <param name="settings">動きの調整値</param>
    void Update(float deltaTime, const Settings& settings);

    /// <summary>
    /// 欠片があるか（Startしてから、Clearしていないか）。
    /// </summary>
    bool IsActive() const { return isActive_; }

    /// <summary>
    /// 欠片の数。
    /// </summary>
    size_t GetCount() const { return shards_.size(); }

    /// <summary>
    /// 欠片の今の位置と回転。
    /// </summary>
    /// <param name="index">欠片の番号</param>
    ShardPose GetPose(size_t index) const { return { shards_[index].position, shards_[index].rotate }; }

private:
    // 1個の欠片の状態
    struct Shard {
        Vector3 position = { 0.0f, 0.0f, 0.0f };          // 中心の位置（核のモデル座標）
        Vector3 velocity = { 0.0f, 0.0f, 0.0f };          // 速度（Zは使わない）
        Vector3 rotate = { 0.0f, 0.0f, 0.0f };            // 回転（オイラー角）
        Vector3 angularVelocity = { 0.0f, 0.0f, 0.0f };   // オイラー角の変化の速さ（ラジアン/秒）
        float radius = 0.1f;                              // 当たりの円の半径
        bool isTouching = false;                          // この計算の回で、辺か欠片に触れたか
    };

    /// <summary>
    /// 1回分の計算（重力、移動、当たり）。
    /// </summary>
    void Step(float deltaTime, const Settings& settings);

    /// <summary>
    /// 六角形の内側の6本の辺との当たりを解く。
    /// </summary>
    void SolveWalls(Shard& shard, const Settings& settings) const;

    /// <summary>
    /// 欠片どうしの当たりを解く（重なりを押し戻し、近づく速さを跳ね返す）。
    /// </summary>
    void SolvePairs(const Settings& settings);

private:
    // 定数
    static constexpr int32_t kSubSteps = 2;          // 1フレームを分ける回数（速いときのすり抜けを減らす）
    static constexpr int32_t kSolverIterations = 4;  // 当たりを解く繰り返しの回数（積もったときの重なりを減らす）
    static constexpr float kMaxDeltaTime = 1.0f / 30.0f; // 1回で進める時間の上限（引っかかったときのすり抜けを防ぐ）

    // 状態
    std::vector<Shard> shards_;
    float innerApothem_ = 0.76f;   // 枠の内側の、中心から辺までの距離（モデル単位）
    bool isActive_ = false;
};

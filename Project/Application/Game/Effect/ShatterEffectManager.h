#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Model.h"
#include "WorldTransform.h"
#include "FractureInstance.h"
#include "IceMaterial.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// 敵が砕けたときの破片を飛ばして描画する。FractureInstanceを起動時に事前確保して使い回す。
/// 撃破でも解放でも、氷の殻の破片だけを飛ばす。
/// </summary>
class ShatterEffectManager : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 破片モデルの事前分割から、スロット数だけ破片のインスタンスを確保する。
    /// 破片モデルが空、または破壊グループが1つでないときは例外を投げる。
    /// </summary>
    /// <param name="shellModel">氷の殻の破片モデル。管理クラスが所有し、このオブジェクトより長く生存する。</param>
    ShatterEffectManager(GameEngine::Model* shellModel);

    /// <summary>
    /// 共有モデルのマテリアル参照を元に戻してから、個体のマテリアルを解放する。
    /// </summary>
    ~ShatterEffectManager() override;

    /// <summary>
    /// 調整値の反映と、飛んでいる破片の更新を行う。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 飛んでいる破片を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 破片を飛ばす。空きスロットがなければ一番古いものを再利用する。
    /// </summary>
    /// <param name="position">砕けた位置（XZを使う）</param>
    /// <param name="shellRadius">氷の殻の破片が収まる球の半径</param>
    void Spawn(const Vector3& position, float shellRadius);

    /// <summary>
    /// 同時破砕の計測を始める。指定数（0なら調整値の数）だけ常に砕け続け、一定時間のフレーム時間をログへ出す。
    /// </summary>
    void StartBenchmark(int32_t count = 0);

private:
    // 破片のモデルごとのデータ。マテリアルはモデルのバッファに設定する
    struct ShardSet {
        GameEngine::Model* model = nullptr;
        GameEngine::IceMaterial material;
        std::vector<uint32_t> chunkIds;          // 使う破片のID（最大kMaxChunks個）
        std::vector<Vector3> centroids;          // 破片の重心（回転の中心）
        std::vector<float> chunkRadii;           // 破片の大きさ（重心から端までの最大距離。地面との接触に使う）
        Vector3 center = { 0.0f, 0.0f, 0.0f };   // 使う破片全体のバウンディングの中心
        float halfExtent = 1.0f;                 // 全体の中心から端までの最大距離（大きさの正規化に使う）
        std::string groupName;                   // 破壊グループ名
        uint32_t originalSrvIndex = 0;           // モデル本来のマテリアルのSRV番号（デストラクタで戻す）
    };

    // 1回分の破砕
    struct Slot {
        GameEngine::FractureInstance shell;
        bool isActive = false;
        float age = 0.0f; // 経過時間（秒）
        float scale = 1.0f;  // 破片モデルをワールドへ拡大する倍率（速度・重力をモデル座標へ直すために使う）
        float baseY = 0.0f;  // 破片モデルの原点のワールドの高さ（地面との接触に使う）
    };

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// モデルから破片のIDと重心、全体の大きさを調べ、マテリアルを設定する。
    /// </summary>
    void SetUpShardSet(ShardSet& set, GameEngine::Model* model);

    /// <summary>
    /// スロットのインスタンスを確保する。起動時に一度だけ呼ぶ。
    /// </summary>
    void AllocateSlots();

    /// <summary>
    /// インスタンスの状態だけを戻して、各破片の初速と回転を決める。バッファは再確保しない。
    /// </summary>
    void LaunchShards(Slot& slot, const ShardSet& set, const Vector3& position, float radius);

    /// <summary>
    /// 空きスロットを探す。なければ一番古いスロットを返す。
    /// </summary>
    Slot& AcquireSlot();

    /// <summary>
    /// 1スロット分の破片を動かし、消えるときの縮小を反映する。
    /// </summary>
    void UpdateSlot(Slot& slot, float deltaTime);

    /// <summary>
    /// 破片を動かして、インスタンスの行列を更新する。重力で落ち、地面で跳ねてから滑り、縮んで消える。
    /// </summary>
    void UpdateShards(Slot& slot, float deltaTime, float fadeScale);

    /// <summary>
    /// 1個の破片を地面に当てる。跳ね返って止まり、地面にある間は摩擦で滑りと回転が弱まる。
    /// </summary>
    /// <param name="state">破片の状態（モデル座標）</param>
    /// <param name="centroidY">破片の重心のモデル座標での高さ</param>
    /// <param name="chunkRadius">破片の大きさ（モデル座標）</param>
    void CollideWithGround(const Slot& slot, GameEngine::FractureChunkState& state, float centroidY, float chunkRadius, float fadeScale, float deltaTime) const;

    /// <summary>
    /// 計測中の処理。常に指定数が砕け続けるよう補充し、フレーム時間を集める。
    /// </summary>
    void UpdateBenchmark();

    /// <summary>
    /// 計測結果をログファイルへ書き出す。
    /// </summary>
    void WriteBenchmarkLog() const;

private:
    // 調整値
    float minLaunchSpeed_ = 3.0f;       // 破片の最低初速（ワールド単位/秒）
    float maxLaunchSpeed_ = 7.0f;       // 破片の最高初速（ワールド単位/秒）
    float maxAngularSpeed_ = 8.0f;      // 破片の回転速度の上限（ラジアン/秒。各軸ランダム）
    float lifetime_ = 1.5f;             // 破片が消えるまでの時間（秒）
    float fadeStartRatio_ = 0.5f;       // 寿命のどこから縮み始めるか（0から1）
    float shardSizeRatio_ = 0.15f;      // 破片の大きさの倍率（1で敵の大きさに収まる大きさ）
    float gravity_ = 30.0f;             // 破片に掛かる重力（ワールド単位/秒の2乗）
    float restitution_ = 0.35f;         // 地面で跳ね返る速さの割合（0で跳ねない）
    float groundFriction_ = 4.0f;       // 地面にある間の摩擦（大きいほど早く滑りと回転が止まる。1/秒）
    float restSpeed_ = 0.6f;            // 跳ね返りの速さがこれ以下なら跳ねずに地面に止まる（ワールド単位/秒）
    int32_t parallelThreshold_ = 8;     // 使用中のスロットがこの数以上のとき、更新を並列に行う（0以下で常に直列）
    Vector4 shellColor_ = { 0.6f, 0.85f, 1.0f, 1.0f };  // 殻の破片の色
    int32_t benchmarkCount_ = 50;       // 計測時に同時に砕く数
    float benchmarkSeconds_ = 10.0f;    // 計測する時間（秒）
    float benchmarkSpread_ = 12.0f;     // 計測時に破片を出す範囲の半径

    // 定数
    static constexpr int32_t kSlotCount = 64;     // 事前確保するスロット数
    static constexpr size_t kMaxChunks = 15;      // 1回の破砕で使う破片の上限
    static constexpr float kBenchmarkWarmup = 1.0f; // 計測の開始後、集計しない時間（秒）
    inline static const char* kBenchmarkLogPath = "ShatterBenchmark.log"; // 計測結果の出力先（作業ディレクトリ）

    // 状態
    ShardSet shellSet_;
    std::vector<std::unique_ptr<Slot>> slots_;
    std::vector<Slot*> activeSlots_;  // 今フレームで更新するスロット（毎フレーム集め直す。並列更新に渡す）
    GameEngine::WorldTransform identityTransform_; // 破片の行列に親行列を含めるため、単位行列を渡す

    // 計測の状態
    bool isBenchmarking_ = false;
    float benchmarkTimer_ = 0.0f;
    std::chrono::steady_clock::time_point lastFrameTime_; // 前のフレームの時刻（実時間の計測用。FpsCounterは1秒ごとの平均のため使わない）
    std::vector<float> benchmarkFrames_; // 集計した各フレームの時間（ミリ秒）
    std::vector<float> benchmarkUpdateMs_; // 集計した各フレームの、破片の更新にかかったCPU時間（ミリ秒）
    std::vector<float> benchmarkDrawMs_;   // 集計した各フレームの、描画登録にかかったCPU時間（ミリ秒）
    std::vector<float> benchmarkSpawnMs_;  // 補充（Spawn）が起きたフレームの、Spawnにかかった合計のCPU時間（ミリ秒）

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

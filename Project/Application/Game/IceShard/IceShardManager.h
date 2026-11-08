#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Model.h"
#include "RefBuffer.h"
#include "IceMaterial.h"
#include "WorldTransform.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Application/Game/Enemy/EnemyManager.h"
#include "Application/Game/Player/IceMistInfo.h"

/// <summary>
/// 氷塊が毎フレーム読むプレイヤーの情報。GameSceneが作り、IceShardManagerは保持しない。
/// </summary>
struct IceShardInput {
    Vector3 playerPosition = { 0.0f, 0.0f, 0.0f }; // プレイヤーのワールド位置
    bool isCharging = false;                       // Aを押してチャージ中か（回収に使う）
};

/// <summary>
/// 氷塊の管理。撃破された敵の位置への落下、チャージ中の回収、氷霧と同時の連鎖の発射を担当する。
/// 氷塊は起動時に事前確保して使い回す。敵の中身は知らず、EnemyManagerの公開関数だけを使う。
/// 更新はUpdateShardsで行い、IGameObject::Updateでは行わない（GameSceneからの手動呼び出しとの重複を避ける）。
/// </summary>
class IceShardManager : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 氷塊の見た目と、事前確保するプールを準備する。
    /// </summary>
    /// <param name="sphereModel">氷塊の仮の見た目に使う球モデル（"sphere.obj"）。管理クラスが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="enemyManager">連鎖の標的と着弾の相手。シーンが所有し、このオブジェクトより長く生存する。</param>
    IceShardManager(GameEngine::Model* sphereModel, EnemyManager* enemyManager);
    ~IceShardManager() override = default;

    /// <summary>
    /// 調整値の反映だけを行う。氷塊の更新はUpdateShardsで行う。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 使用中の氷塊を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 氷塊を1フレーム分更新する。落下した氷塊の寿命、チャージ中の回収、所持の周回、連鎖の飛行と着弾を処理する。
    /// 更新順に頼らないよう、敵の更新後にGameSceneが呼ぶ。
    /// </summary>
    /// <param name="input">プレイヤーの情報。保持しない。</param>
    void UpdateShards(const IceShardInput& input);

    /// <summary>
    /// 氷塊をその場に落とす。解放では呼ばない。プールが満杯なら、一番古い地面の氷塊を再利用する。
    /// </summary>
    /// <param name="position">落とす位置（XZを使う）</param>
    /// <param name="count">落とす数</param>
    void Drop(const Vector3& position, int32_t count);

    /// <summary>
    /// 氷霧と同時に、持っている全部の氷塊を連鎖させて発射する。範囲内に敵がいなければ何もせず、氷塊は消費しない。
    /// 敵に当たった氷塊は消えず、範囲内の全敵に順に当たってから砕けて消える。
    /// </summary>
    /// <param name="info">氷霧の情報。範囲と移動方向を使う。</param>
    void Launch(const IceMistInfo& info);

    /// <summary>
    /// 持っている氷塊の数を取得する。発射待ちの氷塊は含まない。
    /// </summary>
    int32_t GetHeldCount() const;

private:
    // 氷塊の状態
    enum class ShardState {
        kInactive, // プールの空き
        kGround,   // 地面に落ちている
        kPulled,   // チャージ中のプレイヤーへ引き寄せられている
        kHeld,     // 所持してプレイヤーの周りを回っている
        kQueued,   // 発射済みで、自分の順番を待っている（見た目は所持と同じ）
        kFlying,   // 標的の敵へ飛んでいる
    };

    // 氷塊1個分
    struct Shard {
        ShardState state = ShardState::kInactive;
        Vector3 position = { 0.0f, 0.0f, 0.0f };
        float age = 0.0f;           // 地面に落ちてからの経過時間（秒）
        float orbitAngle = 0.0f;    // 所持中に回る位置の基準角（ラジアン）
        int32_t volleyIndex = -1;   // 所属する連鎖（volleys_の番号）
        float launchDelay = 0.0f;   // 飛び始めるまでの残り時間（秒）
        size_t nextTarget = 0;      // 連鎖の順番のうち、次に探し始める位置
        EnemyTarget target;         // 今飛んでいる先の標的
        Vector3 hopStart = { 0.0f, 0.0f, 0.0f };  // 今の区間の出発点
        Vector3 hopEnd = { 0.0f, 0.0f, 0.0f };    // 今の区間の到着点（標的の位置を追う）
        float hopTimer = 0.0f;      // 今の区間の経過時間（秒）
    };

    // 1回の発射分の連鎖。同じ順番を、その発射の全氷塊が回る
    struct Volley {
        std::vector<EnemyTarget> order; // 連鎖の順番
        float hopTime = 0.1f;           // 1区間にかける時間（秒）
        int32_t shardCount = 0;         // まだ飛んでいる氷塊の数（0なら空き）
    };

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 氷塊の見た目（氷マテリアルと参照バッファ）を準備する。
    /// </summary>
    void CreateVisual();

    /// <summary>
    /// 空きの氷塊を返す。なければ一番古い地面の氷塊を再利用する。どちらもなければnullptr。
    /// </summary>
    Shard* AcquireShard();

    /// <summary>
    /// 空きの連鎖を返す。なければ追加する。
    /// </summary>
    int32_t AcquireVolley();

    /// <summary>
    /// 地面の氷塊を進める。寿命が尽きたら消し、チャージ中で範囲内なら引き寄せを始める。
    /// </summary>
    void UpdateGroundShard(Shard& shard, float deltaTime, const IceShardInput& input, int32_t& carryCount);

    /// <summary>
    /// 引き寄せ中の氷塊をプレイヤーへ近づけ、十分近づいたら所持にする。チャージをやめたら地面へ戻す。
    /// </summary>
    void UpdatePulledShard(Shard& shard, float deltaTime, const IceShardInput& input);

    /// <summary>
    /// 所持と発射待ちの氷塊を、プレイヤーの周りで回す。発射待ちは順番が来たら飛び始める。
    /// </summary>
    void UpdateOrbitShard(Shard& shard, float deltaTime);

    /// <summary>
    /// 飛んでいる氷塊を標的へ進める。到着したら着弾を処理して次の標的へ向かい、全員に当たったら砕けて消える。
    /// </summary>
    void UpdateFlyingShard(Shard& shard, float deltaTime);

    /// <summary>
    /// 連鎖の順番から、次の有効な標的を探して区間を始める。倒された敵は時間をかけずに飛ばす。
    /// </summary>
    /// <returns>標的が見つかればtrue。残っていなければfalse</returns>
    bool StartNextHop(Shard& shard);

    /// <summary>
    /// 氷塊を使い終える。所属する連鎖の数を減らす。
    /// </summary>
    void FinishShard(Shard& shard);

    /// <summary>
    /// 所持中の周回位置を求める。
    /// </summary>
    Vector3 GetOrbitPosition(float orbitAngle) const;

    /// <summary>
    /// 範囲内の敵から連鎖の順番を作る。1個目は移動方向の最寄り、以降は直前の敵の最寄り。
    /// </summary>
    void BuildChainOrder(const IceMistInfo& info, std::vector<EnemyTarget>& order);

    /// <summary>
    /// 連鎖の1個目を選ぶ。移動中は移動方向の円錐内の最寄り（なければ全体の最寄り）、静止中は全体の最寄り。
    /// </summary>
    /// <returns>remainingTargets_の中の番号</returns>
    size_t PickFirstIndex(const IceMistInfo& info) const;

    /// <summary>
    /// 1個の氷塊を描画キューへ登録する。
    /// </summary>
    void DrawShard(const Shard& shard);

private:
    // 調整値（回収）
    int32_t maxHeldCount_ = 60;        // 所持できる数の上限（引き寄せ中の分も含めて数える）
    float attractRadius_ = 5.0f;       // チャージ中に氷塊を引き寄せ始める距離
    float attractSpeed_ = 18.0f;       // 引き寄せの速さ（ワールド単位/秒）
    float collectRadius_ = 0.8f;       // 所持になる距離（周回の高さへの距離）
    float groundLifetime_ = 8.0f;      // 落ちた氷塊が消えるまでの時間（秒）
    float dropScatter_ = 0.6f;         // 落とす位置のばらつき（半径）

    // 調整値（所持の見た目）
    float orbitRadius_ = 1.0f;         // 所持中に回る半径
    float orbitHeight_ = 1.3f;         // 所持中に回る高さ
    float orbitSpeed_ = 2.5f;          // 所持中に回る速さ（ラジアン/秒）

    // 調整値（連鎖）
    float aimHalfAngleDegrees_ = 45.0f; // 移動方向にいるとみなす角度の片側の広さ（度）
    float launchInterval_ = 0.05f;      // 氷塊が1個ずつ飛び始める間隔（秒）
    float maxLaunchSpan_ = 0.6f;        // 全氷塊が飛び始めるまでの時間の上限（秒）。超える分は間隔を詰める
    float hopTime_ = 0.08f;             // 敵から敵へ移る1区間の時間（秒）
    float maxHopSpan_ = 1.5f;           // 1個の氷塊が全敵を回る時間の上限（秒）。超える分は区間を短くする
    float hopArcHeight_ = 0.4f;         // 区間の途中で持ち上がる高さ
    float hitHeight_ = 0.4f;            // 当たるときの高さ
    float shardIcePower_ = 2.0f;        // 凍っていない敵へ与える氷ゲージ（大幅な凍結）

    // 調整値（見た目）
    float shardScale_ = 0.25f;                                // 氷塊の大きさ（球の半径）
    Vector4 shardColor_ = { 0.6f, 0.85f, 1.0f, 1.0f };        // 氷塊の色

    // 定数
    static constexpr size_t kMaxShards = 512;       // 事前確保する氷塊の数
    static constexpr float kFadeTime = 1.0f;        // 地面の氷塊が消える直前に縮む時間（秒）
    static constexpr float kMoveEpsilon = 0.1f;     // 移動方向の長さがこれ以下なら静止とみなす
    static constexpr float kGoldenAngle = 2.39996f; // 所持の位置をずらす角（ラジアン）
    static constexpr float kMinHopTime = 0.01f;     // 1区間の時間の下限（秒）

    // 状態
    std::vector<Shard> shards_;
    std::vector<Volley> volleys_;
    int32_t orbitCounter_ = 0;      // 所持にした数（周回の基準角を散らすために使う）
    float orbitPhase_ = 0.0f;       // 所持の周回の回転量（ラジアン）
    Vector3 playerPosition_ = { 0.0f, 0.0f, 0.0f }; // 直近のプレイヤー位置（所持の周回の中心。表示と発射の起点のためだけにコピーを持つ）
    std::vector<EnemyTarget> candidates_;        // 発射時の範囲内の敵（使い回す）
    std::vector<EnemyTarget> remainingTargets_;  // 順番づけ中の未訪問の敵（使い回す）
    std::vector<Vector3> remainingPositions_;    // remainingTargets_の位置（使い回す）

    // 参照（所有しない）
    EnemyManager* enemyManager_ = nullptr;
    GameEngine::Model* model_ = nullptr;

    // 描画
    GameEngine::WorldTransform drawTransform_; // 全個体で使い回す（行列は登録時に取り込まれる）
    GameEngine::IceMaterial iceMaterial_;
    std::vector<GameEngine::RefBuffer> refBuffers_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

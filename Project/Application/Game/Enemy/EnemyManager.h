#pragma once
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "IGameObject.h"
#include "DebugParameter.h"
#include "ModelManager.h"
#include "RefBuffer.h"
#include "Material.h"
#include "IceMaterial.h"
#include "WorldTransform.h"
#include "Vector4.h"
#include "Application/Game/Enemy/Enemy.h"
#include "Application/Game/Enemy/EnemyDefinition.h"
#include "Application/Game/Enemy/EnemyGrid.h"
#include "Application/Game/Enemy/EnemyGaugeView.h"
#include "Application/Game/Enemy/EnemyIO.h"
#include "Application/Game/Player/IceMistInfo.h"
#include "Application/Game/Effect/ShatterEffectManager.h"
#ifdef USE_IMGUI
#include "Application/Game/Enemy/EnemyListEditor.h"
#endif

/// <summary>
/// スポーンのルール。フェーズごとにGameSceneが渡す。EnemyManagerはフェーズを知らない。
/// </summary>
struct EnemySpawnRule {
    float interval = 1.0f;                 // スポーンの間隔（秒）
    int32_t perInterval = 0;               // 1回のスポーンで出そうとする数
    int32_t maxEnemies = 0;                // 同時に存在できる敵の最大数（全種類の合計）
    std::map<std::string, float> weights;  // 敵のIDごとの出現の重み（書いていない種類は0）
};

/// <summary>
/// 全敵の更新・描画・凍結・スポーンを担当する唯一の敵用IGameObject。
/// Playerを知らず、EnemyInputを受け取ってEnemyOutputを返す。
/// 敵の更新はUpdateEnemiesで行い、IGameObject::Updateでは行わない（GameSceneからの手動呼び出しとの重複を避ける）。
/// </summary>
class EnemyManager : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 敵一覧を読み込み、種類ごとの個体と見た目を事前に確保する。
    /// 一覧が不正、またはモデルが見つからないときは例外を投げる。
    /// </summary>
    /// <param name="modelManager">モデル取得用。コンストラクタの間だけ使い、保持しない。</param>
    /// <param name="fieldRadius">グリッドの範囲を決めるフィールド半径</param>
    /// <param name="camera">氷ゲージの表示位置を求めるカメラ。シーンが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="whiteTextureHandle">氷ゲージの矩形に使う白いテクスチャのハンドル</param>
    /// <param name="shatterEffect">砕けた位置を伝える先。シーンが所有し、このオブジェクトより長く生存する。</param>
    EnemyManager(GameEngine::ModelManager* modelManager, float fieldRadius, const GameEngine::Camera* camera, uint32_t whiteTextureHandle, ShatterEffectManager* shatterEffect);
    ~EnemyManager() override = default;

    /// <summary>
    /// 調整値の反映と、氷の殻の色の更新だけを行う。敵の更新はUpdateEnemiesで行う。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 使用中の敵と、凍結中の氷の殻を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 敵一覧の編集画面を表示する。USE_IMGUIのときだけ動く。
    /// </summary>
    void DebugUpdate() override;

    /// <summary>
    /// 全敵を1フレーム分更新する。スポーン、凍結の進行と行動、凍った敵との重なりの解消、プレイヤーとの接触を処理する。
    /// 更新順に頼らないよう、プレイヤーの更新後にGameSceneが呼ぶ。
    /// </summary>
    /// <param name="input">プレイヤーの情報。保持しない。</param>
    /// <returns>プレイヤーが受けるダメージと押し戻し量</returns>
    EnemyOutput UpdateEnemies(const EnemyInput& input);

    /// <summary>
    /// 氷霧を範囲内の敵へ当てる。凍っていない敵は氷ゲージを進め、凍っている敵は砕く。
    /// 同じ氷霧で凍った敵は砕かない。撃破した敵は破片を飛ばす。前フレームのグリッドを使うため、UpdateEnemiesより前でも呼べる。
    /// </summary>
    /// <param name="info">氷霧の情報。保持しない。</param>
    void ApplyIceMist(const IceMistInfo& info);

    /// <summary>
    /// 敵を撃破したときに呼ぶ関数を設定する。氷霧と氷塊のどちらで倒した場合も呼ばれる。解放（倒れない）では呼ばれない。
    /// </summary>
    /// <param name="callback">撃破の情報を受け取る関数。呼び出し先がこのオブジェクトより長く生存する必要がある。</param>
    void SetDefeatCallback(const std::function<void(const EnemyDefeatInfo&)>& callback) { defeatCallback_ = callback; }

    /// <summary>
    /// スポーンのルールを設定する。重みは敵のIDで受け取り、ここで種類の番号に変換する。
    /// 出ている敵は消さない。タイマーは引き継ぐ。
    /// </summary>
    /// <param name="rule">新しいルール。保持しない。</param>
    void SetSpawnRule(const EnemySpawnRule& rule);

    /// <summary>
    /// 敵一覧を読み取り専用で取得する。フェーズ表の検証と編集画面が敵のIDを使う。
    /// </summary>
    const std::vector<EnemyDefinition>& GetDefinitions() const { return definitions_; }

    /// <summary>
    /// 円の範囲と重なる使用中の敵を集める。前フレームのグリッドを使い、位置は現在のものを判定する。
    /// </summary>
    /// <param name="center">円の中心（XZを使う）</param>
    /// <param name="radius">円の半径</param>
    /// <param name="out">結果。呼び出しのたびに空にしてから入れる</param>
    void CollectTargets(const Vector3& center, float radius, std::vector<EnemyTarget>& out);

    /// <summary>
    /// 標的が今も同じ敵として生きているかを確かめ、その現在位置を返す。
    /// </summary>
    /// <param name="target">確かめる標的</param>
    /// <param name="position">生きているときの現在位置</param>
    /// <returns>生きていればtrue。倒された、または別の敵として再利用されていればfalse</returns>
    bool GetTargetPosition(const EnemyTarget& target, Vector3& position) const;

    /// <summary>
    /// 氷塊が当たった処理。凍っていない敵は氷ゲージを大きく進め、凍っている敵は砕く。
    /// 氷霧と違い、同じ連鎖で凍った敵も砕ける。標的が無効なら何もしない。
    /// </summary>
    /// <param name="target">当たった標的</param>
    /// <param name="icePower">凍っていない敵の氷ゲージへ足す量</param>
    /// <param name="chainId">発射元の氷霧のID。撃破の通知に添える</param>
    void ApplyShardHit(const EnemyTarget& target, float icePower, uint32_t chainId);

private:
    /// <summary>
    /// 凍った敵を砕く。撃破なら破片を飛ばして撃破を通知し、解放なら次のEmitReleaseShattersが殻だけを飛ばす。
    /// </summary>
    /// <param name="enemy">砕く敵（凍っていること）</param>
    /// <param name="chainId">撃破の通知に添える連鎖のID（氷霧のID）</param>
    void ShatterEnemy(Enemy* enemy, uint32_t chainId);

    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 敵一覧の各行に対して、個体の事前確保と見た目の準備を行う。
    /// </summary>
    void CreatePools(GameEngine::ModelManager* modelManager);

    /// <summary>
    /// 氷の殻の見た目（氷マテリアルと参照バッファ）を準備する。
    /// </summary>
    void CreateShell(GameEngine::ModelManager* modelManager);

    /// <summary>
    /// 一定間隔で、プレイヤーの周囲の円周上に敵を出す。
    /// </summary>
    void SpawnEnemies(const EnemyInput& input, float deltaTime);

    /// <summary>
    /// 重みで種類を抽選して1体出す。重みが0の種類とプールに空きがない種類は外し、候補がなければ出さない。
    /// </summary>
    /// <returns>出せたらtrue</returns>
    bool SpawnOne(const EnemyInput& input);

    /// <summary>
    /// 使用中の敵の総数を数える。
    /// </summary>
    int32_t CountActiveEnemies() const;

    /// <summary>
    /// 使用中の敵を集める。
    /// </summary>
    void CollectActiveEnemies();

    /// <summary>
    /// 各個体の氷ゲージと行動を更新する。
    /// </summary>
    void UpdateEnemyBehaviors(const EnemyInput& input, float deltaTime);

    /// <summary>
    /// 解放された敵の位置から、氷の殻だけの破片を飛ばす。
    /// </summary>
    void EmitReleaseShatters();

    /// <summary>
    /// 動いている敵と凍った敵の重なりを解消し、壁を回り込ませる。
    /// </summary>
    void ResolveWalls(const EnemyInput& input, float deltaTime);

    /// <summary>
    /// 敵をフィールドの円内へ収める。
    /// </summary>
    void ClampEnemiesToField(float fieldRadius);

    /// <summary>
    /// 凍った敵によるプレイヤーの押し戻しと、動く敵との接触ダメージを求める。
    /// </summary>
    void ResolvePlayerContact(const EnemyInput& input, EnemyOutput& output);

private:
    // 種類ごとの見た目。同じ種類の個体は1つのマテリアルを共有する
    struct EnemyVisual {
        GameEngine::Model* model = nullptr;
        std::vector<GameEngine::RefBuffer> refBuffers;
        std::vector<GameEngine::Material> materials;
    };

    // 種類ごとの事前確保した個体
    struct EnemyPool {
        std::vector<std::unique_ptr<Enemy>> enemies;
        std::unique_ptr<EnemyVisual> visual;
        size_t searchStart = 0; // 空きを探し始める位置
    };

    /// <summary>
    /// プールから空いている個体を取り出す。空きがなければnullptr。
    /// </summary>
    Enemy* AcquireEnemy(EnemyPool& pool);

private:
    // 調整値
    float spawnDistance_ = 28.0f;    // プレイヤーからのスポーン距離
    float freezeRate_ = 1.0f;        // 氷エリア内で氷ゲージが進む速さ（ゲージ/秒）
    float decayRate_ = 0.5f;         // 氷エリア外で氷ゲージが減る速さ（ゲージ/秒）
    float frozenDuration_ = 10.0f;   // 凍ってから解放までの時間（秒）
    float meltTime_ = 0.6f;          // 解放の直前に、殻が縮んで溶ける時間（秒）。0で溶ける表現なし
    float shellScale_ = 1.4f;       // 氷の殻の大きさ（見た目の大きさに対する倍率）
    Vector4 shellColor_ = { 0.6f, 0.85f, 1.0f, 1.0f }; // 氷の殻の色
    bool enableEnemyWall_ = false;       // 動く敵が凍った敵を押し出されて避けるか（密度が高いため既定では無効。後で重なりの対応を決める）
    bool enableWallPushPlayer_ = false;  // 凍った敵がプレイヤーを押し戻すか（同上）
    EnemyGaugeView::Settings gaugeSettings_; // 氷ゲージの表示の調整値
    float slideStrength_ = 1.0f;     // 壁に真正面からぶつかったときの横滑りの強さ（移動速度に対する倍率）

    // 定数
    static constexpr float kGridCellSize = 2.0f;     // グリッドのセルの一辺
    static constexpr float kGridMargin = 5.0f;       // グリッドの範囲をフィールド半径より広げる量
    static constexpr float kGridLag = 0.25f;         // グリッドが1フレーム古いことを補う検索の余白
    static constexpr int32_t kResolveIterations = 2; // 壁の重なりを解消する繰り返し回数
    inline static const char* kShellModelName = "sphere.obj"; // 氷の殻に使うモデル名
    inline static const char* kGaugeModelName = "plane.obj";  // 氷ゲージに使う平面モデル名

    // 状態
    std::vector<EnemyDefinition> definitions_;
    std::vector<EnemyPool> pools_;
    std::vector<Enemy*> active_;     // 使用中の個体（毎フレーム集め直す）
    std::vector<Enemy*> neighbors_;  // 範囲検索の結果（使い回す）
    float spawnTimer_ = 0.0f;
    float spawnInterval_ = 1.0f;         // 現在のルールのスポーン間隔（秒）
    int32_t spawnPerInterval_ = 0;       // 現在のルールの1回の数（ルールが来るまで出さない）
    int32_t maxEnemies_ = 0;             // 現在のルールの同時存在数の上限
    std::vector<float> spawnWeights_;    // 現在のルールの重み（pools_と同じ並び）
    std::vector<float> spawnCandidates_; // 抽選中の重み（使い回す）
    float maxEnemyRadius_ = 0.0f;    // 全種類の当たり半径の最大値
    Vector3 lastPlayerPosition_ = { 0.0f, 0.0f, 0.0f }; // 直近のプレイヤー位置（ゲージの表示数を絞る基準。表示のためだけにコピーを持つ）
    EnemyGrid grid_;
    ShatterEffectManager* shatterEffect_ = nullptr; // 破砕の破片（所有しない）
    std::function<void(const EnemyDefeatInfo&)> defeatCallback_; // 撃破の通知先（GameSceneが設定する）
    uint32_t spawnCounter_ = 0;      // 出現させた敵の数（出現ごとの連番の採番に使う）
    std::unique_ptr<EnemyGaugeView> gaugeView_; // 氷ゲージの仮表示（平面モデルが要るためコンストラクタで作る）

    // 描画
    GameEngine::WorldTransform drawTransform_; // 全個体で使い回す（行列は登録時に取り込まれる）
    GameEngine::Model* shellModel_ = nullptr;
    GameEngine::IceMaterial iceMaterial_;
    std::vector<GameEngine::RefBuffer> shellRefBuffers_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;

#ifdef USE_IMGUI
    // 敵一覧の編集画面
    std::unique_ptr<EnemyListEditor> listEditor_;
#endif
};

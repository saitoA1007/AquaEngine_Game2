#pragma once
#include <cstdint>
#include <memory>
#include "IGameObject.h"
#include "DebugParameter.h"
#include "Model.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "ParticleBehavior.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// 氷霧を発生させた瞬間の見た目。中心から32方向へ霧が吹き出して縁で止まり、範囲の中から冷気が上がり、画面が一瞬水色に光る。
/// 霧と冷気はエンジンのParticleBehavior（ゲームの時間で動く）で、見た目の基本設定はResources/Json/GameDataのIceMistBurst.jsonとIceMistRise.jsonで決める。
/// 地面の円の発光とチャージ中の表示は、IceAreaViewが担当する。
/// Play(中心, 半径)の形を保てば、別の見た目へ差し替えられる。
/// </summary>
class IceMistEffect : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 霧と冷気のパーティクルと、画面の光の板を作る。
    /// </summary>
    /// <param name="planeModel">パーティクル1枚の板に使うモデル（"plane.obj"）。管理クラスが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="textureManager">パーティクルのテクスチャ取得用。コンストラクタの間だけ使う。</param>
    /// <param name="whiteTextureHandle">画面の光の板に使う白い画像のハンドル</param>
    IceMistEffect(GameEngine::Model* planeModel, GameEngine::TextureManager* textureManager, uint32_t whiteTextureHandle);

    /// <summary>
    /// 調整値の反映、パーティクルの更新、画面の光の更新を行う。
    /// </summary>
    void Update() override;

    /// <summary>
    /// パーティクルと画面の光を描画キューへ登録する。画面の光はHUDより先に作られるため、HUDの下に描かれる。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 氷霧の見た目を発生させる。チャージの度合いは半径から求める。
    /// </summary>
    /// <param name="center">氷霧の中心（XZを使う）</param>
    /// <param name="radius">氷霧の半径（霧がこの距離の縁で止まる）</param>
    void Play(const Vector3& center, float radius);

private:
    // 1回の霧の吹き出しの形。半径とチャージから求める
    struct BurstShape {
        float radius = 4.0f;           // 霧が止まる縁の半径
        float chargeRatio = 0.0f;      // チャージの度合い（0から1）
        int32_t countPerDirection = 3; // 1方向あたりの粒の数
        float endSize = 1.4f;          // 粒が最後に広がる大きさ
    };

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 氷霧の半径から、チャージの度合い（0から1）を求める。
    /// </summary>
    float CalcChargeRatio(float radius) const;

    /// <summary>
    /// 半径とチャージの度合いから、霧の吹き出しの形を求める。
    /// </summary>
    BurstShape CalcBurstShape(float radius, float chargeRatio) const;

    /// <summary>
    /// 中心から全方向へ霧を吹き出させる。
    /// </summary>
    void EmitBurst(const Vector3& center, const BurstShape& shape);

    /// <summary>
    /// 霧の粒を1個、範囲の中の出し始める位置から外へ向けて出す。縁で止まるよう、寿命を距離から決める。
    /// </summary>
    void EmitBurstParticle(const Vector3& center, const BurstShape& shape, float cosAngle, float sinAngle);

    /// <summary>
    /// 範囲の中から冷気を上げる。
    /// </summary>
    void EmitRise(const Vector3& center, float radius);

    /// <summary>
    /// 画面の光を始める。
    /// </summary>
    void StartFlash();

    /// <summary>
    /// 画面の光を進め、残り時間に合わせて板の濃さを決める。
    /// </summary>
    void UpdateFlash();

private:
    // 調整値（霧の吹き出し）
    float startAreaRatio_ = 0.9f;      // 霧を出し始める位置の範囲（半径に対する割合。中心からこの距離までに散らす）
    float minTravelRatio_ = 0.1f;      // 霧が外へ流れる距離の下限（半径に対する割合。縁の近くから出た粒も少しは動く）
    float tapRadius_ = 4.0f;           // チャージの度合いが0になる半径（Playerの単押しの半径に合わせる）
    float maxRadius_ = 8.0f;           // チャージの度合いが1になる半径（Playerのフルチャージの半径に合わせる）
    float tapEndSize_ = 1.4f;          // 単押しで粒が最後に広がる大きさ
    float maxEndSize_ = 2.2f;          // 最大チャージで粒が最後に広がる大きさ
    int32_t tapCount_ = 3;             // 単押しの1方向あたりの粒の数
    int32_t maxCount_ = 6;             // 最大チャージの1方向あたりの粒の数
    float lifeVariation_ = 0.08f;      // 粒の寿命のばらつき（割合）
    float burstHeight_ = 0.25f;        // 霧を出す高さ。氷エリアの板の上面（IceAreaViewのslabThickness 0.2＋layerLift 0.02＝0.22）の少し上。板の厚みを変えたらここも合わせる

    // 調整値（冷気）
    int32_t riseCount_ = 1;            // 1回の氷霧で冷気を発生させる回数（1回あたりの粒の数はIceMistRise.jsonで決める）
    float riseHeight_ = 0.3f;          // 冷気を出す高さ。板の上面（0.22）に下側が食い込みにくい高さ

    // 調整値（画面の光）
    Vector4 flashColor_ = { 0.55f, 0.85f, 1.0f, 1.0f }; // 画面の光の色
    float flashMaxAlpha_ = 0.18f;      // 画面の光の最大の濃さ
    float flashTime_ = 0.25f;          // 画面の光が消えるまでの時間（秒）

    // 定数
    static constexpr int32_t kDirectionCount = 32;       // 霧を吹き出す方向の数
    static constexpr uint32_t kMaxBurstParticles = 512;  // 霧の粒の同時数の上限
    static constexpr uint32_t kMaxRiseParticles = 128;   // 冷気の粒の同時数の上限
    static constexpr float kBurstSpeed = 45.0f;          // IceMistBurst.jsonの初速の中心（ワールド単位/秒）
    static constexpr float kMoveEndRatio = 0.4f;         // IceMistBurst.jsonの速度カーブで、寿命のどこで粒が止まるか（0から1）
    static constexpr float kStopDistanceRatio = 1.0f / 3.0f; // 速度カーブの形から決まる「初速×止まるまでの時間」に対する移動距離の割合
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
    inline static const char* kBurstParticleName = "IceMistBurst"; // 霧の設定のJSONの名前
    inline static const char* kRiseParticleName = "IceMistRise";   // 冷気の設定のJSONの名前
    inline static const char* kDebugName = "IceMistEffect";

    // 状態
    float flashTimer_ = 0.0f;          // 画面の光の経過時間（秒）
    bool isFlashing_ = false;          // 画面の光が出ているか

    // 描画
    std::unique_ptr<GameEngine::ParticleBehavior> burst_;
    std::unique_ptr<GameEngine::ParticleBehavior> rise_;
    std::unique_ptr<GameEngine::Sprite> flashSprite_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

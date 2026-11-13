#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include "IGameObject.h"
#include "Camera.h"
#include "FontManager.h"
#include "Sprite.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Application/Game/UI/OutlinedText.h"

/// <summary>
/// 連鎖数の表示。1回の氷霧から飛んだ氷塊で砕いた数（氷霧で倒した分は数えない）を、大きな数字と小さな「HIT!」で数え上げる。
/// 数字の後ろには、氷が砕けた形の画像（氷の破裂）を置く。数字と「HIT!」には縁取りと影が付く。
/// 表示は1つの連鎖につき1つ。新しく砕けた敵の画面位置へ移って数が増え、数が大きいほど数字が大きくなる。
/// 数字・「HIT!」・縁と影・破裂のすべてが画面の内側に収まる位置へ、毎フレーム寄せる。
/// 一定時間増えなければ消える。同時に出せる表示は上限まで。動きはゲームの時間で進める。
/// </summary>
class ComboPopupView : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 表示用の文字と画像を上限の数だけ作る。フォントが見つからなければ例外を投げる。
    /// </summary>
    /// <param name="camera">座標の変換に読むカメラ。シーンが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="fontManager">フォント取得用。コンストラクタの間だけ使い、保持しない。</param>
    /// <param name="burstTextureHandle">氷の破裂の画像（ComboBurst.png）のハンドル</param>
    ComboPopupView(const GameEngine::Camera* camera, GameEngine::FontManager* fontManager, uint32_t burstTextureHandle);
    ~ComboPopupView() override = default;

    /// <summary>
    /// 表示の時間を進め、数字・「HIT!」・破裂の位置・大きさ・色を更新する。増えなくなった表示は消す。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 使用中の表示を描画キューへ登録する。破裂のスプライトは文字より下に描かれる。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 連鎖で1体砕いたことを伝える。同じ連鎖の表示があれば数を増やして移し、なければ1から出す。
    /// 表示が上限まで使われているときは、新しい連鎖の表示は出さない。
    /// 画面の外で砕いた敵の場合も表示は出し、その敵にいちばん近い画面の内側へ置く。
    /// </summary>
    /// <param name="chainId">連鎖のID（発射元の氷霧のID）。0（氷霧で倒した）は数えない</param>
    /// <param name="worldPosition">砕いた敵の位置（ワールド座標）</param>
    void AddHit(uint32_t chainId, const Vector3& worldPosition);

    /// <summary>
    /// 描くかどうかを設定する。falseの間も、数と残り時間は進む・保たれ、trueに戻すと元どおり描く。毎フレーム呼ぶ。
    /// </summary>
    /// <param name="isVisible">描くか</param>
    void SetVisible(bool isVisible) { isVisible_ = isVisible; }

private:
    // 表示の外形（数字の中心からの相対位置。左上と右下）
    struct Bounds {
        Vector2 min = { 0.0f, 0.0f };
        Vector2 max = { 0.0f, 0.0f };
        bool isEmpty = true;                                // まだ何も含んでいないか

        /// <summary>
        /// 矩形を含むように外形を広げる。
        /// </summary>
        void Include(const Vector2& rectMin, const Vector2& rectMax);
    };

    // 表示1つ
    struct Popup {
        bool isActive = false;                              // 使用中か
        uint32_t chainId = 0;                               // 数えている連鎖のID
        int32_t count = 0;                                  // 砕いた数
        Vector2 targetPosition = { 0.0f, 0.0f };            // 数字の中心にしたい画面座標（画面の内側へ寄せる前）
        float idleTimer = 0.0f;                             // 最後に増えてからの経過時間（秒）
        float punchTimer = 0.0f;                            // 増えた瞬間の弾みの経過時間（秒）
        float burstFromAngle = 0.0f;                        // 破裂の向き（弾み始め。ラジアン）
        float burstToAngle = 0.0f;                          // 破裂の向き（弾み終わり。ラジアン）
        std::unique_ptr<OutlinedText> number;               // 数字
        std::unique_ptr<OutlinedText> hit;                  // 「HIT!」
        std::unique_ptr<GameEngine::Sprite> burst;          // 氷の破裂
    };

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 時間・大きさ・弾みの調整値を登録する。
    /// </summary>
    void RegisterSizeParameters();

    /// <summary>
    /// 位置関係（数字との位置、破裂の大きさと回り、画面の端）の調整値を登録する。
    /// </summary>
    void RegisterLayoutParameters();

    /// <summary>
    /// 色・縁・影の調整値を登録する。
    /// </summary>
    void RegisterColorParameters();

    /// <summary>
    /// 表示を上限の数だけ作る。
    /// </summary>
    /// <param name="font">文字のフォント</param>
    /// <param name="burstTextureHandle">氷の破裂の画像のハンドル</param>
    void CreateViews(const GameEngine::Font* font, uint32_t burstTextureHandle);

    /// <summary>
    /// ワールド座標を画面座標（1280x720）へ変換する。
    /// </summary>
    Vector2 WorldToScreen(const Vector3& worldPosition) const;

    /// <summary>
    /// 数が増えた瞬間の弾みを始め、破裂の向きを次の向きへ変える。
    /// </summary>
    void StartPunch(Popup& popup);

    /// <summary>
    /// 表示1つの時間を進め、大きさと色を決めて、位置を画面の内側へ寄せて更新する。
    /// </summary>
    void UpdatePopup(Popup& popup, float deltaTime);

    /// <summary>
    /// 数に応じた数字の大きさ（px）。弾みの分を含む。
    /// </summary>
    /// <param name="count">砕いた数</param>
    /// <param name="punchRate">弾みの進み具合（0で弾み始め、1で終わり）</param>
    float CalcNumberSize(int32_t count, float punchRate) const;

    /// <summary>
    /// 数字の中心を原点にした、「HIT!」の左下の位置。数字の右下に、数字の大きさに応じてずらして置く。
    /// </summary>
    /// <param name="numberSize">数字の大きさ（px）</param>
    /// <param name="numberExtent">数字全体の大きさ（px）</param>
    Vector2 CalcHitOrigin(float numberSize, const Vector2& numberExtent) const;

    /// <summary>
    /// 数字、「HIT!」、縁と影、破裂（弾みと向きの分を含む）のすべてを含む外形を、数字の中心からの相対位置で求める。
    /// </summary>
    /// <param name="popup">表示。数字と「HIT!」の大きさは設定済みであること</param>
    /// <param name="hitOrigin">数字の中心を原点にした、「HIT!」の左下の位置</param>
    /// <param name="burstSize">破裂の一辺（px。弾みの分を含む）</param>
    /// <param name="burstAngle">破裂の向き（ラジアン）</param>
    Bounds MeasureBounds(const Popup& popup, const Vector2& hitOrigin, float burstSize, float burstAngle) const;

    /// <summary>
    /// 文字の矩形に、縁の太さと影のずれを足して、外形へ含める。
    /// </summary>
    /// <param name="bounds">広げる外形</param>
    /// <param name="rectMin">文字の矩形の左上（数字の中心からの相対位置）</param>
    /// <param name="rectMax">文字の矩形の右下（数字の中心からの相対位置）</param>
    /// <param name="fontSize">文字の大きさ（px）。縁と影の大きさの基準</param>
    void IncludeText(Bounds& bounds, const Vector2& rectMin, const Vector2& rectMax, float fontSize) const;

    /// <summary>
    /// 1軸ぶんの位置を、外形が画面の縁の内側に収まる範囲へ寄せる。範囲がないときは中央に揃える。
    /// </summary>
    /// <param name="value">寄せる前の中心の位置</param>
    /// <param name="boundsMin">中心からの外形の下端（px）</param>
    /// <param name="boundsMax">中心からの外形の上端（px）</param>
    /// <param name="screenSize">その軸の画面の大きさ</param>
    float FitAxis(float value, float boundsMin, float boundsMax, float screenSize) const;

    /// <summary>
    /// 外形の全体が画面の内側（screenMarginの内側）に収まるように、数字の中心を寄せる。
    /// 画面より外形が大きいときは、画面の中央に揃える。
    /// </summary>
    /// <param name="target">数字の中心にしたい画面座標</param>
    /// <param name="bounds">数字の中心からの相対位置の外形</param>
    Vector2 FitInsideScreen(const Vector2& target, const Bounds& bounds) const;

    /// <summary>
    /// 数字・「HIT!」・破裂の位置と色を決めて、各表示へ反映する。
    /// </summary>
    /// <param name="popup">表示</param>
    /// <param name="center">数字の中心（画面座標）</param>
    /// <param name="hitOrigin">数字の中心を原点にした、「HIT!」の左下の位置</param>
    /// <param name="burstSize">破裂の一辺（px）</param>
    /// <param name="burstAngle">破裂の向き（ラジアン）</param>
    /// <param name="alpha">消えるときの透明度の倍率</param>
    void PlaceViews(Popup& popup, const Vector2& center, const Vector2& hitOrigin, float burstSize, float burstAngle, float alpha);

private:
    // 調整値（時間・大きさ・弾み）
    int32_t maxPopups_ = 8;                                // 同時に出せる表示の数（上限は kMaxPopups）
    float holdTime_ = 1.0f;                                // 最後に増えてから消え始めるまでの時間（秒）
    float fadeTime_ = 0.3f;                                // 消えるまでの時間（秒）
    float baseSize_ = 36.0f;                               // 数字1つのときの文字の大きさ（px）
    float growPerHit_ = 3.0f;                              // 1つ増えるごとに大きくなる量（px）
    float maxSize_ = 110.0f;                               // 数字の大きさの上限（px）
    float punchScale_ = 1.4f;                              // 増えた瞬間の数字の拡大の倍率
    float punchTime_ = 0.15f;                              // 増えた瞬間の弾みが戻るまでの時間（秒）

    // 調整値（位置関係）
    Vector2 offset_ = { 0.0f, -48.0f };                    // 砕いた敵の画面位置からのずらし（px）
    float hitSizeRatio_ = 0.4f;                            // 「HIT!」の大きさ（数字の大きさに対する割合）
    Vector2 hitOffsetRatio_ = { 0.04f, 0.0f };             // 数字の右下の角から、「HIT!」の左下の角へのずらし（数字の大きさに対する割合）
    float burstSizeRatio_ = 2.2f;                          // 破裂の一辺（数字の大きさに対する割合）
    float burstPunchScale_ = 1.5f;                         // 増えた瞬間の破裂の拡大の倍率
    float burstAngleRange_ = 0.6f;                         // 増えるたびに破裂の向きが変わる範囲（ラジアン。前の向きから±）
    float screenMargin_ = 16.0f;                           // 表示の外側から、画面の縁までの距離（px）

    // 調整値（色・縁・影）
    Vector4 numberColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };     // 数字の色
    Vector4 hitColor_ = { 0.4f, 0.8f, 1.0f, 1.0f };        // 「HIT!」の色（HPアイコンの核と同じ）
    Vector4 outlineColor_ = { 0.05f, 0.08f, 0.2f, 1.0f };  // 縁の色（暗い紺色）
    Vector4 shadowColor_ = { 0.02f, 0.03f, 0.1f, 0.6f };   // 影の色
    Vector4 burstColor_ = { 0.4f, 0.8f, 1.0f, 0.45f };     // 破裂の色（水色の半透明）
    float outlineRatio_ = 0.06f;                           // 縁の太さ（文字の大きさに対する割合）
    Vector2 shadowRatio_ = { 0.1f, 0.1f };                 // 影のずれ（文字の大きさに対する割合。右下）

    // 定数
    static constexpr int32_t kMaxPopups = 8;               // 表示を事前に作る数（maxPopups_の上限）
    static constexpr uint32_t kNumberMaxLength = 6;        // 数字の最大文字数
    static constexpr uint32_t kHitMaxLength = 4;           // 「HIT!」の文字数
    static constexpr uint32_t kRandomSeed = 20261113;      // 破裂の向きの乱数のシード（固定）
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
    inline static const char* kFontName = "源直ゴシック EMG 2 - Medium"; // フォントの登録名
    inline static const char* kHitText = "HIT!";

    // 状態
    std::array<Popup, kMaxPopups> popups_;
    bool isVisible_ = true;                                // 描くか
    std::mt19937 random_{ kRandomSeed };                   // 破裂の向きを変える乱数（シード固定）

    // 参照（所有しない）
    const GameEngine::Camera* camera_ = nullptr;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

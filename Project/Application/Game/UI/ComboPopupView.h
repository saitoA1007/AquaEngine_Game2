#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include "IGameObject.h"
#include "Camera.h"
#include "FontManager.h"
#include "Text.h"
#include "DebugParameter.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// 連鎖数の表示。1回の氷霧から飛んだ氷塊で砕いた数（氷霧で倒した分は数えない）を「HIT 12!」と数え上げる。
/// 表示は1つの連鎖につき1つ。新しく砕けた敵の画面位置へ移って数が増え、数が大きいほど文字が大きくなる。
/// 一定時間増えなければ消える。同時に出せる表示は上限まで。動きはゲームの時間で進める。
/// </summary>
class ComboPopupView : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 表示用の文字を上限の数だけ作る。フォントが見つからなければ例外を投げる。
    /// </summary>
    /// <param name="camera">座標の変換に読むカメラ。シーンが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="fontManager">フォント取得用。コンストラクタの間だけ使い、保持しない。</param>
    ComboPopupView(const GameEngine::Camera* camera, GameEngine::FontManager* fontManager);
    ~ComboPopupView() override = default;

    /// <summary>
    /// 表示の時間を進め、文字の位置・大きさ・色を更新する。増えなくなった表示は消す。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 使用中の表示を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 連鎖で1体砕いたことを伝える。同じ連鎖の表示があれば数を増やして移し、なければ「HIT 1!」から出す。
    /// 表示が上限まで使われているときは、新しい連鎖の表示は出さない。
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
    // 表示1つ
    struct Popup {
        bool isActive = false;                  // 使用中か
        uint32_t chainId = 0;                   // 数えている連鎖のID
        int32_t count = 0;                      // 砕いた数
        Vector2 position = { 0.0f, 0.0f };      // 文字の中心（画面座標）
        float idleTimer = 0.0f;                 // 最後に増えてからの経過時間（秒）
        float punchTimer = 0.0f;                // 増えた瞬間の拡大の経過時間（秒）
        std::unique_ptr<GameEngine::Text> text; // 「HIT N!」
    };

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// ワールド座標を画面座標（1280x720）へ変換する。
    /// </summary>
    /// <param name="worldPosition">ワールド座標</param>
    Vector2 WorldToScreen(const Vector3& worldPosition) const;

    /// <summary>
    /// 表示1つの大きさと色を、数と経過時間から決めて更新する。
    /// </summary>
    void UpdatePopup(Popup& popup, float deltaTime);

private:
    // 調整値
    int32_t maxPopups_ = 8;                                // 同時に出せる表示の数（上限は kMaxPopups）
    float holdTime_ = 1.0f;                                // 最後に増えてから消え始めるまでの時間（秒）
    float fadeTime_ = 0.3f;                                // 消えるまでの時間（秒）
    float baseSize_ = 36.0f;                               // HIT 1!の文字の大きさ（px）
    float growPerHit_ = 3.0f;                              // 1つ増えるごとに大きくなる量（px）
    float maxSize_ = 110.0f;                               // 文字の大きさの上限（px）
    float punchScale_ = 1.4f;                              // 増えた瞬間の拡大の倍率
    float punchTime_ = 0.15f;                              // 増えた瞬間の拡大が戻るまでの時間（秒）
    Vector2 offset_ = { 0.0f, -48.0f };                    // 砕いた敵の画面位置からのずらし（px）
    Vector4 color_ = { 1.0f, 0.95f, 0.55f, 1.0f };         // 文字の色
    float screenMargin_ = 80.0f;                           // 画面の端から、文字の中心を離す距離（px）

    // 定数
    static constexpr int32_t kMaxPopups = 8;               // 文字を事前に作る数（maxPopups_の上限）
    static constexpr float kScreenWidth = 1280.0f;
    static constexpr float kScreenHeight = 720.0f;
    inline static const char* kFontName = "源直ゴシック EMG 2 - Medium"; // フォントの登録名

    // 状態
    std::array<Popup, kMaxPopups> popups_;
    bool isVisible_ = true;                                // 描くか（強化の選択中などはGameSceneが隠す）

    // 参照（所有しない）
    const GameEngine::Camera* camera_ = nullptr;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include "Sprite.h"
#include "Text.h"
#include "FontManager.h"
#include "RenderQueue.h"
#include "Vector2.h"
#include "Vector4.h"
#include <vector>

/// <summary>
/// 強化の選択画面で使う画像のハンドル。GameSceneが取得して渡す。どれも白一色で、色はコードで掛ける。
/// </summary>
struct UpgradeSelectTextures {
    uint32_t white = 0;         // 暗い板に使う白いテクスチャ（white2x2.png）
    uint32_t plate = 0;         // カードの板（UpgradeCardPlate.png。まわりに透明な余白がある。破片もこの画像から切り出す）
    uint32_t frame = 0;         // カードの2重線の枠（UpgradeCardFrame.png。板と同じ大きさ）
    uint32_t light = 0;         // 枠を一周する光。32コマを8列x4行に並べたもの（UpgradeCardLight.png）
    uint32_t divider = 0;       // ひし形のついた区切り線（UpgradeDivider.png）
};

/// <summary>
/// 強化カードの見た目と動きの調整値。UpgradeSelectUIが持ち、DebugParameterへ登録する。カードはこの値を参照して使う。
/// </summary>
struct UpgradeCardStyle {
    // 組み上がり
    float assembleTime = 0.6f;          // 開いてから、最後のカードが組み上がるまでの時間（秒）
    float cardStagger = 0.08f;          // 右のカードが始まるまでの遅れ（秒。1枚ごと）
    float scatterDistance = 160.0f;     // 破片が散らばる距離（画素）
    float scatterRotation = 1.2f;       // 破片が傾く大きさ（ラジアン）
    float shardAngle = 30.0f;           // 破片の格子を傾ける角度（度）
    int32_t shardColumns = 3;           // 破片の格子の列の数
    int32_t shardRows = 4;              // 破片の格子の行の数
    float shardJitter = 0.3f;           // 格子の線を間隔に対してずらす最大の割合（0〜0.45）
    float textFadeTime = 0.25f;         // 組み上がってから、文字が出きるまでの時間（秒）
    float textRiseDistance = 10.0f;     // 文字が出るとき、下から上がってくる距離（画素）

    // 選択中の表示
    float selectedScale = 1.08f;        // 選択中のカードの大きさの倍率
    float lightPeriod = 1.6f;           // 光が枠を一周する時間（秒）

    // 色
    Vector4 plateColor = { 0.1f, 0.15f, 0.2f, 0.9f };
    Vector4 plateSelectedColor = { 0.1f, 0.15f, 0.2f, 0.9f };
    Vector4 frameColor = { 0.55f, 0.6f, 0.65f, 1.0f };
    Vector4 frameSelectedColor = { 0.4f, 0.8f, 1.0f, 1.0f };
    Vector4 lightColor = { 0.8f, 0.95f, 1.0f, 1.0f };
    Vector4 dividerColor = { 0.5f, 0.8f, 1.0f, 0.9f };
    Vector4 textColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    Vector4 shadowColor = { 0.0f, 0.0f, 0.0f, 0.7f };
    Vector2 shadowOffset = { 2.0f, 2.0f };  // 文字の影のずれ（画素）
};

/// <summary>
/// 影付きの文字。同じ文字を暗い色で少しずらして先に描く。位置は中央揃え。
/// </summary>
class ShadowedText {
public:
    /// <summary>
    /// 本体と影の文字を作る。
    /// </summary>
    /// <param name="font">フォント。コンストラクタの間だけ使う。</param>
    /// <param name="text">最初の文字列</param>
    /// <param name="maxLength">文字列の最大の長さ</param>
    /// <param name="fontSize">文字の大きさ（画素）</param>
    ShadowedText(const GameEngine::Font* font, const std::string& text, uint32_t maxLength, float fontSize);

    /// <summary>
    /// 文字列を設定する。
    /// </summary>
    void SetText(const std::string& text);

    /// <summary>
    /// 位置・色・透明度を設定して、描画の準備をする。
    /// </summary>
    /// <param name="position">中央の位置（画面座標）</param>
    /// <param name="style">色と影のずれを取る調整値</param>
    /// <param name="alpha">全体の透明度（0〜1）</param>
    void Update(const Vector2& position, const UpgradeCardStyle& style, float alpha);

    /// <summary>
    /// 影、本体の順に描画キューへ登録する。
    /// </summary>
    void Draw(GameEngine::RenderQueue* renderQueue);

private:
    std::unique_ptr<GameEngine::Text> main_;    // 本体の文字
    std::unique_ptr<GameEngine::Text> shadow_;  // 影の文字
};

/// <summary>
/// 強化カード1枚。板と枠、枠を回る光、組み上がる破片、文字（影付き）を持つ。
/// 開いたときは、散らばった氷の破片が元の位置へ集まって組み上がり、その後に1枚の画像へ切り替わって文字が出る。
/// 動きは止まらない時間で進める。調整値は、所有者（UpgradeSelectUI）のUpgradeCardStyleを参照する。
/// </summary>
class UpgradeCardView {
public:
    /// <summary>
    /// 板・枠・光・破片・文字を作る。
    /// </summary>
    /// <param name="font">フォント。コンストラクタの間だけ使う。</param>
    /// <param name="textures">画像のハンドル</param>
    /// <param name="style">調整値。所有者が管理し、このオブジェクトより長く生存する。</param>
    /// <param name="center">カードの中心（画面座標）</param>
    /// <param name="name">カードの名前</param>
    UpgradeCardView(const GameEngine::Font* font, const UpgradeSelectTextures& textures, const UpgradeCardStyle* style, const Vector2& center, const std::string& name);
    ~UpgradeCardView() = default;

    /// <summary>
    /// 組み上がりを最初からやり直す。
    /// </summary>
    /// <param name="delay">始まるまでの遅れ（秒）</param>
    /// <param name="duration">組み上がるまでの長さ（秒）</param>
    void Start(float delay, float duration);

    /// <summary>
    /// 説明と取得回数の文字列を設定する。
    /// </summary>
    void SetTexts(const std::string& description, const std::string& count);

    /// <summary>
    /// 組み上がりと光を進め、見た目を更新する。
    /// </summary>
    /// <param name="deltaTime">止まらない時間の経過（秒）</param>
    /// <param name="isSelected">選択中か。組み上がるまでは選択中の表示にならない。</param>
    void Update(float deltaTime, bool isSelected);

    /// <summary>
    /// 組み上がり中は破片を、組み上がった後は板と枠と光を、描画キューへ登録する。文字は組み上がった後に出る。
    /// </summary>
    void DrawSprites(GameEngine::RenderQueue* renderQueue);

    /// <summary>
    /// 文字（影つき）を描画キューへ登録する。スプライトより後に呼ぶ。
    /// </summary>
    void DrawTexts(GameEngine::RenderQueue* renderQueue);

private:
    /// <summary>
    /// 破片のスプライトを、最大の数だけ作る。割り方は決めない（BuildShardLayoutが決める）。
    /// </summary>
    void CreateShards(const UpgradeSelectTextures& textures);

    /// <summary>
    /// 板・枠・光のスプライトを作る。
    /// </summary>
    void CreateCardSprites(const UpgradeSelectTextures& textures);

    /// <summary>
    /// 文字を作る。
    /// </summary>
    void CreateTexts(const GameEngine::Font* font, const std::string& name);

    /// <summary>
    /// 調整値が、前に破片を割ったときから変わったかを返す。
    /// </summary>
    bool IsShardLayoutOutdated() const;

    /// <summary>
    /// カードを、傾けた格子で破片に割る。格子の線は、固定したシードの乱数でずらす。カードと重ならない区画には破片を作らない。
    /// 破片の位置・大きさ・UV・散らばる向きと傾きを決める。調整値が変わったときに作り直す。
    /// </summary>
    void BuildShardLayout();

    /// <summary>
    /// 破片1枚の大きさとUVを設定する。UVは、カードの画像の中の、その破片の四角に写る部分を指す。
    /// </summary>
    /// <param name="index">破片の番号</param>
    void ApplyShardPiece(int32_t index);

    /// <summary>
    /// 散らばった破片を、進み具合に応じて元の位置へ寄せる。
    /// </summary>
    /// <param name="progress">組み上がりの進み具合（0〜1）</param>
    void UpdateShards(float progress);

    /// <summary>
    /// 1枚に組み上がった板・枠・光の色と大きさを更新する。
    /// </summary>
    void UpdateCardSprites(bool isSelected);

    /// <summary>
    /// 光のコマを、光の経過時間から選んでUVを動かす。
    /// </summary>
    void UpdateLightFrame();

    /// <summary>
    /// 文字の位置と透明度を更新する。
    /// </summary>
    /// <param name="textAlpha">文字の透明度（0〜1）</param>
    void UpdateTexts(float textAlpha);


private:
    /// <summary>
    /// 破片1枚の割れ方。Updateで位置を動かすために持つ。
    /// </summary>
    struct ShardPiece {
        bool isActive = false;          // 使っているか（カードと重ならない区画は使わない）
        Vector2 offset{};               // 組み上がったときの、カード中心から破片の中心への距離（画素）
        Vector2 size{};                 // 格子の区画の大きさ（画素）
        Vector2 scatterDirection{};     // 散らばる向き（長さは距離の割合）
        float scatterRotation = 0.0f;   // 散らばったときの傾きの向きと大きさ（-1〜1）
    };

private:
    // 定数
    static constexpr int32_t kMaxShardColumns = 6;      // 格子の列の数の上限（破片のスプライトの数を決める）
    static constexpr int32_t kMaxShardRows = 6;
    static constexpr int32_t kMaxShards = kMaxShardColumns * kMaxShardRows;
    static constexpr uint32_t kShardSeed = 20261110;    // 格子のずれと散らばりを決める乱数のシード（開くたびに同じ動きにする）
    static constexpr float kScatterAngleSpread = 0.6f;  // 散らばる向きを、中心から外への向きからずらす最大（ラジアン）
    static constexpr float kScatterMinRatio = 0.6f;     // 散らばる距離の最小の割合
    static constexpr float kMaxShardJitter = 0.45f;     // 格子の線のずれの上限。線どうしが交差しないための値
    inline static const Vector2 kCardSize = { 300.0f, 380.0f };       // カードの見た目の大きさ
    inline static const Vector2 kImageSize = { 760.0f, 760.0f };      // 板と枠の画像の大きさ（カードのまわりに透明な余白がある。縦横は同じ）
    inline static const Vector2 kLightCellSize = { 320.0f, 400.0f };  // 光の1コマの大きさ（カード+余白）
    static constexpr int32_t kLightFrames = 32;
    static constexpr int32_t kLightColumns = 8;
    static constexpr int32_t kLightRows = 4;
    static constexpr float kNameOffsetY = -100.0f;      // カード中心から文字までの縦の距離
    static constexpr float kDescriptionOffsetY = 0.0f;
    static constexpr float kCountOffsetY = 120.0f;

    // 参照（所有しない）
    const UpgradeCardStyle* style_ = nullptr;

    // 状態
    Vector2 center_{};            // カードの中心（画面座標）
    float timer_ = 0.0f;          // 開始からの経過（秒）。遅れの間は負
    float duration_ = 0.6f;       // 組み上がるまでの長さ（秒）
    float lightTimer_ = 0.0f;     // 選択されてからの経過（秒）
    bool wasSelected_ = false;    // 前の更新で選択中だったか
    bool isAssembled_ = false;    // 1枚の画像へ切り替わったか

    // 描画
    std::unique_ptr<GameEngine::Sprite> plateSprite_;
    std::unique_ptr<GameEngine::Sprite> frameSprite_;
    std::unique_ptr<GameEngine::Sprite> lightSprite_;
    std::array<std::unique_ptr<GameEngine::Sprite>, kMaxShards> plateShards_;   // 板の破片
    std::array<std::unique_ptr<GameEngine::Sprite>, kMaxShards> frameShards_;   // 枠の破片
    std::array<ShardPiece, kMaxShards> shardPieces_{};      // 破片の割れ方
    float builtShardAngle_ = 0.0f;       // 破片を割ったときの調整値（変わったら割り直す）
    int32_t builtShardColumns_ = 0;
    int32_t builtShardRows_ = 0;
    float builtShardJitter_ = 0.0f;
    float shardAngleRadians_ = 0.0f;     // 破片の格子の角度（ラジアン）
    std::unique_ptr<ShadowedText> nameText_;
    std::unique_ptr<ShadowedText> descriptionText_;
    std::unique_ptr<ShadowedText> countText_;
    float textAlpha_ = 0.0f;      // 文字の透明度
};

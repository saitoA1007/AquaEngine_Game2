#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "Model.h"
#include "RefBuffer.h"
#include "Material.h"
#include "WorldTransform.h"
#include "WorldTransforms.h"
#include "RenderQueue.h"
#include "DebugParameter.h"
#include "Matrix4x4.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// 氷エリアの描画に使うモデルとテクスチャ。GameSceneが取得して渡す。モデルは管理クラスが所有する。
/// </summary>
struct IceAreaAssets {
    GameEngine::Model* slab = nullptr;   // 角を落とした薄い円柱（iceAreaSlab.gltf。半径1、高さ1、底がY=0。上面と側面のUVが板のテクスチャに合う）
    GameEngine::Model* plane = nullptr;  // 光の帯・結晶・ハイライトに使う、XZ平面の板（planeXZ.obj）
    uint32_t slabTexture = 0;            // 板の絵（iceAreaSlabStylized.png。上の正方形が上面、下の帯が側面）
    uint32_t slabHighlight = 0;          // 板の左上のハイライト（iceAreaSlabHighlight.png）
    uint32_t rimLine = 0;                // 縁の光の帯（iceAreaRimLine.png）
    uint32_t snowflake = 0;              // 雪の結晶（iceAreaSnowflake.png）
};

/// <summary>
/// 氷エリアの見た目。様式化した絵の板（光を使わない）の上に、光る縁と雪の結晶・ハイライト（加算）を重ねる。
/// PBRやレイトレーシングの氷の見た目からは外れ、設計時の概念図に合わせた絵作りにしている。
/// 縁の太さ・結晶の大きさ・板の厚みは半径によらず一定で、円周が長くなると帯と結晶の数が増える。
/// 氷霧を出した瞬間は、発動位置から光の輪が広がって薄くなり、その間は縁が明るくなる。
/// Draw(中心, 半径)の形を保てば、このクラスごと差し替えられる。
/// </summary>
class IceAreaView {
public:
    /// <summary>
    /// 板の材質と、光の帯・結晶・ハイライトのインスタンス領域を作り、調整値を登録する。モデルが足りなければ例外を投げる。
    /// </summary>
    explicit IceAreaView(const IceAreaAssets& assets);

    /// <summary>
    /// 調整値を反映し、結晶の回転と発動時の光の時間を進める。
    /// </summary>
    /// <param name="deltaTime">ゲームの経過時間（秒）</param>
    void Update(float deltaTime);

    /// <summary>
    /// 氷霧を出した瞬間の光の輪を始める。中心はその場に固定され、プレイヤーが動いても追わない。
    /// </summary>
    /// <param name="center">発動位置（XZを使う）</param>
    /// <param name="radius">発動時の氷エリアの半径。光の輪はここから外へ広がる</param>
    void PlayBurst(const Vector3& center, float radius);

    /// <summary>
    /// 板、ハイライト、縁、結晶、発動時の光の輪を描画キューへ登録する。
    /// </summary>
    /// <param name="renderQueue">登録先</param>
    /// <param name="center">氷エリアの中心（XZを使う）</param>
    /// <param name="radius">氷エリアの半径</param>
    void Draw(GameEngine::RenderQueue* renderQueue, const Vector3& center, float radius);

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

    /// <summary>
    /// 板のモデルのメッシュごとに、光を使わない材質と参照バッファを作る。
    /// </summary>
    void CreateSlabBuffers();

    /// <summary>
    /// 光の帯・結晶・ハイライトのインスタンス領域を、上限の数だけ確保する。
    /// </summary>
    void CreateInstances();

    /// <summary>
    /// 板を、半径に合わせて広げて描画キューへ登録する。絵は引き伸ばされる。
    /// </summary>
    void DrawSlab(GameEngine::RenderQueue* renderQueue, const Vector3& center, float radius);

    /// <summary>
    /// 板の左上（画面で左・奥）に白い楕円のハイライトを置く。大きさと位置は半径に比例する。
    /// </summary>
    void DrawHighlight(GameEngine::RenderQueue* renderQueue, const Vector3& center, float radius);

    /// <summary>
    /// 縁の太い光の帯と、その内側の細い白い線を円周に沿って並べ、1回のインスタンス描画で登録する。発動中は縁を明るくする。
    /// </summary>
    void DrawEdge(GameEngine::RenderQueue* renderQueue, const Vector3& center, float radius);

    /// <summary>
    /// 雪の結晶を縁の内側に大小交互に並べ、全体を回して、1回のインスタンス描画で登録する。
    /// </summary>
    void DrawFlakes(GameEngine::RenderQueue* renderQueue, const Vector3& center, float radius);

    /// <summary>
    /// 発動時の光の輪を、広がる半径と薄くなる明るさで登録する。時間が終わっていれば何もしない。
    /// </summary>
    void DrawBurst(GameEngine::RenderQueue* renderQueue);

    /// <summary>
    /// 円周に沿って光の帯を並べる。帯どうしは少し重なり、重なりの合計が一定になる。
    /// </summary>
    /// <param name="transforms">書き込み先</param>
    /// <param name="startIndex">書き込みを始める位置</param>
    /// <param name="center">円の中心</param>
    /// <param name="ringRadius">帯の中心を通る円の半径</param>
    /// <param name="width">帯の太さ</param>
    /// <param name="y">帯を置く高さ</param>
    /// <param name="color">帯の色（透明度はw）</param>
    /// <returns>並べた帯の数</returns>
    uint32_t FillEdgeBand(GameEngine::WorldTransforms& transforms, uint32_t startIndex, const Vector3& center, float ringRadius, float width, float y, const Vector4& color) const;

    /// <summary>
    /// 帯・結晶の板1枚の行列を作る。板の長辺をtangentの向きに合わせ、XZ平面に寝かせる。
    /// </summary>
    /// <param name="position">板の中心</param>
    /// <param name="tangent">長辺の向き（XZ、長さ1）</param>
    /// <param name="halfLength">長辺の半分の長さ</param>
    /// <param name="halfWidth">短辺の半分の長さ</param>
    static Matrix4x4 MakePlateMatrix(const Vector3& position, const Vector3& tangent, float halfLength, float halfWidth);

private:
    // 調整値（板）
    float slabThickness_ = 0.2f;                         // 板の厚み（ワールド単位。半径によらず一定。側面の暗い帯が見える）
    Vector4 slabColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };     // 板の絵に掛ける色（白で絵のまま）
    float highlightStrength_ = 0.5f;                     // ハイライトの濃さ（0で消える）
    float highlightSize_ = 0.3f;                         // ハイライトの長辺の半分の長さ（半径に対する割合）
    Vector3 highlightOffset_ = { -0.4f, 0.0f, 0.4f };    // ハイライトの中心の位置（半径に対する割合。Xが負で左、Zが正で奥）

    // 調整値（縁）
    float edgeWidth_ = 0.45f;                            // 縁の光の帯の太さ（ワールド単位。半径によらず一定）
    float edgeSegmentLength_ = 1.0f;                     // 帯1枚の長さ（円周をこの長さ付近で等分する）
    Vector4 edgeColor_ = { 0.0f, 0.5f, 1.0f, 0.8f };     // 縁の色。加算なので、板が明るくても水色が残るよう赤を0にしている
    float edgeBrightness_ = 1.0f;                        // 縁の明るさ（1で色のまま。上げすぎると白く飛ぶ）
    float edgeLineWidth_ = 0.06f;                        // 縁の内側の細い白い線の太さ
    Vector4 edgeLineColor_ = { 1.0f, 1.0f, 1.0f, 0.9f }; // 細い白い線の色
    float layerLift_ = 0.02f;                            // 加算の板を板の上面から浮かせる高さ（重なりのちらつき防止）

    // 調整値（結晶）
    float flakeSize_ = 0.9f;                             // 大きい結晶1個の大きさ（ワールド単位。半径によらず一定）
    float flakeSmallRatio_ = 0.6f;                       // 小さい結晶の大きさの割合（大小交互）
    float flakeSpacing_ = 1.3f;                          // 結晶の間隔（円周をこの間隔付近で等分する）
    float flakeInset_ = 0.75f;                           // 結晶の中心を、氷エリアの縁から内側へ寄せる距離（縁の帯に埋もれない距離）
    float flakeSpinSpeed_ = 0.25f;                       // 結晶の並び全体が回る速さ（ラジアン/秒）
    Vector4 flakeColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };    // 結晶の色
    float flakeBrightness_ = 1.0f;                       // 結晶の明るさ

    // 調整値（発動時の光）
    float burstTime_ = 0.4f;                             // 光の輪が広がって消えるまでの時間（秒）
    float burstSpread_ = 3.0f;                           // 光の輪が広がる距離（ワールド単位）
    float burstBrightness_ = 1.5f;                       // 光の輪の最初の明るさ
    float edgeFlashBoost_ = 0.8f;                        // 発動の瞬間に縁へ足す明るさの倍率（時間とともに0へ戻る）

    // 定数
    static constexpr uint32_t kMaxEdgeSegments = 768;    // 縁の帯の数の上限（太い帯と細い線は別に数える）
    static constexpr uint32_t kMaxFlakes = 384;          // 結晶の数の上限
    static constexpr uint32_t kMinCount = 4;             // 帯・結晶の数の下限（結晶は偶数にそろえる）
    static constexpr float kEdgeOverlap = 1.25f;         // 帯の長さを、円周の等分よりのばす倍率（帯の端の薄い部分で重なる）
    static constexpr float kFlakeLift = 0.01f;           // 結晶を縁の帯よりさらに浮かせる高さ
    static constexpr float kBurstLift = 0.02f;           // 光の輪を縁の帯よりさらに浮かせる高さ
    static constexpr float kHighlightAngle = -0.5f;      // ハイライトの楕円の傾き（ラジアン）
    static constexpr float kHighlightAspect = 0.5f;      // ハイライトの短辺の半分 / 長辺の半分
    inline static const char* kDebugName = "IceAreaView";
    inline static const char* kAddPassName = "WBOITAccumulatePass";

    // 参照（所有しない）
    IceAreaAssets assets_;

    // 状態
    float spinAngle_ = 0.0f;                             // 結晶の並び全体の回転（ラジアン）
    float burstTimer_ = 0.0f;                            // 光の輪の残り時間（秒）。0なら出ていない
    Vector3 burstCenter_ = { 0.0f, 0.0f, 0.0f };         // 光の輪の中心
    float burstStartRadius_ = 0.0f;                      // 光の輪の最初の半径

    // 描画
    GameEngine::WorldTransform slabTransform_;
    GameEngine::Material slabMaterial_;                  // 光を使わない板の材質（絵はテクスチャ）
    std::vector<GameEngine::RefBuffer> slabRefBuffers_;
    GameEngine::WorldTransforms highlightTransforms_;
    GameEngine::WorldTransforms edgeTransforms_;
    GameEngine::WorldTransforms flakeTransforms_;
    GameEngine::WorldTransforms burstTransforms_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

#pragma once
#include <memory>
#include "IGameObject.h"
#include "Model.h"
#include "TextureManager.h"
#include "ParticleBehavior.h"
#include "Vector3.h"

/// <summary>
/// 氷霧を発生させたときの見た目（試作）。氷エリア全体から、白い冷気が真上へ舞い上がる。
/// エンジンのParticleBehaviorを使い、見た目の設定はResources/Json/GameData/IceMistParticle.jsonで決める。
/// Play(中心, 半径)の形を保てば、別の見た目へ差し替えられる。
/// </summary>
class IceMistEffect : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 冷気のパーティクルを作る。
    /// </summary>
    /// <param name="planeModel">パーティクル1枚の板に使うモデル（"plane.obj"）。管理クラスが所有し、このオブジェクトより長く生存する。</param>
    /// <param name="textureManager">パーティクルのテクスチャ取得用。コンストラクタの間だけ使う。</param>
    IceMistEffect(GameEngine::Model* planeModel, GameEngine::TextureManager* textureManager);

    /// <summary>
    /// パーティクルを更新する。
    /// </summary>
    void Update() override;

    /// <summary>
    /// パーティクルを描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 氷霧の見た目を発生させる。
    /// </summary>
    /// <param name="center">氷霧の中心（XZを使う）</param>
    /// <param name="radius">氷霧の半径（この円の中から冷気が立ち上がる）</param>
    void Play(const Vector3& center, float radius);

private:
    // 定数
    static constexpr uint32_t kMaxParticles = 256; // 同時に存在できるパーティクルの数
    static constexpr float kEmitHeight = 0.1f;     // 冷気を出す高さ（地面の少し上）
    inline static const char* kParticleName = "IceMistParticle"; // 設定のJSONの名前

    // 状態
    std::unique_ptr<GameEngine::ParticleBehavior> particle_;
};

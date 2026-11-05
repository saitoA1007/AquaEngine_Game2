#pragma once
#include <memory>
#include "IGameObject.h"
#include "ModelComponent.h"
#include "DebugParameter.h"
#include "Vector3.h"
#include "Vector4.h"

/// <summary>
/// 円形のフィールド。地面と端の縁を描画し、円内への押し戻しを提供する。
/// 中心はワールド原点（XZ平面）。
/// </summary>
class Field : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 地面と縁のモデルを受け取り、調整値を登録する。
    /// </summary>
    /// <param name="cylinderModel">登録済みの"Cylinder"モデル。管理クラスが所有し、このオブジェクトより長く生存する。</param>
    Field(GameEngine::Model* cylinderModel);
    ~Field() override = default;

    void Initialize() override;

    /// <summary>
    /// 調整値の反映と、円盤の拡大率・位置の更新を行う。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 地面と縁を描画キューへ登録する。
    /// </summary>
    void Draw() override;

    /// <summary>
    /// 位置を円内へ押し戻す。外へ出る分だけ円周に沿って滑る。
    /// </summary>
    /// <param name="position">補正前の位置</param>
    /// <param name="margin">端から内側へ空ける距離（対象の半径）</param>
    /// <returns>円内に収めた位置（Y座標は変えない）</returns>
    Vector3 ClampToField(const Vector3& position, float margin) const;

    /// <summary>
    /// フィールドの半径を取得する。
    /// </summary>
    float GetRadius() const { return radius_; }

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

private:
    // 調整値
    float radius_ = 50.0f;         // フィールドの半径
    float rimWidth_ = 2.0f;        // 縁の幅（外側へ張り出す距離）
    float groundThickness_ = 1.0f; // 地面の厚み

    // 色
    Vector4 groundColor_ = { 0.85f, 0.92f, 1.0f, 1.0f }; // 雪の地面（仮）
    Vector4 rimColor_ = { 0.15f, 0.35f, 0.7f, 1.0f };    // 縁（仮）

    // 描画
    GameEngine::ModelComponent ground_;
    GameEngine::ModelComponent rim_;

    // 調整値の機能
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

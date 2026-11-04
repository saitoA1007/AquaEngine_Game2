#pragma once
#include "IGameObject.h"
#include "InputCommand.h"
#include "Model.h"
#include "WorldTransform.h"
#include "FractureInstance.h"
#include "IceMaterial.h"

/// <summary>
/// Bキーで事前分割された全破片を同時に飛ばすテストオブジェクト。
/// </summary>
class CellFractureTest : public GameEngine::IGameObject {
public:
    /// <summary>
    /// 管理クラスが所有するモデルと入力機能を参照する。
    /// </summary>
    /// <param name="model">単一の破壊グループを持つテスト用モデル。</param>
    /// <param name="inputCommand">破壊操作を登録した入力機能。</param>
    CellFractureTest(GameEngine::Model* model, GameEngine::InputCommand* inputCommand);

    /// <summary>
    /// 共有モデルの材質参照を戻してから、個体の材質を解放する。
    /// </summary>
    ~CellFractureTest() override;

    /// <summary>
    /// 全破片を元の配置へ戻し、破壊前の状態を準備する。
    /// </summary>
    void Initialize() override;

    /// <summary>
    /// 破壊入力と、破壊後の各破片の直線運動を更新する。
    /// </summary>
    void Update() override;

    /// <summary>
    /// 全破片を既存の破片描画へ登録する。
    /// </summary>
    void Draw() override;

public:
    // 調整値
    float minLaunchSpeed_ = 3.0f; // 破片の最低速度（ワールド単位/秒）
    float maxLaunchSpeed_ = 7.0f; // 破片の最高速度（ワールド単位/秒）

private:
    // 管理クラスが所有し、このオブジェクトより長く生存する参照
    GameEngine::Model* model_ = nullptr;
    GameEngine::InputCommand* inputCommand_ = nullptr;

    // 描画と破壊状態
    GameEngine::WorldTransform worldTransform_;
    GameEngine::IceMaterial iceMaterial_;
    GameEngine::FractureInstance fractureInstance_;
    bool isBroken_ = false;

private:
    /// <summary>
    /// 全破片へランダムな方向と速さの初速度を一度だけ与える。
    /// </summary>
    void Break();
};

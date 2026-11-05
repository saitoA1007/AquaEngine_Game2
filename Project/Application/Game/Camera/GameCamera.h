#pragma once
#include <memory>
#include "Camera.h"
#include "DebugParameter.h"
#include "Vector3.h"

/// <summary>
/// このゲーム専用のカメラ。ターゲットを追従する固定角度の見下ろしカメラ。
/// 調整値と指数補間を持ち、エンジンのCameraを所有する。
/// </summary>
class GameCamera {
public:
    /// <summary>
    /// エンジンのCameraと調整値を準備する。
    /// </summary>
    /// <param name="clientWidth">画面の横幅</param>
    /// <param name="clientHeight">画面の縦幅</param>
    GameCamera(int clientWidth, int clientHeight);
    ~GameCamera() = default;

    /// <summary>
    /// ターゲット位置へ補間しながら追従し、Cameraを更新する。初回の呼び出しは補間せず目標位置へ合わせる。
    /// </summary>
    /// <param name="targetPosition">追従するターゲットのワールド位置</param>
    void Update(const Vector3& targetPosition);

    /// <summary>
    /// 描画に登録するエンジンのCameraを取得する。所有はGameCamera。
    /// </summary>
    GameEngine::Camera* GetCamera() { return camera_.get(); }

private:
    /// <summary>
    /// 調整値をDebugParameterへ登録する。
    /// </summary>
    void RegisterDebugParameters();

private:
    // 調整値
    float distance_ = 25.0f;   // ターゲットからの距離
    float pitchDeg_ = 60.0f;   // 見下ろす角度（度）。90で真上
    float followRate_ = 8.0f;  // 追従の速さ（1/秒）。大きいほど早く追いつく

    // 状態
    Vector3 position_ = { 0.0f, 0.0f, 0.0f }; // カメラの現在位置
    bool isInitialized_ = false;               // 初回の位置合わせが済んだか

    // エンジン機能
    std::unique_ptr<GameEngine::Camera> camera_;
    std::unique_ptr<GameEngine::DebugParameter> debugParam_;
};

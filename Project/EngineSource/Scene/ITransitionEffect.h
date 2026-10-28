#pragma once

class ITransitionEffect {
public:

    virtual ~ITransitionEffect() = default;

    /// <summary>
    /// 初期化
    /// </summary>
    virtual void Initialize() = 0;

    /// <summary>
    /// 更新処理(0 ~ 1)の範囲を受け取る
    /// </summary>
    virtual void Update(float timer) = 0;

    /// <summary>
    /// 描画処理
    /// </summary>
    virtual void Draw() = 0;

    /// <summary>
    /// 中間地点に到達したか
    /// </summary>
    virtual bool IsMidTransition(float timer) const = 0;

    virtual float GetMaxTime() = 0;
};
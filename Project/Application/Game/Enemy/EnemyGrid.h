#pragma once
#include <vector>
#include "Vector3.h"
#include "Application/Game/Enemy/Enemy.h"

/// <summary>
/// 敵の位置を入れる均一グリッド。毎フレーム作り直し、範囲検索を提供する。
/// 凍結の壁の判定のほか、氷霧と連鎖の範囲検索にも使う想定。
/// </summary>
class EnemyGrid {
public:
    /// <summary>
    /// グリッドの範囲とセルの大きさを決める。範囲の外の敵は、端のセルに入れる。
    /// </summary>
    /// <param name="halfExtent">原点から端までの距離（フィールド半径より少し大きくする）</param>
    /// <param name="cellSize">1セルの一辺。検索半径と同程度にすると効率がよい</param>
    void Initialize(float halfExtent, float cellSize);

    /// <summary>
    /// 敵の現在位置でグリッドを作り直す。敵のポインタはRebuildの次の呼び出しまで有効である必要がある。
    /// </summary>
    /// <param name="enemies">入れる敵（使用中のもの）</param>
    void Rebuild(const std::vector<Enemy*>& enemies);

    /// <summary>
    /// 中心から半径以内（中心どうしの距離）にいる敵を集める。
    /// </summary>
    /// <param name="center">検索の中心（XZを使う）</param>
    /// <param name="radius">検索の半径</param>
    /// <param name="out">結果。呼び出しのたびに空にしてから入れる</param>
    void Query(const Vector3& center, float radius, std::vector<Enemy*>& out) const;

private:
    /// <summary>
    /// 座標をセルの列・行へ変換する。範囲外は端へ収める。
    /// </summary>
    int32_t ToCellCoord(float value) const;

private:
    // 設定
    float halfExtent_ = 50.0f; // 原点から端までの距離
    float cellSize_ = 2.0f;    // 1セルの一辺
    int32_t cellCount_ = 1;    // 一辺のセル数

    // 状態（毎フレーム作り直す）
    std::vector<int32_t> cellStart_;  // セルごとの先頭位置（セル数+1個）
    std::vector<int32_t> cursor_;     // 並べ替え中の書き込み位置
    std::vector<int32_t> entryCell_;  // 敵ごとのセル番号
    std::vector<Enemy*> sorted_;      // セル順に並べた敵
};

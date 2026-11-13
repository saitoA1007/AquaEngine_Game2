#pragma once
#ifdef USE_IMGUI
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include "Application/Game/Enemy/EnemyDefinition.h"
#include "Application/Game/Phase/PhaseTable.h"

/// <summary>
/// フェーズ表を表示・編集・保存するImGui画面。USE_IMGUIのときだけ存在する。
/// 編集は読み込み済みの表とは別の写しに対して行い、保存すると通知で現在の表へすぐ反映する。
/// </summary>
class PhaseTableEditor {
public:
    /// <summary>
    /// 保存先のJSONから編集用の写しを読み込む。
    /// </summary>
    /// <param name="filePath">フェーズ表のJSON</param>
    /// <param name="definitions">重みを並べる敵一覧と、検証に使う。コピーして保持する。</param>
    PhaseTableEditor(const std::string& filePath, const std::vector<EnemyDefinition>& definitions);

    /// <summary>
    /// 保存に成功したときに呼ぶ関数を設定する。
    /// </summary>
    /// <param name="callback">保存した表を受け取る関数。呼び出し先がこのオブジェクトより長く生存する必要がある。</param>
    void SetSavedCallback(const std::function<void(const PhaseTable&)>& callback) { savedCallback_ = callback; }

    /// <summary>
    /// 指定したフェーズへ飛ぶボタンを押したときに呼ぶ関数を設定する。
    /// </summary>
    /// <param name="callback">フェーズ番号（1から）を受け取る関数。呼び出し先がこのオブジェクトより長く生存する必要がある。</param>
    void SetJumpCallback(const std::function<void(int32_t)>& callback) { jumpCallback_ = callback; }

    /// <summary>
    /// 編集画面を描画する。現在の状態の表示、フェーズの選択と編集、保存、飛び先の指定ができる。
    /// </summary>
    /// <param name="exp">現在のフェーズで溜めた経験値</param>
    /// <param name="phase">現在のフェーズ</param>
    void Draw(int32_t exp, int32_t phase);

private:
    /// <summary>
    /// 選んだ1行分の項目を編集する。
    /// </summary>
    /// <param name="row">編集する行</param>
    void DrawRow(PhaseRow& row);

    /// <summary>
    /// 検証して保存し、成功したら通知する。結果をmessage_へ入れる。
    /// </summary>
    void Save();

    /// <summary>
    /// ファイルから読み直す。結果をmessage_へ入れる。
    /// </summary>
    void Reload();

private:
    std::string filePath_;                          // 保存先
    std::vector<EnemyDefinition> definitions_;      // 敵一覧の写し
    PhaseTable editing_;                            // 編集中の写し
    int32_t selectedPhase_ = 1;                     // 編集中のフェーズ番号（1から）
    std::string message_;                           // 画面に出す結果の文
    std::function<void(const PhaseTable&)> savedCallback_; // 保存の通知先
    std::function<void(int32_t)> jumpCallback_;     // フェーズ移動の通知先
};
#endif

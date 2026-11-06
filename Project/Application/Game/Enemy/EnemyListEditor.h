#pragma once
#ifdef USE_IMGUI
#include <string>
#include <vector>
#include "Application/Game/Enemy/EnemyDefinition.h"

/// <summary>
/// 敵一覧を表示・編集・保存するImGui画面。USE_IMGUIのときだけ存在する。
/// 編集は読み込み済みの一覧とは別の写しに対して行い、保存したJSONは次回の読み込みから反映される。
/// </summary>
class EnemyListEditor {
public:
    /// <summary>
    /// 保存先のJSONから編集用の写しを読み込む。
    /// </summary>
    /// <param name="filePath">敵一覧のJSON</param>
    explicit EnemyListEditor(const std::string& filePath);

    /// <summary>
    /// 編集画面を描画する。行の追加と削除、保存、読み直しができる。
    /// </summary>
    void Draw();

private:
    /// <summary>
    /// 1行分の項目を編集する。
    /// </summary>
    /// <param name="definition">編集する行</param>
    void DrawRow(EnemyDefinition& definition);

    /// <summary>
    /// 検証して保存する。結果をmessage_へ入れる。
    /// </summary>
    void Save();

    /// <summary>
    /// ファイルから読み直す。結果をmessage_へ入れる。
    /// </summary>
    void Reload();

private:
    std::string filePath_;                  // 保存先
    std::vector<EnemyDefinition> editing_;  // 編集中の写し
    std::string message_;                   // 画面に出す結果の文
};
#endif

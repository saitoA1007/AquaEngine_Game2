#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Application/Game/Enemy/EnemyDefinition.h"

/// <summary>
/// フェーズの種類。ボーナスは一発で凍る敵を多く出すフェーズ、ボスはボス戦のフェーズ。
/// </summary>
enum class PhaseType {
    kNormal,
    kBonus,
    kBoss,
};

/// <summary>
/// フェーズ表の1行。1つのフェーズの経験値と出現のルールを持つ。
/// </summary>
struct PhaseRow {
    PhaseType type = PhaseType::kNormal;       // フェーズの種類
    int32_t expToNext = 10;                    // 次のフェーズまでに必要な経験値（ボスでは使わない）
    float spawnInterval = 1.0f;                // スポーンの間隔（秒）
    int32_t spawnPerInterval = 3;              // 1回のスポーンで出そうとする数
    int32_t maxEnemies = 60;                   // フェーズ全体で同時に存在できる敵の最大数
    std::map<std::string, float> weights;      // 敵のIDごとの出現の重み（書いていない種類は0）
};

/// <summary>
/// フェーズ表。行の番号 + 1がそのフェーズの番号になる。
/// </summary>
struct PhaseTable {
    std::vector<PhaseRow> phases;
};

/// <summary>
/// フェーズ表のJSONの読み書きと検証。
/// </summary>
namespace PhaseTableIO {

    // フェーズ表の保存先（作業ディレクトリはProject）
    inline const std::string kPhaseTablePath = "Resources/Json/PhaseData/PhaseTable.json";

    // フェーズ表の行数（フェーズの上限）
    inline constexpr int32_t kPhaseCount = 10;

    /// <summary>
    /// フェーズの種類を、JSONに書く名前へ変換する。
    /// </summary>
    /// <param name="type">フェーズの種類</param>
    const char* ToString(PhaseType type);

    /// <summary>
    /// JSONに書かれた名前をフェーズの種類へ変換する。
    /// </summary>
    /// <param name="name">"Normal" / "Bonus" / "Boss"</param>
    /// <param name="type">変換できたときの結果</param>
    /// <returns>変換できればtrue</returns>
    bool FromString(const std::string& name, PhaseType& type);

    /// <summary>
    /// フェーズ表の内容を検証する。
    /// </summary>
    /// <param name="table">検証する表</param>
    /// <param name="definitions">重みのIDの存在確認に使う敵一覧</param>
    /// <param name="error">問題があるとき、その内容が入る</param>
    /// <returns>問題がなければtrue</returns>
    bool Validate(const PhaseTable& table, const std::vector<EnemyDefinition>& definitions, std::string& error);

    /// <summary>
    /// JSONからフェーズ表を読み込む。ファイルがない、または検証に失敗したときは例外を投げる。
    /// </summary>
    /// <param name="filePath">読み込むファイル</param>
    /// <param name="definitions">重みのIDの存在確認に使う敵一覧</param>
    PhaseTable Load(const std::string& filePath, const std::vector<EnemyDefinition>& definitions);

    /// <summary>
    /// フェーズ表をJSONへ保存する。
    /// </summary>
    /// <param name="filePath">保存先</param>
    /// <param name="table">保存する表</param>
    void Save(const std::string& filePath, const PhaseTable& table);
}

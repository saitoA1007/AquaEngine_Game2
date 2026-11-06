#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Vector4.h"

/// <summary>
/// 敵一覧の1行。種類ごとの固定値を持つ。個体の状態は持たない。
/// </summary>
struct EnemyDefinition {
    std::string id;                          // 一覧内で重複しない識別名
    std::string displayName;                 // 表示名
    std::string kind = "Chase";              // 派生クラスの種類
    std::string modelName = "sphere.obj";    // 見た目に使うモデル名
    float visualScale = 0.4f;                // 見た目の大きさ（球の半径）
    float radius = 0.4f;                     // 当たりの半径
    float moveSpeed = 3.0f;                  // 移動速度（ワールド単位/秒）
    float iceThreshold = 1.5f;               // 氷ゲージの閾値。この値に達すると凍る
    int32_t contactDamage = 1;               // 接触ダメージ
    int32_t exp = 1;                         // 倒したときの経験値（B3以降で使う）
    int32_t iceShardCount = 1;               // 倒したときに落とす氷塊の数（B4で使う）
    int32_t breakCount = 1;                  // 倒すのに必要な砕く回数（B3以降で使う）
    int32_t poolSize = 100;                  // 事前に確保する個体数（同時に存在できる上限）
    Vector4 color = { 1.0f, 0.5f, 0.1f, 1.0f }; // 種類ごとの色（オレンジ系統）
    std::map<std::string, float> extras;     // 種類固有の追加の値（名前と数値の組）
};

/// <summary>
/// 敵一覧のJSONの読み書きと検証。
/// </summary>
namespace EnemyDefinitionIO {

    // 敵一覧の保存先（作業ディレクトリはProject）
    inline const std::string kEnemyListPath = "Resources/Json/EnemyData/EnemyList.json";

    /// <summary>
    /// 実装されている派生クラスの種類名を返す。
    /// </summary>
    const std::vector<std::string>& GetKnownKinds();

    /// <summary>
    /// 種類が必須とする追加の値の名前を返す。
    /// </summary>
    /// <param name="kind">派生クラスの種類名</param>
    std::vector<std::string> GetRequiredExtraNames(const std::string& kind);

    /// <summary>
    /// 一覧の内容を検証する。
    /// </summary>
    /// <param name="definitions">検証する一覧</param>
    /// <param name="error">問題があるとき、その内容が入る</param>
    /// <returns>問題がなければtrue</returns>
    bool Validate(const std::vector<EnemyDefinition>& definitions, std::string& error);

    /// <summary>
    /// JSONから敵一覧を読み込む。ファイルがない、または検証に失敗したときは例外を投げる。
    /// </summary>
    /// <param name="filePath">読み込むファイル</param>
    std::vector<EnemyDefinition> Load(const std::string& filePath);

    /// <summary>
    /// 敵一覧をJSONへ保存する。
    /// </summary>
    /// <param name="filePath">保存先</param>
    /// <param name="definitions">保存する一覧</param>
    void Save(const std::string& filePath, const std::vector<EnemyDefinition>& definitions);
}

#include "pch.h"
#include "PhaseTable.h"
#include "JsonSerializer.h"
#include <stdexcept>

namespace {
    using Json = nlohmann::json;

    /// <summary>
    /// JSONの項目を読む。なければ既定値を返さず例外にする（読み込み条件を満たさない表を黙って通さない）。
    /// </summary>
    template<typename T>
    T ReadRequired(const Json& node, const char* key, size_t rowNumber) {
        if (!node.contains(key)) {
            throw std::runtime_error("PhaseTable.json: phase " + std::to_string(rowNumber) + " has no key '" + key + "'");
        }
        return node.at(key).get<T>();
    }
}

const char* PhaseTableIO::ToString(PhaseType type) {
    switch (type) {
    case PhaseType::kBonus:
        return "Bonus";
    case PhaseType::kBoss:
        return "Boss";
    default:
        return "Normal";
    }
}

bool PhaseTableIO::FromString(const std::string& name, PhaseType& type) {
    if (name == "Normal") {
        type = PhaseType::kNormal;
        return true;
    }
    if (name == "Bonus") {
        type = PhaseType::kBonus;
        return true;
    }
    if (name == "Boss") {
        type = PhaseType::kBoss;
        return true;
    }
    return false;
}

bool PhaseTableIO::Validate(const PhaseTable& table, const std::vector<EnemyDefinition>& definitions, std::string& error) {
    // 行数
    if (static_cast<int32_t>(table.phases.size()) != kPhaseCount) {
        error = "phases must have " + std::to_string(kPhaseCount) + " rows";
        return false;
    }

    for (size_t i = 0; i < table.phases.size(); ++i) {
        const PhaseRow& row = table.phases[i];
        std::string label = "phase " + std::to_string(i + 1) + ": ";

        // 重みのIDが敵一覧に存在すること、重みが0以上であること
        for (const std::pair<const std::string, float>& weight : row.weights) {
            bool exists = false;
            for (const EnemyDefinition& def : definitions) {
                if (def.id == weight.first) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                error = label + "unknown enemy id in weights: " + weight.first;
                return false;
            }
            if (weight.second < 0.0f) {
                error = label + "weight is negative: " + weight.first;
                return false;
            }
        }

        if (row.expToNext < 0 || row.spawnPerInterval < 0 || row.maxEnemies < 0) {
            error = label + "expToNext, spawnPerInterval and maxEnemies must be 0 or more";
            return false;
        }
        if (row.spawnInterval <= 0.0f) {
            error = label + "spawnInterval must be positive";
            return false;
        }
    }
    return true;
}

PhaseTable PhaseTableIO::Load(const std::string& filePath, const std::vector<EnemyDefinition>& definitions) {
    if (!GameEngine::JsonSerializer::FileExists(filePath)) {
        throw std::runtime_error("PhaseTable.json not found: " + filePath);
    }

    Json root = GameEngine::JsonSerializer::LoadFromFile(filePath);
    if (!root.contains("phases") || !root.at("phases").is_array()) {
        throw std::runtime_error("PhaseTable.json has no 'phases' array");
    }

    PhaseTable table;
    for (const Json& node : root.at("phases")) {
        size_t rowNumber = table.phases.size() + 1;
        PhaseRow row;

        // 種類は3種類のどれかであること
        std::string typeName = ReadRequired<std::string>(node, "type", rowNumber);
        if (!FromString(typeName, row.type)) {
            throw std::runtime_error("PhaseTable.json: unknown type '" + typeName + "' in phase " + std::to_string(rowNumber));
        }

        row.expToNext = ReadRequired<int32_t>(node, "expToNext", rowNumber);
        row.spawnInterval = ReadRequired<float>(node, "spawnInterval", rowNumber);
        row.spawnPerInterval = ReadRequired<int32_t>(node, "spawnPerInterval", rowNumber);
        row.maxEnemies = ReadRequired<int32_t>(node, "maxEnemies", rowNumber);
        row.weights = ReadRequired<std::map<std::string, float>>(node, "weights", rowNumber);
        table.phases.push_back(row);
    }

    std::string error;
    if (!Validate(table, definitions, error)) {
        throw std::runtime_error("PhaseTable.json: " + error);
    }
    return table;
}

void PhaseTableIO::Save(const std::string& filePath, const PhaseTable& table) {
    Json rows = Json::array();
    for (const PhaseRow& row : table.phases) {
        Json node;
        node["type"] = ToString(row.type);
        node["expToNext"] = row.expToNext;
        node["spawnInterval"] = row.spawnInterval;
        node["spawnPerInterval"] = row.spawnPerInterval;
        node["maxEnemies"] = row.maxEnemies;
        node["weights"] = row.weights;
        rows.push_back(node);
    }

    Json root;
    root["phases"] = rows;
    GameEngine::JsonSerializer::SaveToFile(filePath, root);
}

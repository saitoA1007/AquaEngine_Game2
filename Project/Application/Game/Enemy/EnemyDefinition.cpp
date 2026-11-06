#include "pch.h"
#include "EnemyDefinition.h"
#include "JsonSerializer.h"
#include <set>
#include <stdexcept>

namespace {
    using Json = nlohmann::json;

    /// <summary>
    /// JSONの項目を読む。なければ既定値を返さず例外にする（読み込み条件を満たさない一覧を黙って通さない）。
    /// </summary>
    template<typename T>
    T ReadRequired(const Json& node, const char* key, const std::string& rowId) {
        if (!node.contains(key)) {
            throw std::runtime_error("EnemyList.json: row '" + rowId + "' has no key '" + key + "'");
        }
        return node.at(key).get<T>();
    }
}

const std::vector<std::string>& EnemyDefinitionIO::GetKnownKinds() {
    static const std::vector<std::string> kKinds = { "Chase" };
    return kKinds;
}

std::vector<std::string> EnemyDefinitionIO::GetRequiredExtraNames(const std::string& kind) {
    // 迫る敵と速い敵は追加の値を持たない
    if (kind == "Chase") {
        return {};
    }
    return {};
}

bool EnemyDefinitionIO::Validate(const std::vector<EnemyDefinition>& definitions, std::string& error) {
    std::set<std::string> ids;
    const std::vector<std::string>& kinds = GetKnownKinds();

    for (const EnemyDefinition& def : definitions) {
        if (def.id.empty()) {
            error = "id is empty";
            return false;
        }
        if (!ids.insert(def.id).second) {
            error = "id is duplicated: " + def.id;
            return false;
        }

        bool isKnownKind = false;
        for (const std::string& kind : kinds) {
            if (kind == def.kind) {
                isKnownKind = true;
            }
        }
        if (!isKnownKind) {
            error = "unknown kind '" + def.kind + "' in row: " + def.id;
            return false;
        }

        // 種類が必須とする追加の値が足りなければエラーにする
        for (const std::string& name : GetRequiredExtraNames(def.kind)) {
            if (def.extras.find(name) == def.extras.end()) {
                error = "extra value '" + name + "' is missing in row: " + def.id;
                return false;
            }
        }

        if (def.radius <= 0.0f || def.visualScale <= 0.0f || def.moveSpeed < 0.0f || def.iceThreshold < 0.0f || def.poolSize < 0) {
            error = "invalid number in row: " + def.id;
            return false;
        }
    }
    return true;
}

std::vector<EnemyDefinition> EnemyDefinitionIO::Load(const std::string& filePath) {
    if (!GameEngine::JsonSerializer::FileExists(filePath)) {
        throw std::runtime_error("EnemyList.json not found: " + filePath);
    }

    Json root = GameEngine::JsonSerializer::LoadFromFile(filePath);
    if (!root.contains("enemies") || !root.at("enemies").is_array()) {
        throw std::runtime_error("EnemyList.json has no 'enemies' array");
    }

    std::vector<EnemyDefinition> definitions;
    for (const Json& row : root.at("enemies")) {
        EnemyDefinition def;
        def.id = row.value("id", std::string());
        def.displayName = ReadRequired<std::string>(row, "displayName", def.id);
        def.kind = ReadRequired<std::string>(row, "kind", def.id);
        def.modelName = ReadRequired<std::string>(row, "modelName", def.id);
        def.visualScale = ReadRequired<float>(row, "visualScale", def.id);
        def.radius = ReadRequired<float>(row, "radius", def.id);
        def.moveSpeed = ReadRequired<float>(row, "moveSpeed", def.id);
        def.iceThreshold = ReadRequired<float>(row, "iceThreshold", def.id);
        def.contactDamage = ReadRequired<int32_t>(row, "contactDamage", def.id);
        def.exp = ReadRequired<int32_t>(row, "exp", def.id);
        def.iceShardCount = ReadRequired<int32_t>(row, "iceShardCount", def.id);
        def.breakCount = ReadRequired<int32_t>(row, "breakCount", def.id);
        def.poolSize = ReadRequired<int32_t>(row, "poolSize", def.id);

        std::vector<float> color = ReadRequired<std::vector<float>>(row, "color", def.id);
        if (color.size() != 4) {
            throw std::runtime_error("EnemyList.json: 'color' needs 4 numbers in row: " + def.id);
        }
        def.color = { color[0], color[1], color[2], color[3] };

        if (row.contains("extras")) {
            def.extras = row.at("extras").get<std::map<std::string, float>>();
        }
        definitions.push_back(def);
    }

    std::string error;
    if (!Validate(definitions, error)) {
        throw std::runtime_error("EnemyList.json: " + error);
    }
    return definitions;
}

void EnemyDefinitionIO::Save(const std::string& filePath, const std::vector<EnemyDefinition>& definitions) {
    Json rows = Json::array();
    for (const EnemyDefinition& def : definitions) {
        Json row;
        row["id"] = def.id;
        row["displayName"] = def.displayName;
        row["kind"] = def.kind;
        row["modelName"] = def.modelName;
        row["visualScale"] = def.visualScale;
        row["radius"] = def.radius;
        row["moveSpeed"] = def.moveSpeed;
        row["iceThreshold"] = def.iceThreshold;
        row["contactDamage"] = def.contactDamage;
        row["exp"] = def.exp;
        row["iceShardCount"] = def.iceShardCount;
        row["breakCount"] = def.breakCount;
        row["poolSize"] = def.poolSize;
        row["color"] = { def.color.x, def.color.y, def.color.z, def.color.w };
        row["extras"] = def.extras;
        rows.push_back(row);
    }

    Json root;
    root["enemies"] = rows;
    GameEngine::JsonSerializer::SaveToFile(filePath, root);
}

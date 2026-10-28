#include "pch.h"
#include "JsonSerializer.h"
#include <cassert>
#include <sstream>
#include <Windows.h>
using json = nlohmann::json;
using namespace GameEngine;

void JsonSerializer::SaveToFile(const std::string& filePath, const json& root, int indent) {

    // ディレクトリが無ければ作成する
    std::filesystem::path path(filePath);
    std::filesystem::path dir = path.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    // ファイルを書き込み用に開く
    std::ofstream ofs(filePath);
    if (ofs.fail()) {
        std::string message = "Failed to open file for write: " + filePath;
        MessageBoxA(nullptr, message.c_str(), "JsonSerializer", 0);
        assert(false && message.c_str());
    }

    ofs << std::setw(indent) << root << std::endl;
    ofs.close();
}

json JsonSerializer::LoadFromFile(const std::string& filePath) {

    std::ifstream ifs(filePath);
    if (ifs.fail()) {
        std::string message = "Failed to open file for load: " + filePath;
        MessageBoxA(nullptr, message.c_str(), "JsonSerializer", 0);
        assert(false && message.c_str());
    }

    json root;
    try {
        // json文字列からjsonのデータ構造に展開
        ifs >> root;
    }
    catch (const json::parse_error& e) {
        std::string message = "JSON parse error in: " + filePath + "\n" + e.what();
        MessageBoxA(nullptr, message.c_str(), "JsonSerializer", 0);
        assert(false && message.c_str());
    }

    ifs.close();
    return root;
}

void JsonSerializer::LoadDirectory(const std::string& directoryPath,
    const std::function<void(const std::string&, const json&)>& callback) {
    // ディレクトリが無ければスキップ
    if (!std::filesystem::exists(directoryPath)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
        const std::filesystem::path& filePath = entry.path();

        // .json以外はスキップ
        if (filePath.extension().string() != ".json") {
            continue;
        }

        // ファイル名とjsonをコールバックに渡す
        const std::string stem = filePath.stem().string();
        try {
            json root = LoadFromFile(filePath.string());
            callback(stem, root);
        }
        catch (const std::runtime_error&) {
            continue;
        }
    }
}

bool JsonSerializer::CanDeserialize(const nlohmann::json& node, nlohmann::json::value_t expectedType) {
    return node.type() == expectedType;
}

bool JsonSerializer::FileExists(const std::string& filePath) {
    return std::filesystem::exists(filePath);
}

bool JsonSerializer::DirectoryExists(const std::string& directoryPath) {
    return std::filesystem::exists(directoryPath);
}
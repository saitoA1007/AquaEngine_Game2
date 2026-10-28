#pragma once
#include <string>
#include "LogManager.h"

namespace GameEngine::PostEffectGraphFormat {

    // PostEffectGraphのjsonで使うノードの種類
    inline constexpr const char* kKindScene = "Scene";
    inline constexpr const char* kKindOutput = "Output";
    inline constexpr const char* kKindEffect = "Effect";

    // 保存形式のバージョン
    inline constexpr int kVersion = 1;

    // 保存、読み込みのログ
    inline void LogGraph(const std::string& message) {
        LogManager::GetInstance().Log("PostEffectGraph: " + message + "\n");
    }
}

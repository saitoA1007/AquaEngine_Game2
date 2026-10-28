#pragma once
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>
#include "JsonSerializer.h"
#include "NodeSystem/PostEffectGraph.h"
#include "PostEffectRegistry.h"

namespace GameEngine {

    /// <summary>
    ///  PostEffectGraphのJsonで保存、読み込み
    /// </summary>
    class PostEffectGraphSerializer {
    public:
        // 読み込み結果。最後まで成功した時だけ呼び出し側が反映する
        struct Loaded {
            PostEffectGraph graph;
            std::unordered_map<std::string, std::unique_ptr<IPostEffect>> effects;
            std::vector<std::string> passesToCreate;
        };

        static bool Save(const std::string& filePath, const PostEffectGraph& graph, const PostEffectRegistry& registry);

        static bool Load(const std::string& filePath, const PostEffectRegistry& registry,
            std::span<const std::string> reservedPasses, Loaded& out);
    };
}

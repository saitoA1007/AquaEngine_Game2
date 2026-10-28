#pragma once
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "PostEffectGraphSerializer.h"

namespace GameEngine {

    /// <summary>
    /// jsonからPostEffectGraphを読み込む1回分の作業。ノードを全部作ってからリンクを繋ぐ
    /// </summary>
    class PostEffectGraphLoader {
    public:
        // 成功が確定するまで反映を待つエフェクトの状態
        struct Pending {
            IPostEffect* fx;
            bool active;
            nlohmann::json params;
        };

    public:
        PostEffectGraphLoader(const PostEffectRegistry& registry, std::span<const std::string> reservedPasses,
            PostEffectGraphSerializer::Loaded& out)
            : registry_(registry), reservedPasses_(reservedPasses), out_(out) {}

        bool Run(const nlohmann::json& root);

    private:
        const PostEffectRegistry& registry_;

        std::span<const std::string> reservedPasses_;
        PostEffectGraphSerializer::Loaded& out_;
        std::unordered_map<int, PostEffectNode*> idMap_; // 保存時のID → 新しいノード
        std::unordered_set<std::string> usedPasses_;
        std::vector<Pending> pending_;

        bool hasScene_ = false;
        bool hasOutput_ = false;

    private:
        void LoadNodes(const nlohmann::json& nodes);
        PostEffectNode* LoadNode(const nlohmann::json& n);
        PostEffectNode* LoadEffectNode(const nlohmann::json& n);
        std::string ResolvePassName(const nlohmann::json& n);
        void LoadLinks(const nlohmann::json& root);
        // 成功が確定したので、エフェクトの状態を反映する
        void Finish();
    };
}

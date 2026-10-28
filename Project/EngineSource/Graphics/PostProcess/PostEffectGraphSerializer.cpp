#include "pch.h"
#include "PostEffectGraphSerializer.h"
#include "PostEffectGraphLoader.h"
#include "PostEffectGraphFormat.h"
#include <algorithm>

using namespace GameEngine;
using namespace GameEngine::PostEffectGraphFormat;

namespace {
    // 入力ピンが何番目のスロットか。見つからなければ-1
    int FindInputSlot(const PostEffectNode& node, int pinId) {
        auto it = std::ranges::find_if(node.inputs, [&](const Pin& p) { return p.id == pinId; });
        return it != node.inputs.end() ? static_cast<int>(it - node.inputs.begin()) : -1;
    }
}

bool PostEffectGraphSerializer::Save(const std::string& filePath, const PostEffectGraph& graph, const PostEffectRegistry& registry) {
    // 未登録のエフェクトは保存しない
    auto IsSaved = [&](const PostEffectNode& n) {
        return n.kind != PostEffectNodeKind::kEffect || registry.Owns(n.name, n.effect);
        };

    nlohmann::json root;
    root["version"] = kVersion;
    root["nodes"] = nlohmann::json::array();
    root["links"] = nlohmann::json::array();

    for (const auto& up : graph.nodes) {
        const PostEffectNode& node = *up;
        if (!IsSaved(node)) { continue; }

        nlohmann::json n;
        n["id"] = node.id;
        n["pos"] = nlohmann::json::array({ node.pos.x, node.pos.y });

        switch (node.kind) {
        case PostEffectNodeKind::kScene:
            n["kind"] = kKindScene;
            break;
        case PostEffectNodeKind::kOutput:
            n["kind"] = kKindOutput;
            break;
        case PostEffectNodeKind::kEffect: {
            n["kind"] = kKindEffect;
            n["type"] = node.typeName;
            n["name"] = node.name;
            n["active"] = node.effect->IsActive();
            n["passName"] = node.passName;
            nlohmann::json params = nlohmann::json::object();
            node.effect->SaveParams(params);
            n["params"] = std::move(params);
            break;
        }
        }
        root["nodes"].push_back(std::move(n));
    }

    for (const Link& l : graph.links) {
        const PostEffectNode* from = graph.FindNodeByPin(l.startPinId);
        const PostEffectNode* to = graph.FindNodeByPin(l.endPinId);
        if (!from || !to || !IsSaved(*from) || !IsSaved(*to)) { continue; }

        const int slot = FindInputSlot(*to, l.endPinId);
        if (slot < 0) { continue; }

        root["links"].push_back({ {"from", from->id}, {"to", to->id}, {"slot", slot} });
    }

    JsonSerializer::SaveToFile(filePath, root);
    return true;
}

bool PostEffectGraphSerializer::Load(const std::string& filePath, const PostEffectRegistry& registry,
    std::span<const std::string> reservedPasses, Loaded& out) {
    if (!JsonSerializer::FileExists(filePath)) { return false; }

    const nlohmann::json root = JsonSerializer::LoadFromFile(filePath);
    if (!root.is_object() || !root.contains("nodes") || !root["nodes"].is_array()) {
        LogGraph("jsonの形式が不正です");
        return false;
    }

    out = Loaded{};
    PostEffectGraphLoader loader(registry, reservedPasses, out);
    return loader.Run(root);
}

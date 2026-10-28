#include "pch.h"
#include "PostEffectGraphLoader.h"
#include "PostEffectGraphFormat.h"
#include <algorithm>

using namespace GameEngine;
using namespace GameEngine::PostEffectGraphFormat;

namespace {
    // "pos": [x, y]
    void ReadPosition(const nlohmann::json& n, PostEffectNode& node) {
        if (!n.contains("pos")) { return; }
        const auto& p = n["pos"];
        if (p.is_array() && p.size() == 2 && p[0].is_number() && p[1].is_number()) {
            node.pos.x = p[0].get<float>();
            node.pos.y = p[1].get<float>();
        }
    }
}

bool PostEffectGraphLoader::Run(const nlohmann::json& root) {
    LoadNodes(root["nodes"]);
    if (!hasScene_ || !hasOutput_) {
        LogGraph("Scene/Outputノードがありません");
        return false;
    }
    LoadLinks(root);
    Finish();
    return true;
}

void PostEffectGraphLoader::LoadNodes(const nlohmann::json& nodes) {
    for (const nlohmann::json& n : nodes) {
        try {
            const int oldId = n.at("id").get<int>();
            PostEffectNode* node = LoadNode(n);
            if (!node) { continue; }

            ReadPosition(n, *node);
            idMap_[oldId] = node;
        }
        catch (const nlohmann::json::exception&) {
            LogGraph("ノードの読み込みに失敗しました");
        }
    }
}

PostEffectNode* PostEffectGraphLoader::LoadNode(const nlohmann::json& n) {
    const std::string kind = n.at("kind").get<std::string>();

    // SceneとOutputは1つだけ
    if (kind == kKindScene) {
        if (hasScene_) { return nullptr; }
        hasScene_ = true;
        return out_.graph.AddSceneNode();
    }
    if (kind == kKindOutput) {
        if (hasOutput_) { return nullptr; }
        hasOutput_ = true;
        return out_.graph.AddOutputNode();
    }
    if (kind == kKindEffect) { return LoadEffectNode(n); }
    return nullptr;
}

PostEffectNode* PostEffectGraphLoader::LoadEffectNode(const nlohmann::json& n) {
    std::string type = n.value("type", std::string());
    if (type.empty()) {
        // version1 互換: "effect": "BloomPass" → "Bloom"
        type = n.at("effect").get<std::string>();
        if (type.ends_with("Pass")) { type.resize(type.size() - 4); }
    }

    auto fx = registry_.CreateInstance(type);
    if (!fx) {
        LogGraph("未登録の種類をスキップ: " + type);
        return nullptr;
    }

    const std::string name = MakeUniqueName(n.value("name", type), out_.effects);
    const std::string passName = ResolvePassName(n);

    IPostEffect* raw = fx.get();
    out_.effects[name] = std::move(fx);
    pending_.push_back({ raw, n.value("active", raw->IsActive()),
        n.contains("params") ? n["params"] : nlohmann::json::object() });

    return out_.graph.AddEffectNode(type, name, passName, raw);
}

std::string PostEffectGraphLoader::ResolvePassName(const nlohmann::json& n) {
    std::string passName = n.value("passName", n.value("persistentPass", std::string()));
    if (passName.empty()) { return {}; }

    if (std::ranges::find(reservedPasses_, passName) != reservedPasses_.end()) { return {}; }
    if (!usedPasses_.insert(passName).second) { return {}; }

    out_.passesToCreate.push_back(passName);
    return passName;
}

void PostEffectGraphLoader::LoadLinks(const nlohmann::json& root) {
    if (!root.contains("links") || !root["links"].is_array()) { return; }

    int total = 0, loaded = 0;
    for (const nlohmann::json& l : root["links"]) {
        ++total;
        try {
            const int fromId = l.at("from").get<int>();
            const int toId = l.at("to").get<int>();
            const int slot = l.at("slot").get<int>();

            auto s = idMap_.find(fromId);
            auto e = idMap_.find(toId);
            if (s == idMap_.end() || e == idMap_.end()) {
                LogGraph("リンクのノードが見つかりません from=" + std::to_string(fromId) + " to=" + std::to_string(toId));
                continue;
            }

            const PostEffectNode* from = s->second;
            const PostEffectNode* to = e->second;
            if (from->kind == PostEffectNodeKind::kOutput || slot < 0 || slot >= static_cast<int>(to->inputs.size())) {
                LogGraph("リンクのスロットが不正です");
                continue;
            }
            if (!out_.graph.CanConnect(from->output.id, to->inputs[slot].id)) {
                LogGraph("CanConnectが拒否しました " + from->name + " -> " + to->name);
                continue;
            }
            out_.graph.AddLink(from->output.id, to->inputs[slot].id);
            ++loaded;
        }
        catch (const nlohmann::json::exception&) {
            LogGraph("リンクの読み込みに失敗しました");
        }
    }
    LogGraph("リンク " + std::to_string(loaded) + "/" + std::to_string(total) + " 本を読み込みました");
}

void PostEffectGraphLoader::Finish() {
    for (auto& p : pending_) {
        p.fx->SetIsActive(p.active);
        try {
            p.fx->LoadParams(p.params);
        }
        catch (const nlohmann::json::exception&) {
            LogGraph("パラメータの読み込みに失敗しました");
        }
    }
}

#include "pch.h"
#include "PostEffectGraph.h"

using namespace GameEngine;

PostEffectNode* PostEffectGraph::FindNode(int nodeId) const {
    for (auto& n : nodes) { 
        if (n->id == nodeId) {
            return n.get(); 
        }
    }
    return nullptr;
}

PostEffectNode* PostEffectGraph::FindNodeByPin(int pinId) const {
    const Pin* pin = FindPin(pinId);
    return pin ? FindNode(pin->parentNodeId) : nullptr;
}

PostEffectNode* PostEffectGraph::FindOutputNode() const {
    for (auto& n : nodes) { 
        if (n->kind == PostEffectNodeKind::kOutput) { 
            return n.get(); 
        } 
    }
    return nullptr;
}

const Pin* PostEffectGraph::FindPin(int pinId) const {
    for (auto& n : nodes) {
        if (n->kind != PostEffectNodeKind::kOutput && n->output.id == pinId) { return &n->output; }
        for (auto& p : n->inputs) { 
            if (p.id == pinId) { return &p; 
            } 
        }
    }
    return nullptr;
}

const Link* PostEffectGraph::FindLinkToPin(int endPinId) const {
    for (auto& l : links) {
        if (l.endPinId == endPinId) {
            return &l; 
        } 
    }
    return nullptr;
}

PostEffectNode* PostEffectGraph::AddSceneNode() {
    PostEffectNode& n = EmplaceNode(PostEffectNodeKind::kScene, "Scene");
    n.output = MakePin("Scene", PinKind::kOutput, n.id);
    return  &n;
}

PostEffectNode* PostEffectGraph::AddOutputNode() {
    PostEffectNode& n = EmplaceNode(PostEffectNodeKind::kOutput, "Output");
    n.inputs.push_back(MakePin("Result", PinKind::kInput, n.id));
    return &n;
}

PostEffectNode* PostEffectGraph::AddEffectNode(const std::string& typeName, const std::string& name,
    const std::string& passName, IPostEffect* effect) {
    assert(effect != nullptr);
    PostEffectNode& n = EmplaceNode(PostEffectNodeKind::kEffect, name);
    n.typeName = typeName;
    n.name = name;
    n.passName = passName;
    n.effect = effect;
    for (uint32_t i = 0; i < effect->GetInputCount(); ++i) {
        n.inputs.push_back(MakePin(effect->GetInputName(i), PinKind::kInput, n.id));
    }
    n.output = MakePin("Out", PinKind::kOutput, n.id);
    return &n;
}

int PostEffectGraph::AddLink(int startPinId, int endPinId) {
    // 入力ピンへのリンクは1本だけ
    std::erase_if(links, [&](const Link& l) { return l.endPinId == endPinId; });
    int id = GetNextId();
    links.push_back({ id, startPinId, endPinId });
    dirty = true;
    return id;
}

void PostEffectGraph::RemoveLink(int linkId) {
    std::erase_if(links, [&](const Link& l) { return l.id == linkId; });
    dirty = true;
}

void PostEffectGraph::RemoveNode(int nodeId) {
    PostEffectNode* node = FindNode(nodeId);
    if (!node || node->kind != PostEffectNodeKind::kEffect) { return; }

    // そのノードのピンに繋がるリンクを全部消す
    std::erase_if(links, [&](const Link& l) {
        const Pin* s = FindPin(l.startPinId);
        const Pin* e = FindPin(l.endPinId);
        return (s && s->parentNodeId == nodeId) || (e && e->parentNodeId == nodeId);
        });
    std::erase_if(nodes, [&](const auto& n) { return n->id == nodeId; });
    dirty = true;
}

bool PostEffectGraph::DependsOn(int nodeId, int targetId, std::unordered_set<int>& visited) const {
    // nodeId の上流を辿って targetId に到達するか
    if (nodeId == targetId) { return true; }
    // 合流のあるグラフで同じノードを何度も辿らないようにする
    if (!visited.insert(nodeId).second) { return false; }

    const PostEffectNode* node = FindNode(nodeId);
    if (!node) { return false; }
    for (const Pin& pin : node->inputs) {
        const PostEffectNode* upstream = FindUpstreamNode(pin);
        if (upstream && DependsOn(upstream->id, targetId, visited)) { return true; }
    }
    return false;
}

bool PostEffectGraph::CanConnect(int startPinId, int endPinId) const {
    const Pin* start = FindPin(startPinId);
    const Pin* end = FindPin(endPinId);
    if (!start || !end) { return false; }
    if (start->pinKind != PinKind::kOutput || end->pinKind != PinKind::kInput) { return false; }
    if (start->parentNodeId == end->parentNodeId) { return false; }

    // start側のノードがすでにend側のノードに依存していたら、繋ぐと循環する
    std::unordered_set<int> visited;
    return !DependsOn(start->parentNodeId, end->parentNodeId, visited);
}

bool PostEffectGraph::TopoSort(int nodeId, std::unordered_set<int>& visited,
    std::unordered_set<int>& visiting, std::vector<int>& order) const {
    if (visited.contains(nodeId)) { return true; }
    if (visiting.contains(nodeId)) { return false; } // 循環
    visiting.insert(nodeId);

    const PostEffectNode* node = FindNode(nodeId);
    if (!node) { return false; }

    visiting.insert(nodeId);
    for (const Pin& pin : node->inputs) {
        const PostEffectNode* upstream = FindUpstreamNode(pin);
        if (upstream && !TopoSort(upstream->id, visited, visiting, order)) { return false; }
    }
    visiting.erase(nodeId);

    visited.insert(nodeId);
    order.push_back(nodeId);
    return true;
}

bool PostEffectGraph::BuildOrder(std::vector<int>& order) const {
    order.clear();
    const PostEffectNode* out = FindOutputNode();
    if (!out) { return false; }
    std::unordered_set<int> visited, visiting;
    return TopoSort(out->id, visited, visiting, order);
}

bool PostEffectGraph::IsPinLinked(int pinId) const {
    for (auto& l : links) {
        if (l.startPinId == pinId || l.endPinId == pinId) {
            return true; 
        }
    }
    return false;
}

PostEffectNode& PostEffectGraph::EmplaceNode(PostEffectNodeKind kind, std::string label) {
    PostEffectNode& n = *nodes.emplace_back(std::make_unique<PostEffectNode>());
    n.id = GetNextId();
    n.kind = kind;
    n.label = std::move(label);
    dirty = true;
    return n;
}

Pin PostEffectGraph::MakePin(const char* name, PinKind kind, int parentNodeId) {
    return { GetNextId(), name, PinType::kTexture2D, kind, parentNodeId };
}

const PostEffectNode* PostEffectGraph::FindUpstreamNode(const Pin& inputPin) const {
    const Link* link = FindLinkToPin(inputPin.id);
    if (!link) { return nullptr; }
    const Pin* start = FindPin(link->startPinId);
    return start ? FindNode(start->parentNodeId) : nullptr;
}
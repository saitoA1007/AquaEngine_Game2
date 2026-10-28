#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "MaterialGraph.h"
#include "PostProcess/IPostEffect.h"
#include "Vector2.h"

namespace GameEngine {

    enum class PostEffectNodeKind { kScene, kEffect, kOutput };

    struct PostEffectNode {
        int id = 0;
        PostEffectNodeKind kind = PostEffectNodeKind::kEffect;
        std::string label;
        std::string typeName;          // エフェクトの種類
        std::string name;              // インスタンス名
        std::string passName;          // 保存する描画パスの名前。空だと使い回し用のパスを使用
        IPostEffect* effect = nullptr; // kEffectのみ
        std::vector<Pin> inputs;
        Pin output{};                  // kOutputでは未使用
        Vector2 pos{};
    };

    struct PostEffectGraph {
        std::vector<std::unique_ptr<PostEffectNode>> nodes;
        std::vector<Link> links;
        int nextId = 1;
        bool dirty = true;             // 変更されたら実行順を再計算する

        int GetNextId() { return nextId++; }

        PostEffectNode* FindNode(int nodeId) const;
        PostEffectNode* FindNodeByPin(int pinId) const;
        PostEffectNode* FindOutputNode() const;
        const Pin* FindPin(int pinId) const;
        const Link* FindLinkToPin(int endPinId) const;

        // ノード生成
        PostEffectNode* AddSceneNode();
        PostEffectNode* AddOutputNode();
        PostEffectNode* AddEffectNode(const std::string& typeName, const std::string& name,
            const std::string& passName, IPostEffect* effect);

        // start=出力ピン, end=入力ピン。入力ピンは1リンクのみなので既存を置き換える
        int AddLink(int startPinId, int endPinId);
        void RemoveLink(int linkId);
        void RemoveNode(int nodeId);

        // 接続可能か
        bool CanConnect(int startPinId, int endPinId) const;

        // Outputノードから辿れるノードの実行順。循環があればfalse
        bool BuildOrder(std::vector<int>& order) const;

        bool IsPinLinked(int pinId) const;

    private:
        bool DependsOn(int nodeId, int targetId, std::unordered_set<int>& visited) const;
        bool TopoSort(int nodeId, std::unordered_set<int>& visited,
            std::unordered_set<int>& visiting, std::vector<int>& order) const;

        // ノードの共通初期化
        PostEffectNode& EmplaceNode(PostEffectNodeKind kind, std::string label);
        // ピン生成
        Pin MakePin(const char* name, PinKind kind, int parentNodeId);
        // 入力ピンにつながっている上流ノード。未接続ならnullptr
        const PostEffectNode* FindUpstreamNode(const Pin& inputPin) const;
    };
}
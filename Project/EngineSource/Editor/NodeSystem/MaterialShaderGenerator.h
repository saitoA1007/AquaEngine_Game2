#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include "MaterialNode.h"

namespace GameEngine {

    /// <summary>
    /// マテリアルノードからシェーダーのhlslを生成する
    /// </summary>
    class MaterialShaderGenerator {
    public:
        static std::string Generate(const MaterialGraph& graph);

    private:
        static PBROutputNode* FindOutputNode(const MaterialGraph& graph);

        // 出力ノードから入力方向へ深さ優先探索をして、依存関係順を求める
        static void TopologicalSortRecursive(
            const MaterialGraph& graph, int nodeId,
            std::unordered_set<int>& visited,
            std::unordered_set<int>& visiting,
            std::vector<int>& order);
    };
}
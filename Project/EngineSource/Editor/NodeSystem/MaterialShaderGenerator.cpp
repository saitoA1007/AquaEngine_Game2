#include "pch.h"
#include "MaterialShaderGenerator.h"
#include <cassert>
#include <format>
#include <unordered_map>
using namespace GameEngine;

PBROutputNode* MaterialShaderGenerator::FindOutputNode(const MaterialGraph& graph) {
    for (auto& node : graph.nodes) {
        if (auto* output = dynamic_cast<PBROutputNode*>(node.get())) {
            return output;
        }
    }
    return nullptr;
}

void MaterialShaderGenerator::TopologicalSortRecursive(
    const MaterialGraph& graph, int nodeId,
    std::unordered_set<int>& visited,
    std::unordered_set<int>& visiting,
    std::vector<int>& order)
{
    if (visited.contains(nodeId)) {
        return;
    }

    // マテリアルグラフは循環参照を許可しない
    assert(!visiting.contains(nodeId) && "MaterialGraph: 循環参照が検出されました");
    visiting.insert(nodeId);

    IMaterialNode* node = graph.FindNode(nodeId);
    assert(node && "MaterialGraph: ノードが見つかりません");

    // 入力ピンに繋がっている上流ノードを先に処理する
    for (auto& pin : node->GetInputs()) {
        for (auto& link : graph.links) {
            if (link.endPinId != pin.id) { continue; }

            const Pin* startPin = graph.FindPin(link.startPinId);
            if (startPin) {
                TopologicalSortRecursive(graph, startPin->parentNodeId, visited, visiting, order);
            }
        }
    }

    visiting.erase(nodeId);
    visited.insert(nodeId);
    order.push_back(nodeId);
}

std::string MaterialShaderGenerator::Generate(const MaterialGraph& graph) {
    PBROutputNode* outputNode = FindOutputNode(graph);
    assert(outputNode && "MaterialGraph: PBROutputNodeが見つかりません");
    if (!outputNode) { return ""; }

    // 出力ノードからトポロジカルソート
    std::vector<int> order;
    std::unordered_set<int> visited;
    std::unordered_set<int> visiting;
    TopologicalSortRecursive(graph, outputNode->GetId(), visited, visiting, order);

    // リンクされているピンに上流ノードの変数名を登録
    std::unordered_map<int, std::string> pinVars;
    for (auto& link : graph.links) {
        pinVars[link.endPinId] = std::format("v{}", link.startPinId);
    }

    // トポロジカル順にHLSLコードを連結
    std::string body;
    for (int nodeId : order) {
        if (IMaterialNode* node = graph.FindNode(nodeId)) {
            body += node->GenerateHLSL(pinVars);
        }
    }

    // PBROutputNodeの入力から最終出力に使う変数を取得
    auto& outInputs = outputNode->GetInputs();
    std::string baseColor = GetPinVar(pinVars, outInputs[0].id, "float4(0.8, 0.8, 0.8, 1.0)");
    std::string emissive = GetPinVar(pinVars, outInputs[4].id, "float3(0, 0, 0)");

    return std::format(R"(
#include "../../LightElement.hlsli"

Texture2D<float4> gTexture[] : register(t0, space0);
SamplerState gSampler : register(s0);

struct Camera
{{
    float3 worldPosition;
    float4x4 vpMatrix;
}};
ConstantBuffer<Camera> gCamera : register(b1);

cbuffer LightGroup : register(b2)
{{
    DirectionalLight gDirectionalLight;
    PointLight gPointLight;
    SpotLight gSpotLight;
    uint environmentTexture;
    int isActiveEnvironment;
}};

struct VertexShaderOutput
{{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD1;
    float3 normal : NORMAL1;
    float3 worldPosition : POSITION1;
}};

struct PixelShaderOutput
{{
    float4 color : SV_TARGET0;
}};

PixelShaderOutput main(VertexShaderOutput input)
{{
    PixelShaderOutput output;
{0}
    output.color = float4({1}.rgb + {2}, {1}.a);
    return output;
}}
)",
body, baseColor, emissive);
}
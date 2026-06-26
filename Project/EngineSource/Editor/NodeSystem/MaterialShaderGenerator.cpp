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

    // リソース宣言を収集し、レジスタ番号をここで割り振る
    std::string resourceDecls;
    int textureSlot = 0;
    for (int nodeId : order) {
        if (IMaterialNode* node = graph.FindNode(nodeId); node && node->RequiresTextureSlot()) {
            resourceDecls += node->GenerateResourceDeclaration(textureSlot++);
        }
    }

    // PBROutputNodeの各入力から最終出力に使う変数名を取得
    auto& outInputs = outputNode->GetInputs();
    std::string baseColor = GetPinVar(pinVars, outInputs[0].id, "float4(0.8, 0.8, 0.8, 1.0)");
    std::string metallic = GetPinVar(pinVars, outInputs[1].id, "0.0");
    std::string roughness = GetPinVar(pinVars, outInputs[2].id, "0.5");
    std::string normal = GetPinVar(pinVars, outInputs[3].id, "input.normal");
    std::string emissive = GetPinVar(pinVars, outInputs[4].id, "float3(0,0,0)");

    // テンプレートへ書き込み
    return std::format(R"(
        {0}
        SamplerState linearSampler : register(s0);
        
        struct VSOutput
        {{
            float4 position : SV_POSITION;
            float3 worldPos : TEXCOORD0;
            float3 normal   : TEXCOORD1;
            float2 uv       : TEXCOORD2;
        }};
        
        struct PSOutput
        {{
            float4 baseColor : SV_TARGET0;
            float4 normal    : SV_TARGET1;
            float4 material  : SV_TARGET2; // R:Metallic, G:Roughness
            float4 emissive  : SV_TARGET3;
        }};
        
        PSOutput main(VSOutput input)
        {{
            PSOutput output;
        {1}
            output.baseColor = {2};
            output.material  = float4({3}, {4}, 0, 1);
            output.normal    = float4({5}, 1);
            output.emissive  = float4({6}, 1);
            return output;
        }}
        )",
        resourceDecls, body, baseColor, metallic, roughness, normal, emissive);
}
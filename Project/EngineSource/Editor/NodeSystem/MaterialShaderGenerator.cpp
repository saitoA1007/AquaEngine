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

std::string MaterialShaderGenerator::ToIdentifier(const std::string& name) {
    std::string result;
    result.reserve(name.size());
    for (char c : name) {
        // 英数字以外はアンダースコアに置き換える
        bool isAlnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        result += isAlnum ? c : '_';
    }

    // 先頭が数字や空の場合は識別子として使えないため接頭辞を付ける
    if (result.empty() || (result[0] >= '0' && result[0] <= '9')) {
        result = "M_" + result;
    }
    return result;
}

std::string MaterialShaderGenerator::GenerateSurface(const MaterialGraph& graph, const std::string& identifier) {
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
            body += "    " + node->GenerateHLSL(pinVars);
        }
    }

    // PBROutputNodeの入力から最終出力に使う変数を取得
    auto& outInputs = outputNode->GetInputs();
    std::string baseColor = GetPinVar(pinVars, outInputs[0].id, "float4(0.8, 0.8, 0.8, 1.0)");
    std::string metallic = GetPinVar(pinVars, outInputs[1].id, "0.0f");
    std::string roughness = GetPinVar(pinVars, outInputs[2].id, "0.5f");
    std::string normal = GetPinVar(pinVars, outInputs[3].id, "input.normal");
    std::string emissive = GetPinVar(pinVars, outInputs[4].id, "float3(0, 0, 0)");

    // ピンの型が異なっても良いように、出力はAsFloat系で明示的に変換する
    return std::format(R"(// 自動生成ファイル。マテリアルエディターから書き出されるため直接編集しない
#ifndef SURFACE_{0}_HLSLI
#define SURFACE_{0}_HLSLI
#include "../MaterialSurface.hlsli"

SurfaceOutput EvaluateSurface_{0}(MaterialInput input)
{{
{1}
    SurfaceOutput output;
    output.baseColor = AsFloat4({2});
    output.metallic = AsFloat({3});
    output.roughness = AsFloat({4});
    output.normal = normalize(AsFloat3({5}));
    output.emissive = AsFloat3({6});
    return output;
}}

#endif
)",
identifier, body, baseColor, metallic, roughness, normal, emissive);
}

std::string MaterialShaderGenerator::GeneratePixelShader(const std::string& identifier) {
    return std::format(R"(// 自動生成ファイル。マテリアルエディターから書き出されるため直接編集しない
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

#include "{0}.Surface.hlsli"

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
    MaterialInput materialInput;
    materialInput.texcoord = input.texcoord;
    materialInput.normal = normalize(input.normal);
    materialInput.worldPosition = input.worldPosition;

    SurfaceOutput surface = EvaluateSurface_{0}(materialInput);

    PixelShaderOutput output;
    output.color = float4(surface.baseColor.rgb + surface.emissive, surface.baseColor.a);
    return output;
}}
)",
identifier);
}

std::string MaterialShaderGenerator::GenerateRayQueryMaterial(const std::string& identifier) {
    return std::format(R"(// 自動生成ファイル。マテリアルエディターから書き出されるため直接編集しない
#ifndef RAYQUERY_MATERIAL_{0}_HLSLI
#define RAYQUERY_MATERIAL_{0}_HLSLI
#include "../SurfaceShading.hlsli"
#include "../../../Material/Generated/{0}.Surface.hlsli"

ShadeResult ShadeMaterial_{0}(HitInfo hit, float3 rayDir)
{{
    ShadeResult result = InitShadeResult();

    // アクセスデータと頂点データを取得
    BufferRef ref = gBufferRefs[hit.instanceID];
    VertexData vtx = FetchHitVertex(hit, ref.vertexHandle, ref.indexHandle, ref.vertexOffset, ref.indexOffset);

    // ワールド空間に変換
    float3 worldPosition = mul(vtx.position, hit.objectToWorld);
    float3x3 normalMatrix = transpose((float3x3) hit.worldToObject);
    float3 worldNormal = normalize(mul(vtx.normal, normalMatrix));
    result.position = worldPosition;

    // 裏面の法線を視線側に向け直す
    float3 viewDir = normalize(gCamera.worldPosition.xyz - worldPosition);
    if (dot(worldNormal, viewDir) < 0.0f) {{ worldNormal = -worldNormal; }}

    // マテリアルグラフを評価
    MaterialInput input;
    input.texcoord = vtx.texcoord;
    input.normal = worldNormal;
    input.worldPosition = worldPosition;
    SurfaceOutput surface = EvaluateSurface_{0}(input);

    float3 N = surface.normal;
    if (dot(N, viewDir) < 0.0f) {{ N = -N; }}

    // ライティング
    ShadeOpaqueSurface(result, worldPosition, N, rayDir, surface.baseColor.rgb, surface.metallic, surface.roughness);
    result.color += surface.emissive;
    return result;
}}

#endif
)",
identifier);
}

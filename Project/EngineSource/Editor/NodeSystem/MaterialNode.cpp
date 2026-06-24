#include "MaterialNode.h"

using namespace GameEngine;

//=======================================================
//
//=======================================================

MultiplyNode::MultiplyNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Multiply";
    inputs_.push_back({ g.GetNextId(), "A", PinType::kFloat4, PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "B", PinType::kFloat4, PinKind::kInput, id_ });
    outputs_.push_back({ g.GetNextId(), "Result", PinType::kFloat4, PinKind::kOutput, id_ });
}

std::string MultiplyNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    std::string a = GetPinVar(pinVars, inputs_[0].id, "float4(1,1,1,1)");
    std::string b = GetPinVar(pinVars, inputs_[1].id, "float4(1,1,1,1)");
    return std::format("float4 v{} = {} * {};\n", outputs_[0].id, a, b);
}

//==============================================
// テクスチャノード
//==============================================

TextureSampleNode::TextureSampleNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Texture_Sample";
    inputs_.push_back({ g.GetNextId(), "UV", PinType::kFloat2, PinKind::kInput, id_ });
    outputs_.push_back({ g.GetNextId(), "RGBA", PinType::kFloat4, PinKind::kOutput, id_ });
    outputs_.push_back({ g.GetNextId(), "RGB",  PinType::kFloat3, PinKind::kOutput, id_ });
    outputs_.push_back({ g.GetNextId(), "A",    PinType::kFloat,  PinKind::kOutput, id_ });
    textureName = "tex_" + std::to_string(id_);
}

std::string TextureSampleNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    std::string uv = GetPinVar(pinVars, inputs_[0].id, "input.uv");
    return std::format(
        "float4 v{0} = {1}.Sample(linearSampler, {2});\n"
        "float3 v{3} = v{0}.rgb;\n"
        "float  v{4} = v{0}.a;\n",
        outputs_[0].id, textureName, uv,
        outputs_[1].id, outputs_[2].id
    );
}

//=============================================================
// PBRの出力ノード
//=============================================================

PBROutputNode::PBROutputNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "PBR_Output";
    inputs_.push_back({ g.GetNextId(), "BaseColor",  PinType::kFloat4, PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "Metallic",   PinType::kFloat,  PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "Roughness",  PinType::kFloat,  PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "Normal",     PinType::kFloat3, PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "Emissive",   PinType::kFloat3, PinKind::kInput, id_ });
}

//==========================================================
// ヘルパー関数
//==========================================================

namespace {

    std::string GetPinVar(
        const std::unordered_map<int, std::string>& pinVars, int pinId, const std::string& defaultVal) {
        // ピンIDを確認
        auto it = pinVars.find(pinId);
        if (it != pinVars.end()) {
            // 接続されている変数名を返す
            return it->second;
        }
        // 接続されていなければ、デフォルト値を返す
        return defaultVal;
    }
}


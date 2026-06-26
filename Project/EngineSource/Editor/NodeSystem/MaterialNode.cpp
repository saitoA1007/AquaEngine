#include "MaterialNode.h"
#include <format>
#include "ImGuiManager.h"
using namespace GameEngine;

//=======================================================
// 数式ノード
//=======================================================

MathNode::MathNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Math";
    inputs_.push_back({ g.GetNextId(), "A", PinType::kFloat4, PinKind::kInput, id_ });
    inputs_.push_back({ g.GetNextId(), "B", PinType::kFloat4, PinKind::kInput, id_ });
    outputs_.push_back({ g.GetNextId(), "Result", PinType::kFloat4, PinKind::kOutput, id_ });
}

void MathNode::DrawNodeUI() {
    static const char* operationNames[] = { "Add", "Subtract", "Multiply", "Divide" };
    const char* currentLabel = operationNames[static_cast<int>(operation_)];

    // ノード単位でIDが衝突しないよう"###"以降を固定IDにする
    std::string buttonId = std::format("{}###MathOpBtn{}", currentLabel, id_);
    std::string popupId = std::format("MathOpPopup{}", id_);

    //ノード内には現在値を表示するボタンだけを置く
    ImGui::SetNextItemWidth(100.0f);
    if (ImGui::Button(buttonId.c_str(), ImVec2(100.0f, 0.0f))) {
        // クリックされたらポップアップを開く
        ax::NodeEditor::Suspend();
        ImGui::OpenPopup(popupId.c_str());
        ax::NodeEditor::Resume();
    }

    // ポップアップの実描画はSuspendしている間に行う
    ax::NodeEditor::Suspend();
    if (ImGui::BeginPopup(popupId.c_str())) {
        for (int i = 0; i < IM_ARRAYSIZE(operationNames); ++i) {
            bool isSelected = (i == static_cast<int>(operation_));
            if (ImGui::Selectable(operationNames[i], isSelected)) {
                operation_ = static_cast<MathOperation>(i);
            }
        }
        ImGui::EndPopup();
    }
    ax::NodeEditor::Resume();
}

std::string MathNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    // 除算の場合はデフォルトで割る数を1にしておく
    std::string defaultB = (operation_ == MathOperation::kDivide || operation_ == MathOperation::kMultiply)
        ? "float4(1,1,1,1)" : "float4(0,0,0,0)";

    std::string a = GetPinVar(pinVars, inputs_[0].id, "float4(0,0,0,0)");
    std::string b = GetPinVar(pinVars, inputs_[1].id, defaultB);

    // モードに応じて演算子を切り替え
    char opSign = '+';
    switch (operation_) {
    case MathOperation::kAdd:       opSign = '+'; break;
    case MathOperation::kSubtract:  opSign = '-'; break;
    case MathOperation::kMultiply:  opSign = '*'; break;
    case MathOperation::kDivide:    opSign = '/'; break;
    }

    return std::format("float4 v{} = {} {} {};\n", outputs_[0].id, a, opSign, b);
}

//====================================================
// 定数ノード
//====================================================

ConstantNode::ConstantNode(MaterialGraph& g) {
    id_ = g.GetNextId();
    label_ = "Constant";
    outputs_.push_back({ g.GetNextId(), "Value", PinType::kFloat, PinKind::kOutput, id_ });
}

std::string ConstantNode::GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const {
    return std::format("float v{} = {};\n", outputs_[0].id, value_);
}

void ConstantNode::DrawNodeUI() {
    ImGui::SetNextItemWidth(70.0f);

    ImGui::DragFloat("##val", &value_, 0.01f, 0.0f, 0.0f, "%.3f");
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

std::string TextureSampleNode::GenerateResourceDeclaration(int textureSlot) const {
    return std::format("Texture2D {} : register(t{});\n", textureName, textureSlot);
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

namespace GameEngine{

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


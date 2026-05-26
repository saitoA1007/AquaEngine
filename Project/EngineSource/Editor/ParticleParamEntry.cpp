#include "ParticleParamEntry.h"
#include "ImGuiManager.h"
using json = nlohmann::json;
using namespace GameEngine;

//=============================================================
// float
//=============================================================
void FloatEntry::DrawImGui() {
    ImGui::Text("%s", label.c_str());
    ImGui::DragFloat(("##" + label).c_str(), ptr, 0.01f);
}
void FloatEntry::Serialize(json& n) const { n = *ptr; }
void FloatEntry::Deserialize(const json& n) { if (n.is_number()) *ptr = n.get<float>(); }

//=============================================================
// int32_t
//=============================================================
void Int32Entry::DrawImGui() {
    ImGui::Text("%s", label.c_str());
    ImGui::DragInt(("##" + label).c_str(), ptr);
}
void Int32Entry::Serialize(json& n)         const { n = *ptr; }
void Int32Entry::Deserialize(const json& n) { if (n.is_number()) *ptr = n.get<int32_t>(); }

//=============================================================
// uint32_t
//=============================================================
void UInt32Entry::DrawImGui() {
    ImGui::Text("%s", label.c_str());
    ImGui::DragScalar(("##" + label).c_str(), ImGuiDataType_U32, ptr, 1.0f);
}
void UInt32Entry::Serialize(json& n)         const { n = *ptr; }
void UInt32Entry::Deserialize(const json& n) { if (n.is_number()) *ptr = n.get<uint32_t>(); }

//=============================================================
// bool
//=============================================================
void BoolEntry::DrawImGui() {
    ImGui::Checkbox(label.c_str(), ptr);
}
void BoolEntry::Serialize(json& n)         const { n = *ptr; }
void BoolEntry::Deserialize(const json& n) { if (n.is_boolean()) *ptr = n.get<bool>(); }

//=============================================================
// Vector2
//=============================================================
void Vector2Entry::DrawImGui() {
    ImGui::Text("%s", label.c_str());
    ImGui::DragFloat2(("##" + label).c_str(), reinterpret_cast<float*>(ptr), 0.01f);
}
void Vector2Entry::Serialize(json& n) const {
    n = json::array({ ptr->x, ptr->y });
}
void Vector2Entry::Deserialize(const json& n) {
    if (n.is_array() && n.size() == 2) {
        *ptr = { n[0].get<float>(), n[1].get<float>() };
    }
}

//=============================================================
// Vector3
//=============================================================
void Vector3Entry::DrawImGui() {
    ImGui::Text("%s", label.c_str());
    ImGui::DragFloat3(("##" + label).c_str(), reinterpret_cast<float*>(ptr), 0.01f);
}
void Vector3Entry::Serialize(json& n) const {
    n = json::array({ ptr->x, ptr->y, ptr->z });
}
void Vector3Entry::Deserialize(const json& n) {
    if (n.is_array() && n.size() == 3) {
        *ptr = { n[0].get<float>(), n[1].get<float>(), n[2].get<float>() };
    }
}

//=============================================================
// Vector4
//=============================================================
void Vector4Entry::DrawImGui() {
    ImGui::Text("%s", label.c_str());
    ImGui::ColorEdit4(("##" + label).c_str(), reinterpret_cast<float*>(ptr));
}
void Vector4Entry::Serialize(json& n) const {
    n = json::array({ ptr->x, ptr->y, ptr->z, ptr->w });
}
void Vector4Entry::Deserialize(const json& n) {
    if (n.is_array() && n.size() == 4) {
        *ptr = { n[0].get<float>(), n[1].get<float>(), n[2].get<float>(), n[3].get<float>() };
    }
}


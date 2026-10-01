#include "RayQueryMaterialRegistry.h"
#include <format>
#include <fstream>
#include <cassert>
#include "JsonSerializer.h"
#include "LogManager.h"
using namespace GameEngine;

void RayQueryMaterialRegistry::Load() {
	entries_.clear();

	if (!JsonSerializer::FileExists(kRegistryPath_)) { return; }

	nlohmann::json root = JsonSerializer::LoadFromFile(kRegistryPath_);
	if (!root.contains("materials")) { return; }

	for (const auto& material : root["materials"]) {
		entries_.push_back({ material["name"].get<std::string>(), material["id"].get<uint32_t>() });
	}
}

uint32_t RayQueryMaterialRegistry::RegisterMaterial(const std::string& identifier) {
	uint32_t id = 0;
	bool isFound = false;
	for (const auto& entry : entries_) {
		if (entry.identifier == identifier) {
			id = entry.id;
			isFound = true;
			break;
		}
	}

	// 未登録であれば新しいIDを割り当てる。既存のIDは変えない
	if (!isFound) {
		id = kFirstGeneratedMaterialId;
		for (const auto& entry : entries_) {
			id = (std::max)(id, entry.id + 1);
		}
		entries_.push_back({ identifier, id });
		Save();
	}

	// マテリアル関数の中身が変わっている可能性があるため毎回書き出して再生成させる
	WriteDispatch();
	++version_;

	LogManager::GetInstance().Log(std::format("RayQuery material registered : {} (id:{})", identifier, id));
	return id;
}

uint32_t RayQueryMaterialRegistry::GetMaterialId(const std::string& identifier) const {
	for (const auto& entry : entries_) {
		if (entry.identifier == identifier) {
			return entry.id;
		}
	}
	LogManager::GetInstance().Log("RayQuery material not found : " + identifier);
	return kDefaultMaterialId;
}

void RayQueryMaterialRegistry::Save() const {
	nlohmann::json root = nlohmann::json::object();
	root["materials"] = nlohmann::json::array();
	for (const auto& entry : entries_) {
		root["materials"].push_back({ { "name", entry.identifier }, { "id", entry.id } });
	}
	JsonSerializer::SaveToFile(kRegistryPath_, root);
}

void RayQueryMaterialRegistry::WriteDispatch() const {
	std::string includes;
	std::string cases;
	for (const auto& entry : entries_) {
		includes += std::format("#include \"Materials/Generated/{}.hlsli\"\n", entry.identifier);
		cases += std::format(
			"        case {}:\n"
			"            return ShadeMaterial_{}(hit, rayDir);\n\n",
			entry.id, entry.identifier);
	}

	std::string source = std::format(R"(// 自動生成ファイル。RayQueryMaterialRegistryが書き出すため直接編集しない
#ifndef MATERIAL_DISPATCH_HLSLI
#define MATERIAL_DISPATCH_HLSLI
#include "RayQueryCommon.hlsli"
#include "Materials/DefaultMaterial.hlsli"
#include "Materials/IceMaterial.hlsli"
{0}
// マテリアルIDから対応するシェーディング関数を呼び出す
ShadeResult EvaluateMaterial(HitInfo hit, float3 rayDir)
{{
    switch (hit.materialId)
    {{
        case MATERIAL_ID_ICE:
            return ShadeIceMaterial(hit, rayDir);

{1}        case MATERIAL_ID_DEFAULT:
        default:
            return ShadeDefaultMaterial(hit, rayDir);
    }}
}}

#endif
)", includes, cases);

	// 他のシェーダーファイルと改行コードを揃えるためテキストモードで書き出す
	std::ofstream out(kDispatchPath_);
	assert(out.is_open() && "Failed to write MaterialDispatch.hlsli");
	out << source;
}

#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include "Vector2.h"

namespace GameEngine {

	enum class PinType {
		kFloat,
		kFloat2,
		kFloat3,
		kFloat4,
		kTexture2D,
		kBool,
	};

	enum class PinKind {
		kInput,
		kOutput,
	};

	struct Pin {
		int id;				// id
		std::string name;	// 名前
		PinType pinType;	// ピンの型
		PinKind pinKind;	// ピンの種類
		int parentNodeId;	// 親のid
		float defaultVal[4] = { 0,0,0,1 };
	};

	struct Link {
		int id;
		int startPinId;
		int endPinId;
	};

	// ノードの基底クラス
	class IMaterialNode {
	public:
		virtual ~IMaterialNode() = default;

		// hlsl生成
		virtual std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const = 0;

		// テクスチャ等、グローバルなリソース宣言が必要かどうか
		virtual bool RequiresTextureSlot() const { return false; }

		// リソース宣言のHLSL文字列を生成する
		virtual std::string GenerateResourceDeclaration(int textureSlot) const { return ""; }

		// ノード内に独自描画
		virtual void DrawNodeUI() {}

	public:

		int GetId() const { return id_; }

		const std::string& GetLabel() const { return label_; }

		std::vector<Pin>& GetInputs() { return inputs_; }
		std::vector<Pin>& GetOutputs() { return outputs_; }

	protected:
		int id_;
		std::string label_;
		std::vector<Pin> inputs_;
		std::vector<Pin> outputs_;
		Vector2 pos_;
	};

	// マテリアルノードデータ
	struct MaterialGraph {
		std::vector<std::unique_ptr<IMaterialNode>> nodes;
		std::vector<Link> links;
		int nextId = 1;

		int GetNextId() { return nextId++; }

		// ピンIDからピン情報を検索
		const Pin* FindPin(int pinId) const;

		// ノードIDからノードを検索
		IMaterialNode* FindNode(int nodeId) const;
	};
}


#pragma once
#include <format>
#include "MaterialGraph.h"

namespace GameEngine {

    class MultiplyNode : public IMaterialNode {
    public:
        MultiplyNode(MaterialGraph& g);

        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;
    };

    // テクスチャノード
    class TextureSampleNode : public IMaterialNode {
    public:
        TextureSampleNode(MaterialGraph& g);

        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

    private:
        // テクスチャ名
        std::string textureName; 
    };

    // マスターサーフェスノード
    class PBROutputNode : public IMaterialNode {
    public:
        PBROutputNode(MaterialGraph& g);

        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override {
            // 出力ノードは値を収集するだけ
            pinVars;
            return "";
        }
    };
}

// ヘルパー関数
namespace {

    std::string GetPinVar(const std::unordered_map<int, std::string>& pinVars, int pinId, const std::string& defaultVal);
}
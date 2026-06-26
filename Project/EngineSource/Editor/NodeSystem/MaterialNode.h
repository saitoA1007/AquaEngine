#pragma once
#include "MaterialGraph.h"

namespace GameEngine {

    // 数式ノードの演算タイプ
    enum class MathOperation {
        kAdd,       // 加算
        kSubtract,  // 減算
        kMultiply,  // 乗算
        kDivide     // 除算
    };

    // 数学ノード
    class MathNode : public IMaterialNode {
    public:
        MathNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

        // ノードUIの描画
        void DrawNodeUI() override;

    private:
        MathOperation operation_ = MathOperation::kAdd;
    };

    // 定数ノード
    class ConstantNode : public IMaterialNode {
    public:
        ConstantNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

        // ノードUI描画
        void DrawNodeUI() override;

    private:
        float value_ = 0.0f; // 保持する数値
    };

    // テクスチャノード
    class TextureSampleNode : public IMaterialNode {
    public:
        TextureSampleNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

        bool RequiresTextureSlot() const override { return true; }
        std::string GenerateResourceDeclaration(int textureSlot) const override;

    private:
        // テクスチャ名
        std::string textureName; 
    };

    // マスターサーフェスノード
    class PBROutputNode : public IMaterialNode {
    public:
        PBROutputNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override {
            // 出力ノードは値を収集するだけ
            pinVars;
            return "";
        }
    };

    // ヘルパー関数
    std::string GetPinVar(const std::unordered_map<int, std::string>& pinVars, int pinId, const std::string& defaultVal);
}
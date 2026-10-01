#pragma once
#include <string>
#include <unordered_set>
#include <vector>
#include "MaterialNode.h"

namespace GameEngine {

    /// <summary>
    /// マテリアルノードからシェーダーのhlslを生成する
    /// ノードの計算はサーフェス関数(EvaluateSurface_<名前>)にまとめ、
    /// ラスタライズ用のPSとRayQuery用のマテリアル関数の両方から呼び出す
    /// </summary>
    class MaterialShaderGenerator {
    public:

        /// <summary>
        /// マテリアル名をHLSLの識別子として使える形に変換する
        /// </summary>
        static std::string ToIdentifier(const std::string& name);

        /// <summary>
        /// サーフェス関数を生成する。PBROutputNodeが無ければ空文字を返す
        /// </summary>
        /// <param name="graph">マテリアルグラフ</param>
        /// <param name="identifier">ToIdentifierで変換したマテリアル名</param>
        static std::string GenerateSurface(const MaterialGraph& graph, const std::string& identifier);

        /// <summary>
        /// ラスタライズ用のPSを生成する。同じディレクトリのサーフェス関数をincludeする
        /// </summary>
        static std::string GeneratePixelShader(const std::string& identifier);

        /// <summary>
        /// RayQuery用のマテリアル関数(ShadeMaterial_<名前>)を生成する
        /// </summary>
        static std::string GenerateRayQueryMaterial(const std::string& identifier);

    private:
        static PBROutputNode* FindOutputNode(const MaterialGraph& graph);

        // 出力ノードから入力方向へ深さ優先探索をして、依存関係順を求める
        static void TopologicalSortRecursive(
            const MaterialGraph& graph, int nodeId,
            std::unordered_set<int>& visited,
            std::unordered_set<int>& visiting,
            std::vector<int>& order);
    };
}

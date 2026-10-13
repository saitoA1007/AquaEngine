#pragma once
#include <string>
#include <vector>
#include "JsonSerializer.h"
#include "NodeSystem/PostEffectGraph.h"
#include "PostEffectRegistry.h"

namespace GameEngine {

    // PostEffectGraphのJsonで保存、読み込み
    class PostEffectGraphSerializer {
    public:
        // 読み込み結果。最後まで成功した時だけ呼び出し側が反映する
        struct Loaded {
            PostEffectGraph graph;
            std::unordered_map<std::string, std::unique_ptr<IPostEffect>> effects;
            std::vector<std::string> passesToCreate;
        };

        static bool Save(const std::string& filePath, const PostEffectGraph& graph, const PostEffectRegistry& registry);
        static bool Load(const std::string& filePath, const PostEffectRegistry& registry, Loaded& out);
    };

    // 読み込み1回分の作業。ノードを全部作ってからリンクを繋ぐ
    class Loader {
    public:

        struct Pending { 
            IPostEffect* fx;
            bool active;
            nlohmann::json params;
        };

    public:
        Loader(const PostEffectRegistry& registry, PostEffectGraphSerializer::Loaded& out)
            : registry_(registry), out_(out) {}

        bool Run(const nlohmann::json& root);

    private:
        const PostEffectRegistry& registry_;
        PostEffectGraphSerializer::Loaded& out_;
        std::unordered_map<int, PostEffectNode*> idMap_;
        std::unordered_set<std::string> usedPasses_;
        std::vector<Pending> pending_;
        bool hasScene_ = false;
        bool hasOutput_ = false;

    private:
        
        void LoadNodes(const nlohmann::json& nodes);

        PostEffectNode* LoadNode(const nlohmann::json& n);

        PostEffectNode* LoadEffectNode(const nlohmann::json& n);

        std::string ResolvePassName(const nlohmann::json& n);

        void LoadLinks(const nlohmann::json& root);

        // 成功が確定したので、エフェクトの状態を反映する
        void Finish();
    };
}



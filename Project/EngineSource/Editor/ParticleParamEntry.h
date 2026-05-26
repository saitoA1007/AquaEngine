#pragma once
#include <string>
#include <json.hpp>

#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"

namespace GameEngine {

    struct IParamEntry {
        virtual ~IParamEntry() = default;
        explicit IParamEntry(std::string lbl, int pri)
            : label(std::move(lbl)), priority(pri) {}


        std::string label;
        int priority = INT_MAX;

        /// ImGuiを描画する
        virtual void DrawImGui() = 0;
        /// JSONノードに値を書き出す
        virtual void Serialize(nlohmann::json& node) const = 0;
        /// JSONノードから値を読み込む
        virtual void Deserialize(const nlohmann::json& node) = 0;
    };

    // float
    struct FloatEntry : IParamEntry {
        float* ptr;

        FloatEntry(std::string l, float* p, int pri) : IParamEntry(std::move(l), pri), ptr(p) {}
        void DrawImGui() override;
        void Serialize(nlohmann::json& n) const override;
        void Deserialize(const nlohmann::json& n) override;
    };

    // int32_t
    struct Int32Entry : IParamEntry {
        int32_t* ptr;

        Int32Entry(std::string l, int32_t* p, int pri) : IParamEntry(std::move(l), pri), ptr(p) {}
        void DrawImGui() override;
        void Serialize(nlohmann::json& n) const override;
        void Deserialize(const nlohmann::json& n) override;
    };

    // uint32_t
    struct UInt32Entry : IParamEntry {
        uint32_t* ptr;

        UInt32Entry(std::string l, uint32_t* p, int pri) : IParamEntry(std::move(l), pri), ptr(p) {}
        void DrawImGui() override;
        void Serialize(nlohmann::json& n) const override;
        void Deserialize(const nlohmann::json& n) override;
    };

    // bool
    struct BoolEntry : IParamEntry {
        bool* ptr;

        BoolEntry(std::string l, bool* p, int pri) : IParamEntry(std::move(l), pri), ptr(p) {}
        void DrawImGui() override;
        void Serialize(nlohmann::json& n) const override;
        void Deserialize(const nlohmann::json& n) override;
    };

    // Vector2
    struct Vector2Entry : IParamEntry {
        Vector2* ptr;

        Vector2Entry(std::string l, Vector2* p, int pri) : IParamEntry(std::move(l), pri), ptr(p) {}
        void DrawImGui() override;
        void Serialize(nlohmann::json& n) const override;
        void Deserialize(const nlohmann::json& n) override;
    };

    // Vector3
    struct Vector3Entry : IParamEntry {
        Vector3* ptr;

        Vector3Entry(std::string l, Vector3* p, int pri) : IParamEntry(std::move(l), pri), ptr(p) {}
        void DrawImGui() override;
        void Serialize(nlohmann::json& n) const override;
        void Deserialize(const nlohmann::json& n) override;
    };

    // Vector4
    struct Vector4Entry : IParamEntry {
        Vector4* ptr;

        Vector4Entry(std::string l, Vector4* p, int pri) : IParamEntry(std::move(l), pri), ptr(p) {}
        void DrawImGui() override;
        void Serialize(nlohmann::json& n) const override;
        void Deserialize(const nlohmann::json& n) override;
    };
}


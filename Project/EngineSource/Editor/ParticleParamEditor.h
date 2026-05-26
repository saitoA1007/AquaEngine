#pragma once
#include <memory>
#include <functional>
#include <unordered_map>
#include "ParticleParamEntry.h"

namespace GameEngine {

    // パーティクル用のパラメータ、IParamEntryを継承した型を生成する
    template<typename T>
    std::unique_ptr<IParamEntry> MakeEntry(const std::string& label, T* ptr, int priority) {
        // 未対応型はコンパイルエラーにする
        static_assert(sizeof(T) == 0, "ParticleParamEditor MakeEntry: 未対応の型です。");
        return nullptr;
    }

	class ParticleParamEditor {
	public:

        // グループ
        struct Group {
            std::vector<std::unique_ptr<IParamEntry>> entries;  // パラメータ
            std::unordered_map<std::string, Group> children; // サブグループ
        };

    public:

        /// <summary>
        /// 登録
        /// </summary>
        template<typename T>
        void Register(const std::string& path, const std::string& label, T* ptr, int priority = INT_MAX) {
            ResolveGroup(path).entries.push_back(MakeEntry<T>(label, ptr, priority));
        }

        // 型を別の形で使用したい場合は都度追加していく
        //void ParticleParamEditor::Register(const std::string& path, const std::string& label, Vector4* ptr, int priority) {
        //    ResolveGroup(path).entries.push_back(std::make_unique<Vector4Entry>(label, ptr, priority));
        //}

        /// <summary>
        /// グループの登録解除
        /// </summary>
        void RemoveGroup(const std::string& path);

        /// <summary>
        /// アイテムの登録解除
        /// </summary>
        void RemoveItem(const std::string& path, const std::string& label);

        /// <summary>
        /// imguiを描画
        /// </summary>
        void DrawImgui();

        // jsonの保存と読み込み
        void Serialize(nlohmann::json& root) const;
        void Deserialize(const nlohmann::json& root);
	private:

        // 保存グループ
        Group root_;

    private:

        // パスをたどってグループの取得、生成
        Group& ResolveGroup(const std::string& path);
        Group* FindGroup(const std::string& path);

        // 描画
        void DrawGroup(Group& group);
        void DrawEntries(Group& group);

        // jsonの再帰処理
        void SerializeGroup(nlohmann::json& node, const Group& group)   const;
        void DeserializeGroup(const nlohmann::json& node, Group& group);

        static std::vector<std::string> SplitPath(const std::string& path);
	};

#define PARTICLE_MAKE_ENTRY(Type, EntryType)                          \
    template<> inline std::unique_ptr<IParamEntry>                    \
    MakeEntry<Type>(const std::string& label, Type* ptr, int priority)\
    { return std::make_unique<EntryType>(label, ptr, priority); }

    PARTICLE_MAKE_ENTRY(float, FloatEntry)
    PARTICLE_MAKE_ENTRY(int32_t, Int32Entry)
    PARTICLE_MAKE_ENTRY(uint32_t, UInt32Entry)
    PARTICLE_MAKE_ENTRY(bool, BoolEntry)
    PARTICLE_MAKE_ENTRY(Vector2, Vector2Entry)
    PARTICLE_MAKE_ENTRY(Vector3, Vector3Entry)
    PARTICLE_MAKE_ENTRY(Vector4, Vector4Entry)

#undef PARTICLE_MAKE_ENTRY
}


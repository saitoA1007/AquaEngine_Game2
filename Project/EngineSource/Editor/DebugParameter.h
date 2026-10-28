#pragma once
#include <string>
#include <vector>
#include <memory>
#include "GameParamEditor.h"

namespace GameEngine {

    // 前方宣言
    class WorldTransform;
    class Sprite;
    class Text;

	class DebugParameter {
	public:
		DebugParameter(const std::string& rootGroupName);

        static void StaticInitialize(GameParamEditor* gameParamEditor) {
            gameParamEditor_ = gameParamEditor;
        }

		template<typename T>
        void Register(const std::string& key, T& valueRef, int priority = INT_MAX,const std::string subGroupName = "") {
            std::string path;
            if (subGroupName.empty()) {
                path = rootGroupName_;
            } else {
                path = rootGroupName_ + "/" + subGroupName;
            }

            // すでに同じグループ・同じキーが登録されているか検索
            auto it = std::find_if(bindings_.begin(), bindings_.end(), [&](const std::unique_ptr<IParamBinding>& b) {
                return b->GetGroupName() == path && b->GetKeyName() == key;
                });
            if (it != bindings_.end()) {
                // GameParamEditor側から古い値を一度削除
                (*it)->Remove();
                // 自身の bindings_ からも古いバインディングを削除・解放
                bindings_.erase(it);
            }

            // 値を登録する
            gameParamEditor_->AddItem(path, key, valueRef, priority);

            // 登録されたデータのポインタを取得する
            GameParamEditor::Item* itemPtr = gameParamEditor_->FindItemMutable(path, key);

            // 値を保持
            bindings_.push_back(std::make_unique<ParamBinding<T>>(path, subGroupName, key, valueRef, itemPtr));
        }

        void RegisterWorld(const std::string& worldName, WorldTransform& world, const std::string subGroupName = "");
        void RegisterSprite(const std::string& spriteName, Sprite& sprite,const std::string subGroupName = "");
        void RegisterText(const std::string& textName, Text& text, const std::string subGroupName = "");

        // 値を適応する
        void Apply();

        // 値が変更された時だけ適応する
        bool ApplyIfDirty();

        // 登録を解除する
        void RemoveItem(const std::string& key, const std::string& subGroupName = "");
        void RemoveGroup(const std::string& subGroupName = "");

	private:
        // 基底クラス
        struct IParamBinding {
        public:
            virtual ~IParamBinding() = default;
            virtual void Apply() = 0;
            virtual bool IsDirty() const = 0;
            virtual void ClearDirty() = 0;
            virtual void Remove() = 0;
            virtual std::string GetGroupName() const = 0;
            virtual std::string GetKeyName() const = 0;
        };

        // 型ごとに実装
        template<typename T>
        struct ParamBinding : IParamBinding {
            std::string groupName;
            std::string subGroupName;
            std::string key;
            T& valueRef;
            GameParamEditor::Item* cachedItem;

            ParamBinding(const std::string& group,const std::string& subGroup, const std::string& k, T& ref, GameParamEditor::Item* item)
                : groupName(group), subGroupName(subGroup), key(k), valueRef(ref), cachedItem(item) {
            }

            void Apply() override {
                if (cachedItem) {
                    valueRef = std::get<T>(cachedItem->value);
                }
            }

            bool IsDirty() const override {
                return cachedItem ? cachedItem->isDirty : false;
            }

            void ClearDirty() override {
                if (cachedItem) {
                    cachedItem->isDirty = false;
                }
            }

            void Remove() override {
                gameParamEditor_->RemoveItem(groupName, key);
            }

            std::string GetGroupName() const override { return groupName; }
            std::string GetKeyName() const override { return key; }
        };

    private:
        static GameParamEditor* gameParamEditor_;
        std::string rootGroupName_;
        std::vector<std::unique_ptr<IParamBinding>> bindings_;
	};
}


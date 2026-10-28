#pragma once
#include <vector>
#include <memory>
#include "IGameObject.h"

namespace GameEngine {

    /// <summary>
    /// ゲームオブジェクト管理クラス
    /// </summary>
    class GameObjectManager {
    public:
        GameObjectManager();
        ~GameObjectManager() = default;

        /// <summary>
        /// オブジェクトを生成して追加する
        /// </summary>
        /// <typeparam name="T">追加するオブジェクトの型</typeparam>
        /// <typeparam name="...Args">コンストラクタの項目</typeparam>
        /// <param name="...args">コンストラクタの項目</param>
        /// <returns>追加したオブジェクトのポインタ</returns>
        template<typename T, typename... Args>
        T* AddObject(Args&&... args) {
            static_assert(std::is_base_of<IGameObject, T>::value, "T must derive from GameObjectBase");
            auto obj = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = obj.get();
            obj->Initialize();

            if (isIterating_) {
                // ループ中は待機リストへ
                pendingObjects_.push_back(std::move(obj));
            } else {
                objects_.push_back(std::move(obj));
                needsSort_ = true;
            }
            return ptr;
        }

        /// <summary>
        /// オブジェクトを削除
        /// </summary>
        /// <param name="object"></param>
        void RemoveObject(IGameObject* object) {
            if (isIterating_) {
                // ループ中は削除フラグだけ立て、UpdateAllの最後で消す
                object->Destroy();
                return;
            }
            std::erase_if(objects_, [object](const std::unique_ptr<IGameObject>& item) {
                return item.get() == object;
                });
        }

        // 初期化処理
        void InitializeAll();

        // 更新処理
        void UpdateAll();

        // 描画処理
        void DrawAll();

        // デバック更新処理
        void DebugUpdateAll();

        // 解放
        void ClearAll() { objects_.clear(); }

    private:
        // オブジェクトデータ
        std::vector<std::unique_ptr<IGameObject>> objects_;

        // ループ中に追加されたオブジェクト
        std::vector<std::unique_ptr<IGameObject>> pendingObjects_;

        // objects_が回っているか
        bool isIterating_ = false;
        // ソートが必要か
        bool needsSort_ = false;

    private:

        /// <summary>
        /// 待機リストを本体へ移す
        /// </summary>
        void FlushPendingObjects();
    };
}
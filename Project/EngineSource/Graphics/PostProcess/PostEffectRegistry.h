#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "IPostEffect.h"
#include "PSO/Core/PSOManager.h"

namespace GameEngine {

    // 重複しない名前を作る
    template <class T>
    std::string MakeUniqueName(const std::string& base, const T& existing) {
        if (!existing.contains(base)) { return base; }
        for (int i = 1;; ++i) {
            std::string s = base + "_" + std::to_string(i);
            if (!existing.contains(s)) { return s; }
        }
    }

    class PostEffectRegistry {
    public:
        // 種類
        template <class T>
        void RegisterType(const std::string& typeName, PSOManager* psoManager) {
            psoList_[typeName] = psoManager->GetDrawPsoData(typeName);
            factories_[typeName] = [typeName]() {
                auto e = std::make_unique<T>();
                e->Register(0, typeName);
                return e;
                };
        }
        bool HasType(const std::string& typeName) const { return factories_.contains(typeName); }
        std::vector<std::string> GetTypeNames() const;
        // インスタンスを作るだけ (登録はしない)。未登録の種類ならnullptr
        std::unique_ptr<IPostEffect> CreateInstance(const std::string& typeName) const;
        const DrawPsoData* FindPso(const std::string& psoName) const;

        // 作成
        IPostEffect* Create(const std::string& typeName, const std::string& baseName, std::string& outName);
        IPostEffect* Find(const std::string& name) const;
        // このノードのエフェクトが登録済みか
        bool Owns(const std::string& name, const IPostEffect* fx) const;
        const std::unordered_map<std::string, std::unique_ptr<IPostEffect>>& GetAll() const { return effects_; }

        // 遅延破棄へ送る
        void Destroy(const std::string& name);
        void RetireAll();
        // ロード結果で丸ごと入れ替える
        void Replace(std::unordered_map<std::string, std::unique_ptr<IPostEffect>>&& newEffects);
        // 毎フレーム呼ぶ。遅延破棄のカウントを進める
        void Tick();

    private:
        // フレームバッファ数以上にする
        static constexpr int kRetireFrames = 3;
        struct Retired { std::unique_ptr<IPostEffect> fx; int framesLeft; };

        std::unordered_map<std::string, std::function<std::unique_ptr<IPostEffect>()>> factories_;
        std::unordered_map<std::string, DrawPsoData> psoList_;
        std::unordered_map<std::string, std::unique_ptr<IPostEffect>> effects_;
        std::vector<Retired> retired_;
    };
}
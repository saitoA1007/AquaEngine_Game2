#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <stdexcept>
#include "IEngineSubsystem.h"

namespace GameEngine {

    /// <summary>
    /// システムの実行順序を管理する
    /// </summary>
    class SubsystemRegistry {
    public:
        struct Entry {
            std::string           name;
            IEngineSubsystem* system = nullptr;
        };

        /// <summary>
        /// サブシステムを登録する（呼んだ順が初期化順になる）
        /// </summary>
        void Register(const std::string& name, IEngineSubsystem* subsystem) {
            entries_.push_back({ name, subsystem });
        }

        /// <summary>
        /// 全サブシステムを登録順に初期化
        /// </summary>
        void InitializeAll() {
            for (auto& e : entries_) {
                e.system->Initialize();
            }
        }

        /// <summary>
        /// 全サブシステムを登録順に更新
        /// </summary>
        void UpdateAll() {
            for (auto& e : entries_) {
                e.system->Update();
            }
        }

        /// <summary>
        /// 全サブシステムを登録の逆順に終了
        /// </summary>
        void FinalizeAll() {
            for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
                it->system->Finalize();
            }
        }

    private:
        std::vector<Entry> entries_;
    };
}
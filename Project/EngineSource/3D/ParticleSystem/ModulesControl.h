#pragma once
#include <functional>
#include <unordered_map>
#include "DebugParameter.h"
#include "ParticleData.h"
#include "ParticleModules.h"

namespace GameEngine {

	class ModulesControl {
	public:

		struct ModuleData {
			bool isActive = false;
			bool isCreated = false;
			std::string mainGroupName = "null";
			std::function<std::unique_ptr<IParticleModule>()> maker;
		};

	public:
		ModulesControl(DebugParameter* param);

		/// <summary>
		/// 管理機能の更新
		/// </summary>
		void Update();

		/// <summary>
		/// パーティクルの作成
		/// </summary>
		/// <param name="particleData"></param>
		void ParticleCreate(ParticleData& particleData);

		/// <summary>
		/// パーティクルの更新
		/// </summary>
		/// <param name="particleData"></param>
		/// <param name="time"></param>
		void ParticleUpdate(ParticleData& particleData, float time);

		// モジュールを取得
		template<typename T>
		T* GetModule(const std::string& moduleName) {
			auto it = modules_.find(moduleName);
			if (it != modules_.end()) {
				return dynamic_cast<T*>(it->second.get());
			}
			return nullptr;
		}

	private:
		DebugParameter* param_ = nullptr;

		// モジュール
		std::unordered_map<std::string, std::unique_ptr<IParticleModule>> modules_;

		// モジュールの管理
		std::unordered_map<std::string, ModuleData> enableModules_;

	private:
		// モジュールを登録
		template<typename T>
		void RegisterModule(const std::string& mainGroupName, const std::string& moduleName) {
			// 登録
			ModuleData moduleData;
			moduleData.mainGroupName = mainGroupName;
			static_assert(std::is_base_of<IParticleModule, T>::value, "T must derive from IParticleModule");
			moduleData.maker = []() { return std::make_unique<T>(); };
			enableModules_[moduleName] = moduleData;

			// パラメータに登録
			param_->Register("Enable", enableModules_[moduleName].isActive,0, mainGroupName + "/" + moduleName);
		}
	};

}
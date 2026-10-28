#pragma once
#include "DebugParameter.h"
#include "ParticleData.h"

namespace GameEngine {

	struct MainModule {
		bool isLoop = true; // ループするか
		bool isBillBoard = false; // ビルボードを使用するか判断する
		bool isActiveBlendAdd_ = true; // 加算モードを有効

		uint32_t spawnMaxCount = 1; // 出現する数
		float spawnCoolTime = 1.0f; // 発生する間隔
		float lifeTime = 1.0f; // 生存時間

		bool isEmitByDistance = false; // 距離間隔で発生の有効化フラグ
		float spawnDistance = 0.5f;    // 何m動くごとに発生するか

		Vector3 emitterPos = {0.0f,0.0f,0.0f}; // 発生位置
		Vector3 rotate = { 0.0f,0.0f,0.0f };
		Vector3 scale = { 1.0f,1.0f,1.0f };

		Vector4 color = { 1.0f,1.0f,1.0f,1.0f }; // 色
	};

	// パーティクルの拡張機能の基底クラス
	class IParticleModule {
	public:
		virtual ~IParticleModule() = default;

		void SetGroupName(const std::string& groupName, const std::string mainSubGroupName) {
			groupName_ = groupName;
			mainSubGroupName_ = mainSubGroupName;
		}

		// 値を登録
		virtual void Register(DebugParameter* param) = 0;
		// 値を解除。無効かした時の動きをためしたいだけなのにパラメータ自体を消してしまったら出来ないので一旦保留
		virtual void Remove(DebugParameter* param) {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveGroup(subGroup);
		}

		virtual void Create([[maybe_unused]]ParticleData& particleData) { return; }
		virtual void Update([[maybe_unused]]ParticleData& particleData, [[maybe_unused]]float time) { return; }
		
	protected:
		std::string groupName_ = "null";
		std::string mainSubGroupName_ = "null";
		ParticleData* particleData_ = nullptr;
		bool isActiveCreate_ = false;
		bool isActiveUpdate_ = false;
	};
}
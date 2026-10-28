#pragma once
#include "ParticleModules.h"

namespace GameEngine {

	// 前方宣言
	class TextureManager;

	// テクスチャ
	class TextureModule : public IParticleModule {
	public:
		~TextureModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Texture", textureData_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Texture", subGroup);
		}

		void Create(ParticleData& particleData) override {
			// テクスチャを設定
			particleData.textureHandle = textureData_.handle;
		}

		// 名前からテクスチャハンドルを取得
		void SetTexture(TextureManager* textureManager);

	private:
		TextureData textureData_;
	};

	// 速度の生成
	class VelocityEmitModule : public IParticleModule {
	public:
		~VelocityEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("RangeVelocity", velocityRange_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("RangeVelocity", subGroup);
		}

		void Create(ParticleData& particleData) override;

	private:
		Range3 velocityRange_;
	};

	// 方向を指定した速度の生成
	class DirectionEmitModule : public IParticleModule {
	public:
		~DirectionEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Direction", direction_, index++, subGroup);
			param->Register("MinSpeed", minSpeed_, index++, subGroup);
			param->Register("MaxSpeed", maxSpeed_, index++, subGroup);
			param->Register("SpreadAngle", spreadAngle_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Direction", subGroup);
			param->RemoveItem("MinSpeed", subGroup);
			param->RemoveItem("MaxSpeed", subGroup);
			param->RemoveItem("SpreadAngle", subGroup);
		}

		void Create(ParticleData& particleData) override;

		// 外部から基準方向を設定する
		void SetDirection(const Vector3& direction) { direction_ = direction; }

	private:
		// 基準となる方向
		Vector3 direction_ = { 0.0f, 0.0f, 1.0f };
		// 速さの範囲
		float minSpeed_ = 1.0f;
		float maxSpeed_ = 1.0f;
		// 基準方向からのばらつき角度
		float spreadAngle_ = 0.0f;
	};

	// 回転の生成
	class RotateEmitModule : public IParticleModule {
	public:
		~RotateEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("RangeRotate", rotateRange_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("RangeRotate", subGroup);
		}

		void Create(ParticleData& particleData) override;

	private:
		Range3 rotateRange_;
	};

	// サイズの生成
	class ScaleEmitModule : public IParticleModule {
	public:
		~ScaleEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("SeparateAxes", separateAxes_, index++, subGroup);
			param->Register("RangeScale", scaleRange_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("SeparateAxes", subGroup);
			param->RemoveItem("RangeScale", subGroup);
		}

		void Create(ParticleData& particleData) override;

	private:
		Range3 scaleRange_;
		bool separateAxes_ = false;
	};

	// 回転速度
	class RotateVelocityEmitModule : public IParticleModule {
	public:
		~RotateVelocityEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("RangeRotateVelocity", velocityRange_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("RangeRotateVelocity", subGroup);
		}

		void Create(ParticleData& particleData) override;

	private:
		Range3 velocityRange_;
	};

	// 発射形状
	class ShapeEmitModule : public IParticleModule {
	public:
		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Shape", emitterShape_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Shape", subGroup);
		}

		void Create(ParticleData& particleData) override;

		void SetRadius(float radius) {
			emitterShape_.radius = radius;
		}

	private:
		// 形状
		EmitterShape emitterShape_;
	};

	// 色
	class ColorEmitModule : public IParticleModule {
	public:
		~ColorEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("MinColor", minColor_, index++, subGroup);
			param->Register("MaxColor", maxColor_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("MinColor", subGroup);
			param->RemoveItem("MaxColor", subGroup);
		}

		void Create(ParticleData& particleData) override;

	private:
		Vector4 minColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		Vector4 maxColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	// 生存時間の生成
	class LifeTimeEmitModule : public IParticleModule {
	public:
		~LifeTimeEmitModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("MinLifeTime", minLifeTime_, index++, subGroup);
			param->Register("MaxLifeTime", maxLifeTime_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("MinLifeTime", subGroup);
			param->RemoveItem("MaxLifeTime", subGroup);
		}

		void Create(ParticleData& particleData) override;

		// 外部から生存時間の範囲を設定する
		void SetLifeTime(float minLifeTime, float maxLifeTime) {
			minLifeTime_ = minLifeTime;
			maxLifeTime_ = maxLifeTime;
		}

	private:
		// 生存時間の範囲
		float minLifeTime_ = 1.0f;
		float maxLifeTime_ = 1.0f;
	};
}
#pragma once
#include "ParticleModules.h"
#include "EasingManager.h"

namespace GameEngine {

	// 速度変化
	class VelocityOverLifeTimeModule : public IParticleModule {
	public:
		~VelocityOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EaseCurve", easeCurve_, index++, subGroup);
			param->Register("EndVelocity", endVelocity_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EaseCurve", subGroup);
			param->RemoveItem("EndVelocity", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;

	private:
		Vector3 endVelocity_;
		EaseCurve easeCurve_;
	};

	// サイズを変化させる
	class SizeOverLifeTimeModule : public IParticleModule {
	public:
		~SizeOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EaseCurve", easeCurve_, index++, subGroup);
			param->Register("SeparateAxesEndSize", separateAxesEndSize_, index++, subGroup);
			param->Register("EndSize", endSize_, index++, subGroup);
			param->Register("SeparateAxes", separateAxes_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EaseCurve", subGroup);
			param->RemoveItem("SeparateAxesEndSize", subGroup);
			param->RemoveItem("EndSize", subGroup);
			param->RemoveItem("SeparateAxes", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;

		void SetEndScale(float scale) {
			endSize_ = scale;
		}

	private:
		float endSize_ = 0.0f;
		Vector3 separateAxesEndSize_ = {};
		// 各軸制御
		bool separateAxes_ = false;
		EaseCurve easeCurve_;
	};

	// 透明度補間
	class AlphaOverLifeTimeModule : public IParticleModule {
	public:
		~AlphaOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EaseCurve", easeCurve_, index++, subGroup);
			param->Register("EndAlpha", endAlpha_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EaseCurve", subGroup);
			param->RemoveItem("EndAlpha", subGroup);
		}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;

	private:
		float endAlpha_ = 0.0f;
		EaseCurve easeCurve_;
	};

	// 色変化
	class ColorOverLifeTimeModule : public IParticleModule {
	public:
		~ColorOverLifeTimeModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("EndColor", endRGB_, index++, subGroup);
			param->Register("EaseCurve", easeCurve_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("EndColor", subGroup);
			param->RemoveItem("EaseCurve", subGroup);
		}

		void Create(ParticleData& particleData) override;
		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;

		// 外部から色を設定する
		void SetHSV(const Vector3& endRGB) {
			endRGB_ = { endRGB.x,endRGB.y,endRGB.z };
		}

	private:
		// 色相、彩度、明度
		Vector3 startHSV_ = {};
		Vector4 endRGB_ = { 1.0f, 1.0f, 1.0f,1.0f };
		EaseCurve easeCurve_;
	};

	// 引力モジュール
	class AttractionModule : public IParticleModule {
	public:
		~AttractionModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("TargetPos", targetPos_, index++, subGroup);
			param->Register("Strength", strength_, index++, subGroup);
			param->Register("Damping", damping_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("TargetPos", subGroup);
			param->RemoveItem("Strength", subGroup);
			param->RemoveItem("Damping", subGroup);
		}

		void Update(ParticleData& particleData, float time) override;

		// 目標の位置を設定
		void SetTargetPosition(const Vector3& pos) { targetPos_ = pos; }

	private:
		// 吸い込み先の目標位置
		Vector3 targetPos_ = { 0.0f, 0.0f, 0.0f };
		// 引力の強さ
		float strength_ = 10.0f;
		// 速度の減衰率
		float damping_ = 2.0f;
	};

	// らせんモジュール
	class VortexModule : public IParticleModule {
	public:
		~VortexModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("CenterPos", centerPos_, index++, subGroup);
			param->Register("RotationSpeed", rotationSpeed_, index++, subGroup);
			param->Register("AttractionSpeed", attractionSpeed_, index++, subGroup);
			param->Register("AxisSpeed", axisSpeed_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("CenterPos", subGroup);
			param->RemoveItem("RotationSpeed", subGroup);
			param->RemoveItem("AttractionSpeed", subGroup);
			param->RemoveItem("AxisSpeed", subGroup);
		}

		void Update(ParticleData& particleData, float time) override;

		// 外部から渦の中心を動かすためのセッター
		void SetCenterPosition(const Vector3& pos) { centerPos_ = pos; }

	private:
		// 渦の中心座標
		Vector3 centerPos_ = { 0.0f, 0.0f, 0.0f };
		// 回転の勢い
		float rotationSpeed_ = 5.0f;
		// 吸い込みの強さ
		float attractionSpeed_ = 1.0f;
		// 上昇、下降の速度
		float axisSpeed_ = 2.0f;
	};

	// 重力場モジュール
	class GravityFieldModule : public IParticleModule {
	public:
		~GravityFieldModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("Direction", direction_, index++, subGroup);
			param->Register("Strength", strength_, index++, subGroup);
			param->Register("MaxSpeed", maxSpeed_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("Direction", subGroup);
			param->RemoveItem("Strength", subGroup);
			param->RemoveItem("MaxSpeed", subGroup);
		}

		void Update(ParticleData& particleData, float time) override;

		// 外部から重力の方向を設定する
		void SetDirection(const Vector3& direction) { direction_ = direction; }

		// 外部から重力の強さを設定する
		void SetStrength(float strength) { strength_ = strength; }

	private:
		// 重力の方向
		Vector3 direction_ = { 0.0f, -1.0f, 0.0f };
		// 重力の強さ
		float strength_ = 9.8f;
		// 重力方向の速度の上限。0以下なら制限しない
		float maxSpeed_ = 0.0f;
	};

	// 速度方向に回転を向けさせる
	class RotationByVelocityModule : public IParticleModule {
	public:
		~RotationByVelocityModule() = default;

		void Register([[maybe_unused]] DebugParameter* param) override {}

		void Remove([[maybe_unused]] DebugParameter* param) override {}

		void Update(ParticleData& particleData, [[maybe_unused]] float time) override;
	};

	// トレイル
	class TrailModule : public IParticleModule {
	public:
		~TrailModule() = default;

		void Register(DebugParameter* param) override {
			int index = 1;
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->Register("TrailLength", trailLength_, index++, subGroup);
			param->Register("RecordInterval", recordInterval_, index++, subGroup);
			param->Register("StartScale", startScale_, index++, subGroup);
			param->Register("EndScale", endScale_, index++, subGroup);
			param->Register("StartAlpha", startAlpha_, index++, subGroup);
			param->Register("EndAlpha", endAlpha_, index++, subGroup);
			param->Register("TrailColor", trailColor_, index++, subGroup);
			param->Register("EaseCurve", easeCurve_, index++, subGroup);
		}

		void Remove(DebugParameter* param) override {
			std::string subGroup = groupName_ + "/" + mainSubGroupName_;
			param->RemoveItem("TrailLength", subGroup);
			param->RemoveItem("RecordInterval", subGroup);
			param->RemoveItem("StartScale", subGroup);
			param->RemoveItem("EndScale", subGroup);
			param->RemoveItem("StartAlpha", subGroup);
			param->RemoveItem("EndAlpha", subGroup);
			param->RemoveItem("TrailColor", subGroup);
			param->RemoveItem("EaseCurve", subGroup);
		}

		void Create(ParticleData& particleData) override;
		void Update(ParticleData& particleData, float time) override;

		/// <summary>
		/// 軌跡のindex番目の大きさと色を求める
		/// </summary>
		/// <param name="particleData">元のパーティクル</param>
		/// <param name="index">軌跡の番号</param>
		/// <param name="outScale">大きさ</param>
		/// <param name="outColor">色</param>
		void CalcTrailPoint(const ParticleData& particleData, uint32_t index, Vector3& outScale, Vector4& outColor) const;

		// 描画する軌跡の数
		uint32_t GetTrailLength() const;

		// 外部から軌跡の長さを設定する
		void SetTrailLength(uint32_t length) { trailLength_ = length; }

	private:
		// 軌跡の数
		uint32_t trailLength_ = 8;
		// 位置を記録する間隔。0以下なら毎フレーム記録
		float recordInterval_ = 0.02f;
		// パーティクルに対する大きさの割合
		float startScale_ = 1.0f;
		float endScale_ = 0.0f;
		// パーティクルに対する透明度の割合
		float startAlpha_ = 1.0f;
		float endAlpha_ = 0.0f;
		// 最も古い軌跡の目標色。パーティクルの色からHSV空間で補間する
		Vector4 trailColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
		EaseCurve easeCurve_;
	};

}
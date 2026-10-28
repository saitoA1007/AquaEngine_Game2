#pragma once
#pragma once
#include"EngineSource/Math/Vector4.h"
#include"EngineSource/Math/Vector3.h"
#include<cstdint>

namespace GameEngine {

	class SpotLight {
	public:

		struct alignas(16) SpotLightData {
			Vector4 color; // ライトの色
			Vector3 position; // ライトの位置
			float intensity; // 輝度
			Vector3 direction; // スポットライトの方向
			float distance; // ライトの届く最大距離
			float decay; // 減衰率
			float cosAngle; // スポットライトの余弦
			float cosFalloffStart; // 光の角度
			int32_t active; // ライトの使用
		};

	public:
		SpotLight() = default;
		~SpotLight() = default;

		/// <summary>
		/// 初期化
		/// </summary>
		void Initialize( const Vector4& color, const Vector3& direction, const float& intensity);

		/// <summary>
		/// ライト位置をセット
		/// </summary>
		/// <param name="lightdir">ライト方向</param>
		void SetLightPosition(const Vector3& position);

		/// <summary>
		/// ライト位置を取得
		/// </summary>
		/// <returns>ライト方向</returns>
		const Vector3& GetLightPosition() const { return spotLightData_.position; }

		/// <summary>
		/// ライト色をセット
		/// </summary>
		/// <param name="lightcolor">ライト色</param>
		void SetLightColor(const Vector4& lightcolor) { spotLightData_.color = lightcolor; }

		/// <summary>
		/// ライト色を取得
		/// </summary>
		/// <returns>ライト色</returns>
		const Vector4& GetLightColor() const { return spotLightData_.color; }

		/// <summary>
		/// ライトの明度をセット
		/// </summary>
		/// <param name="lightIntensity"></param>
		void SetLightIntensity(const float& lightIntensity) { spotLightData_.intensity = lightIntensity; }

		/// <summary>
		/// ライトの明度を取得
		/// </summary>
		/// <returns></returns>
		const float GetLightIntensity() const { return spotLightData_.intensity; }

		/// <summary>
		/// ライトの有効化を設定
		/// </summary>
		/// <param name="actice"></param>
		void SetLightActive(const bool& actice) { spotLightData_.active = actice; }

		/// <summary>
		/// ライトの方向
		/// </summary>
		/// <param name="direction"></param>
		void SetLighDirection(const Vector3& direction) { spotLightData_.direction = direction; }

		/// <summary>
		/// ライトの適応距離
		/// </summary>
		/// <param name="distance"></param>
		void SetDistance(const float& distance) { spotLightData_.distance = distance; }

		/// <summary>
		/// 減衰率
		/// </summary>
		/// <param name="decay"></param>
		void SetDecay(const float& decay) { spotLightData_.decay = decay; }

		/// <summary>
		/// ライトの入射角度
		/// </summary>
		/// <param name="cosFalloffStart"></param>
		void SetCosFalloffStart(const float& cosFalloffStart) { spotLightData_.cosFalloffStart = cosFalloffStart; }

		/// <summary>
		/// ライトデータを適応
		/// </summary>
		/// <param name="spotLightData"></param>
		void SetSpotLightData(const SpotLightData& spotLightData) { spotLightData_ = spotLightData; }

		SpotLightData& GetSpotLightData() { return spotLightData_; }

	private:

		// スポットライトのデータを作る
		SpotLightData spotLightData_;
	};
}
#pragma once
#include "ConstantBuffer.h"
#include <memory>
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"

namespace GameEngine {

    class LightManager {
    public:
        // 定数バッファ
        struct LightGroupData {
            DirectionalLight::DirectionalLightData directionalLightData_;
            PointLight::PointLightData pointLightData_;
            SpotLight::SpotLightData spotLightData_;
            uint32_t environmentTexture = 0;
            int32_t isActiveEnvironment = false;
            float padding[2];
        };

    public:
        LightManager() = default;
        ~LightManager();

        /// <summary>
        /// 初期化
        /// </summary>
        /// <param name="device"></param>
        void Initialize(const bool& isDirectionalActive,const bool& isPointActive,const bool& isSpotActive);

        /// <summary>
        /// 更新処理
        /// </summary>
        void Update();

    public:

        /// <summary>
        /// 平行光源のデータ設定
        /// </summary>
        /// <param name="directionalData"></param>
        void SetDirectionalData(const DirectionalLight::DirectionalLightData& directionalData);

        void Setshadow(const Vector3& targetCenter, float shadowRange);

        /// <summary>
        /// 平行光源の方向
        /// </summary>
        /// <param name="lightdir"></param>
        void SetDirectionalDirction(const Vector3& lightdir);

        void SetDirectionalIntensity(const float& intensity);

        void SetDirectionalColor(Vector4 color);

        /// <summary>
        /// 平行光源の有効化
        /// </summary>
        /// <param name="active"></param>
        void SetDirectionalLightActive(const bool& active);

        /// <summary>
        /// 点光源のデータ設定
        /// </summary>
        /// <param name="pointData"></param>
        void SetPointData(const PointLight::PointLightData& pointData);

        /// <summary>
        /// 点光源の位置
        /// </summary>
        /// <param name="position"></param>
        void SetPointLightPosition(const Vector3& position);

        /// <summary>
        /// 点光源の有効化
        /// </summary>
        /// <param name="active"></param>
        void SetPointLightActive(const bool& active);

        /// <summary>
        /// スポットライトのデータ設定
        /// </summary>
        /// <param name="spotData"></param>
        void SetSpotData(const SpotLight::SpotLightData& spotData);

        /// <summary>
        /// スポットライトの位置
        /// </summary>
        /// <param name="position"></param>
        void SetSpotLightPosition(const Vector3& position);

        /// <summary>
        /// スポットライトの有効化
        /// </summary>
        /// <param name="active"></param>
        void SetSpotLightActive(const bool& active);

        /// <summary>
        /// 環境マップを設定
        /// </summary>
        /// <param name="index"></param>
        void SetEnvironmentTexture(const uint32_t& index) {
            lightGroupData_->environmentTexture = index;
            lightGroupData_->isActiveEnvironment = true;
        }

        ID3D12Resource* GetResource() const { return constBuffer_.GetResource(); }
        ConstantBuffer<LightGroupData>* GetConstantBuffer() { return &constBuffer_; }

        std::unique_ptr<DirectionalLight> directionalLight_;
        std::unique_ptr<PointLight> pointLight_;
        std::unique_ptr<SpotLight> spotLight_;

    private:
        ConstantBuffer<LightGroupData> constBuffer_;
        // 平行光源のデータを作る
        LightGroupData* lightGroupData_ = nullptr;
    };

}

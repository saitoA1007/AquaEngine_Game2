#pragma once
#include <vector>
#include <map>
#include <unordered_map>

#include "DrawRequest.h"
#include "RenderPass/RenderPassController.h"

#include "RefBuffer.h"
#include "Camera.h"
#include "LightManager.h"
#include "DirectionalLight.h"
#include "TLAS.h"

#include "PSO/Core/BlendMode.h"

namespace GameEngine {

    class FractureStaticMesh;

    /// <summary>
    /// 溜めた描画コマンドを解放する機能
    /// </summary>
    class RenderQueue {
    public:
        struct WBOITData {
            float nearPlane; // ニアプレーン距離
            float farPlane; // ファープレーン距離
            float alphaThreshold; // 棄却アルファ閾値
            float depthPow; // 深度感度指数

            float weightMin; // 重みの下限
            float weightMax; // 重みの上限
            float pad[2];
        };

        struct PerView
        {
            Matrix4x4 viewProjection;
            Matrix4x4 billboardMatrix;
        };

    public:
        RenderQueue();
        ~RenderQueue() = default;

        // 初期化処理
        void Initialize();

        void Update();
     
    public:

        void SetCamera(Camera* camera) {
            cameraPtr_ = camera;
        }

        void SetDebugCamera(GpuResource* cameraResource) {
            debugCameraResource_ = cameraResource;  
        }

        void SetUseDebugCamera(const bool& useDebugCamera,const Matrix4x4& vpMatrix4x4, const Matrix4x4& worldMatrix) {
            useDebugCamera_ = useDebugCamera;

            if (useDebugCamera_) {
                auto* perViewData = perViewData_.GetData();
                perViewData->viewProjection = vpMatrix4x4;

                // ビルボードの回転行列を作成
                //Matrix4x4 backToFrontMatrix = Math::MakeRotateYMatrix(0.0f);
                //Matrix4x4 billboardMatrix = backToFrontMatrix * worldMatrix;
                //billboardMatrix.m[3][0] = 0.0f;
                //billboardMatrix.m[3][1] = 0.0f;
                //billboardMatrix.m[3][2] = 0.0f;

                perViewData->billboardMatrix = worldMatrix;
            }
        }

        // デバックカメラのワールド行列を取得
        Matrix4x4 GetDebugCameraWorldMatrix() const {
            auto* perViewData = perViewData_.GetData();
            return perViewData->billboardMatrix;
        }

        const bool& GetUseDebugCamera() const { return useDebugCamera_; }

        // カメラリソースを取得
        GpuResource* GetCameraResource() { return mainCamera_.GetConstantBuffer(); }

        // カメラを取得
        Camera& GetMainCamera() { return mainCamera_; }

        // デバックカメラリソースを取得
        GpuResource* GetDebugCameraResource() { return debugCameraResource_; }

        // ライトリソースを取得
        GpuResource* GetLightResource() { return lightManager_.GetConstantBuffer(); }

        // ライト管理機能を取得
        LightManager* GetLightManager() { return &lightManager_; }

        // wboitリソースを取得
        GpuResource* GetWboitResource() { return &wboitData_; }

        // Csパーティクル用のカメラリソース
        GpuResource* GetPerViewResource() { return &perViewData_; }

        // 背景画像ハンドルを設定する
        void SetSkyboxTexture(const uint32_t& texture) {
            skyboxTextureIndex_ = texture;
            lightManager_.SetEnvironmentTexture(texture);
        }

        const uint32_t& GetSkyboxTexture() const { return skyboxTextureIndex_; }

    public:

        // 画像描画
        void SubmitSprite(const Sprite* sprite, const std::string& passName = "DefaultPass");

        // テキスト描画
        void SubmitText(const Text* text, const std::string& passName = "DefaultPass");

        /// 通常モデル（ライトあり）
        void SubmitModel(const Model* model,WorldTransform& worldTransform,const float& alpha = 1.0f, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");
        void SubmitAddModel(const Model* model,WorldTransform& worldTransform,const float& alpha = 1.0f, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        /// インスタンシング描画
        void SubmitInstancing(const Model* model,uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha = 1.0f, BlendMode blendMode = BlendMode::kBlendModeNormal, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        void SubmitInstancingWboit(const Model* model,uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha = 1.0f, BlendMode blendMode = BlendMode::kBlendModeNormal, const GpuResource* material = nullptr, const std::string& passName = "WBOITAccumulatePass");

        void SubmitParticleCS(const Model* model, uint32_t numInstances, SrvResource* particle, const float& alpha = 1.0f, const std::string& passName = "DefaultPass");

        /// スケルタルアニメーション
        void SubmitAnimation(const Model* model, WorldTransform& worldTransform, const float& alpha = 1.0f, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        /// スカイボックス
        void SubmitSkybox(const Model* model, WorldTransform& worldTransform, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        /// シャドウマップ
        void SubmitShadowMap(const Model* model, WorldTransform& worldTransform, const std::string& passName = "ShadowPass");

        /// グリッド
        void SubmitGrid(const Model* model,WorldTransform& worldTransform, const std::string& passName = "DefaultPass");

        // デバック用ライン
        void SubmitDebugLine(const DebugRenderer* debugRenderer, const std::string& passName = "DefaultPass");

        // レイトレーシングでのモデル
        void SubmitRaytracingModel(Model* model, WorldTransform& worldTransform, std::vector<RefBuffer>* customRefBuffer = {});

        // 破片の描画
        void SubmitFracture(const Model* model, FractureInstance& fractureInstance, const float& alpha = 1.0f, const GpuResource* material = nullptr, const std::string& passName = "DefaultPass");

        // レイトレの破片を描画
        void SubmitRaytracingFracture(Model* model, FractureInstance& fractureInstance, WorldTransform& worldTransform);

        // 静的なチャンクを1つにまとめたメッシュをレイトレで描画
        void SubmitRaytracingFractureStaticMesh(const FractureStaticMesh& staticMesh, WorldTransform& worldTransform);

        void SubmitRuntimeCutFragments(FractureInstance& fractureInstance, const GpuResource* material, const float& alpha = 1.0f, const std::string& passName = "DefaultPass");

        void SubmitRuntimeCutIceFragments(FractureInstance& fractureInstance, const GpuResource* material, const float& alpha = 1.0f, const std::string& passName = "DefaultPass");

        // psoの名前を取得
        const char* Get3dPsoName(Draw3dType type);
        const char* Get2dPsoName(Draw2dType type);

        // 描画コマンドのクリア
        void Clear() {
            draw2dQueueList_.clear();
            draw3dQueueList_.clear();
            translucentDrawQueueList_.clear();
            raytracingDrawQueueList_.clear();
        }

        const std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw3dRequest>>>>& GetDraw3dQueue() const { return draw3dQueueList_; }
        const std::map<std::string, std::vector<Draw3dRequest>>& GetTranslucentQueue() const { return translucentDrawQueueList_; }
        const std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw2dRequest>>>>& GetDraw2dQueue() const { return draw2dQueueList_; }
        const std::vector<TLASInstanceData>& GetRaytracingQueue()  const { return raytracingDrawQueueList_; }

        /// レイトレーシングに積まれた描画コマンドが存在するか
        bool HasRaytracingDrawCalls() const { return !raytracingDrawQueueList_.empty(); }

    private:
        // コピー、ムーブは禁止
        RenderQueue(const RenderQueue&) = delete;
        RenderQueue& operator=(const RenderQueue&) = delete;
        RenderQueue(RenderQueue&&) = default;
        RenderQueue& operator=(RenderQueue&&) = default;

        // 2D描画コマンドのスタックメモリ [描画パス]->[PSO]->[描画コマンド]
        std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw2dRequest>>>> draw2dQueueList_;
        // 3D描画コマンドのスタックメモリ [描画パス]->[PSO]->[描画コマンド]
        std::map<std::string, std::map<RenderLayer, std::unordered_map<std::string, std::vector<Draw3dRequest>>>> draw3dQueueList_;
        // 半透明の描画コマンドのスタックメモリ
        std::map<std::string, std::vector<Draw3dRequest>> translucentDrawQueueList_;
        // レイトレーシングの描画コマンド
        std::vector<TLASInstanceData> raytracingDrawQueueList_;

        // カメラリソース
        Camera mainCamera_;
        Camera* cameraPtr_ = nullptr;
        GpuResource* debugCameraResource_ = nullptr;
        // ライトリソース
        LightManager lightManager_;
        // 平行光源
        DirectionalLight::DirectionalLightData directionalData_;
        // 背景画像ハンドル
        uint32_t skyboxTextureIndex_ = 0;

        // wboitデータ
        ConstantBuffer<WBOITData> wboitData_;
        // パーティクル用のデータ
        ConstantBuffer<PerView> perViewData_;

        // デバックカメラを使用するか
        bool useDebugCamera_ = false;
    };
}
#include "pch.h"
#include "RenderQueue.h"
#include "DebugRenderer.h"
#include "Sprite.h"
#include "Material.h"
#include "Model.h"
#include "WorldTransform.h"
#include "WorldTransforms.h"
#include "FractureInstance.h"
#include "FractureStaticMesh.h"
#include "DrawRequest.h"
#include "MyMath.h"
using namespace GameEngine;

RenderQueue::RenderQueue() {

}

void RenderQueue::Initialize() {
    // カメラを設定
    mainCamera_.Initialize({ {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,-10.0f} }, 1280, 720);

    // 平行光源ライト
    directionalData_.active = true;
    directionalData_.color = { 1.0f,1.0f,1.0f,1.0f };
    directionalData_.direction = { 0.0,-1.0f,0.0f };
    directionalData_.intensity = 1.0f;

    // ライトの設定
    lightManager_.Initialize(true, false, false);
    lightManager_.SetDirectionalData(directionalData_);

    // wboitのデータ
    wboitData_.Create();
    auto* wboitData =  wboitData_.GetData();
    wboitData->nearPlane = 0.01f; // ニアプレーン距離
    wboitData->farPlane = 200.0f; // ファープレーン距離
    wboitData->alphaThreshold = 1.0f / 255.0f; // 棄却アルファ閾値
    wboitData->depthPow = 4.0f; // 深度感度指数
    wboitData->weightMin = 0.01f; // 重みの下限
    wboitData->weightMax = 1000.0f; // 重みの上限

    // CSパーティクル用
    perViewData_.Create();
    auto* perViewData = perViewData_.GetData();
    perViewData->billboardMatrix = Matrix4x4::MakeIdentity();
    perViewData->viewProjection = Matrix4x4::MakeIdentity();
}

void RenderQueue::Update() {
    // カメラを設定する
    if (cameraPtr_ != nullptr) {
        mainCamera_.SetCamera(*cameraPtr_);

        if (!useDebugCamera_) {
            auto* perViewData = perViewData_.GetData();
            perViewData->viewProjection = mainCamera_.GetVPMatrix();

            // ビルボードの回転行列を作成
            Matrix4x4 backToFrontMatrix = Matrix4x4::MakeRotateYMatrix(0.0f);
            Matrix4x4 billboardMatrix = backToFrontMatrix * mainCamera_.GetWorldMatrix();
            billboardMatrix.m[3][0] = 0.0f;
            billboardMatrix.m[3][1] = 0.0f;
            billboardMatrix.m[3][2] = 0.0f;

            perViewData->billboardMatrix = billboardMatrix;
        }
    }

    // ライトの更新
    lightManager_.Update();
}

void RenderQueue::SubmitSprite(const Sprite* sprite, const std::string& passName) {

    Draw2dRequest request;
    request.type = Draw2dType::Normal;
    request.layer = RenderLayer::Sprite;
    request.sprite = sprite;
    // 登録
    draw2dQueueList_[passName][request.layer][Get2dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitText(const Text* text, const std::string& passName) {

    Draw2dRequest request;
    request.type = Draw2dType::Text;
    request.layer = RenderLayer::Sprite;
    request.text = text;
    // 登録
    draw2dQueueList_[passName][request.layer][Get2dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitModel(const Model* model, WorldTransform& worldTransform, const float& alpha, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::Default;
    request.layer = RenderLayer::Opaque;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitAddModel(const Model* model, WorldTransform& worldTransform, const float& alpha, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::DefaultAdd;
    request.layer = RenderLayer::Opaque;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitInstancing(const Model* model, uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha, BlendMode blendMode, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.layer = RenderLayer::Opaque;
    // ブレンド設定
    switch (blendMode)
    {
    case GameEngine::kBlendModeNormal:
    default:
        request.type = Draw3dType::Instancing;
        break;

    case GameEngine::kBlendModeNone:
    case GameEngine::kBlendModeAdd:
    case GameEngine::kBlendModeSubtract:
    case GameEngine::kBlendModeMultily:
    case GameEngine::kBlendModeScreen:
        request.type = Draw3dType::InstancingAdd;
        break;
    }

    request.passName = passName;
    request.model = model;
    request.numInstances = numInstances;
    request.worldTransforms = &worldTransforms;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitInstancingWboit(const Model* model, uint32_t numInstances, WorldTransforms& worldTransforms, const float& alpha, BlendMode blendMode, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.layer = RenderLayer::Opaque;
    // ブレンド設定
    switch (blendMode)
    {
    case GameEngine::kBlendModeNormal:
    default:
        request.type = Draw3dType::InstancingWboit;
        break;

    case GameEngine::kBlendModeNone:
    case GameEngine::kBlendModeAdd:
    case GameEngine::kBlendModeSubtract:
    case GameEngine::kBlendModeMultily:
    case GameEngine::kBlendModeScreen:
        request.type = Draw3dType::InstancingWboit;
        break;
    }

    request.passName = passName;
    request.model = model;
    request.numInstances = numInstances;
    request.worldTransforms = &worldTransforms;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitParticleCS(const Model* model, uint32_t numInstances, SrvResource* particle, const float& alpha, const std::string& passName) {
    Draw3dRequest request;
    request.layer = RenderLayer::Opaque;
    request.type = Draw3dType::ParticleCS;
    request.passName = passName;
    request.model = model;
    request.numInstances = numInstances;
    request.particleCsResource = particle;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitAnimation(const Model* model, WorldTransform& worldTransform, const float& alpha, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    if (model->IsSkeleton()) {
        request.type = Draw3dType::Animation;
        request.layer = RenderLayer::Animation;
    } else {
        request.type = Draw3dType::Default;
        request.layer = RenderLayer::Opaque;
    }
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitSkybox(const Model* model, WorldTransform& worldTransform, const GpuResource* material, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::Skybox;
    request.layer = RenderLayer::Skybox;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;
    request.material = material;

    // 不透明描画に登録
    draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitShadowMap(const Model* model, WorldTransform& worldTransform, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::ShadowMap;
    request.layer = RenderLayer::Shadow;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;

    // 不透明描画に登録
    draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitGrid(const Model* model, WorldTransform& worldTransform, const std::string& passName) {
    Draw3dRequest request;
    request.type = Draw3dType::Grid;
    request.layer = RenderLayer::Grid;
    request.passName = passName;
    request.model = model;
    request.worldTransform = &worldTransform;

    // 不透明描画に登録
    draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitDebugLine(const DebugRenderer* debugRenderer, const std::string& passName) {
    if (!debugRenderer->IsEnabled()) { return; }
    Draw3dRequest request;
    request.type = Draw3dType::DebugLine;
    request.layer = RenderLayer::Debug;
    request.passName = passName;
    request.debugRenderer_ = debugRenderer;

    // 不透明描画に登録
    draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
}

void RenderQueue::SubmitRaytracingModel(Model* model, WorldTransform& worldTransform, std::vector<RefBuffer>* customRefBuffer) {
    // 登録
    const auto& meshes = model->GetMeshes();
    auto& refBuffers = model->GetRefBuffers();

    // 読み込んだモデルのローカル行列を設定
    if (model->IsLoad()) {
        worldTransform.SetWVPMatrix(model->GetLocalMatrix());
    }

    for (uint32_t i = 0; i < meshes.size(); ++i) {
        const auto& mesh = meshes[i];
        auto& refBuffer = refBuffers[i];
        uint32_t refIndex = 0;
        TLASInstanceData data;

        // blasを登録
        data.blas = mesh->GetBLAS();

        if (customRefBuffer == nullptr || i >= customRefBuffer->size()) {
            // デフォルトのマテリアルを設定
            Material* drawMaterial = model->GetMaterial(mesh->GetMaterialName());
            auto& materialBuffer = drawMaterial->GetMaterialBuffer();
            uint32_t type = static_cast<uint32_t>(BufferType::kDefalutMaterial);
            // 半透明であればを分岐
            float alpha = materialBuffer.GetData()->color.w;
            if (alpha < 1.0f) {
                type = static_cast<uint32_t>(BufferType::kDefalutMaterialTranslucent);
            }
            refBuffer.SetBufferMaterial(type, materialBuffer.GetSrvIndex());
            refIndex = refBuffer.GetRefIndex();

            // 使用するヒットグループを設定
            data.hitGroupIndexOffset = refBuffer.GetUseHitGroupIndex();
            // レイキャスト時のフィルタリング用マスクを設定
            data.instanceMask = refBuffer.GetInstanceMask();
        } else {
            auto& customRef = (*customRefBuffer)[i];

            // スケルトンがあれば参照するデータを変える
            uint32_t vertexHandle = 0;
            if (model->IsSkeleton()) {
                const auto& skeleton = model->GetSkeleton();
                vertexHandle = skeleton->GetOutputVertexBufferSrvIndex(i);
            } else {
                vertexHandle = mesh->GetVertexBufferSrvIndex();
            }

            // モデルデータを設定
            customRef.SetModelData(vertexHandle, mesh->GetIndexBufferSrvIndex());
            refIndex = customRef.GetRefIndex();

            // 使用するヒットグループを設定
            data.hitGroupIndexOffset = customRef.GetUseHitGroupIndex();
            // レイキャスト時のフィルタリング用マスクを設定
            data.instanceMask = customRef.GetInstanceMask();
        }

        // 使用するデータを設定
        data.instanceID = refIndex;

        // 座標を設定
        Matrix4x4 matrix = Matrix4x4::Transpose(worldTransform.GetWorldMatrix());
        std::memcpy(&data.transform, &matrix, sizeof(float) * 12);

        // データを登録
        raytracingDrawQueueList_.push_back(std::move(data));
    }
}

void RenderQueue::SubmitFracture(const Model* model, FractureInstance& fractureInstance, const float& alpha, const GpuResource* material, const std::string& passName) {
    
    Draw3dRequest request;
    request.layer = RenderLayer::Opaque;
    // ブレンド設定
    request.type = Draw3dType::Fracture;
    request.passName = passName;
    request.model = model;
    request.fractureInstance = &fractureInstance;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitRaytracingFracture(Model* model, FractureInstance& fractureInstance, WorldTransform& worldTransform) {

    for (auto& [groupName, chunks] : model->GetFractureChunks()) {
        PackedGeometryBuffer* buffer = model->GetFractureBuffers().at(groupName).get();

        if (!buffer->HasBLAS()) { return; }

        uint32_t numInstance = fractureInstance.GetNumInstance();
        for (uint32_t i = 0; i < numInstance; ++i) {
            uint32_t chunkId = fractureInstance.GetChunkId(i);

            BLAS* blas = buffer->GetChunkBLAS(chunkId);
            if (blas == nullptr) { continue; }

            TLASInstanceData data;
            // blasを登録
            data.blas = blas;
            // 使用するヒットグループを設定
            data.hitGroupIndexOffset = 1;
            // 氷のヒットグループを使うため、影レイでも氷として扱う
            data.instanceMask = static_cast<uint32_t>(RayInstanceMask::kRayMaskIce);
            // 使用するデータを設定
            data.instanceID = buffer->GetChunkRefIndex(chunkId);
            // 座標
            Matrix4x4 finalMatrix = fractureInstance.GetChunkWorldMatrix(i) * worldTransform.GetWorldMatrix();
            Matrix4x4 matrix = Matrix4x4::Transpose(finalMatrix);
            std::memcpy(&data.transform, &matrix, sizeof(float) * 12);

            // データを登録
            raytracingDrawQueueList_.push_back(std::move(data));
        }
    }
}

void RenderQueue::SubmitRaytracingFractureStaticMesh(const FractureStaticMesh& staticMesh, WorldTransform& worldTransform) {
    if (!staticMesh.HasGeometry()) { return; }

    TLASInstanceData data;
    // blasを登録
    data.blas = staticMesh.GetBLAS();
    // 使用するヒットグループを設定
    data.hitGroupIndexOffset = 1;
    // 氷のヒットグループを使うため、影レイでも氷として扱う
    data.instanceMask = staticMesh.GetInstanceMask();
    // 使用するデータを設定
    data.instanceID = staticMesh.GetRefIndex();
    // 静的なチャンクは動かないので、オブジェクトのワールド行列をそのまま使う
    Matrix4x4 matrix = Matrix4x4::Transpose(worldTransform.GetWorldMatrix());
    std::memcpy(&data.transform, &matrix, sizeof(float) * 12);

    // データを登録
    raytracingDrawQueueList_.push_back(std::move(data));
}

void RenderQueue::SubmitRuntimeCutFragments(FractureInstance& fractureInstance, const GpuResource* material, const float& alpha, const std::string& passName) {
    Draw3dRequest request;
    request.layer = RenderLayer::Opaque;
    // ブレンド設定
    request.type = Draw3dType::RuntimeCutFragments;
    request.passName = passName;
    request.fractureInstance = &fractureInstance;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

void RenderQueue::SubmitRuntimeCutIceFragments(FractureInstance& fractureInstance, const GpuResource* material, const float& alpha, const std::string& passName) {
    Draw3dRequest request;
    request.layer = RenderLayer::Opaque;
    // ブレンド設定
    request.type = Draw3dType::RuntimeCutIceFragments;
    request.passName = passName;
    request.fractureInstance = &fractureInstance;
    request.material = material;

    if (alpha == 1.0f) {
        // 不透明描画に登録
        draw3dQueueList_[passName][request.layer][Get3dPsoName(request.type)].push_back(request);
    } else {
        // 半透明描画に登録
        translucentDrawQueueList_[passName].push_back(request);
    }
}

const char* RenderQueue::Get3dPsoName(Draw3dType type) {
    switch (type) {
    case Draw3dType::Default: { return "Default3D"; }
    case Draw3dType::DefaultAdd: { return "Additive3D"; }
    case Draw3dType::Instancing: { return "Instancing3D"; }
    case Draw3dType::InstancingAdd: { return "AdditiveInstancing3D"; }
    case Draw3dType::InstancingWboit: { return "wboit3D"; }
    case Draw3dType::ParticleCS: { return "CSParticle3D"; }
    case Draw3dType::Animation: { return "Animation"; }
    case Draw3dType::Skybox: { return "Skybox"; }
    case Draw3dType::ShadowMap: { return "ShadowMap"; }
    case Draw3dType::Grid: { return "Grid"; }
    case Draw3dType::DebugLine: { return "Line"; }
    case Draw3dType::Fracture: { return "Fracture3D"; }
    case Draw3dType::RuntimeCutFragments: { return "Fracture3D"; }
    case Draw3dType::RuntimeCutIceFragments: { return "IceFracture3D"; }
    default: { return "Default3D"; }
    }
}

const char* RenderQueue::Get2dPsoName(Draw2dType type) {
    switch (type)
    {
    case GameEngine::Draw2dType::Normal: { return "DefaultSprite"; }
    case GameEngine::Draw2dType::Add: { return "AdditiveSprite"; }
    case GameEngine::Draw2dType::Text: { return "Text"; }
    default: { return "DefaultSprite"; }
    }
}
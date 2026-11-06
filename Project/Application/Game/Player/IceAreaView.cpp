#include "pch.h"
#include "IceAreaView.h"
#include "MyMath.h"
#include <algorithm>
using namespace GameEngine;

IceAreaView::IceAreaView(Model* sphereModel) : model_(sphereModel) {
    worldTransform_.Initialize({ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} });
    CreateLayer(ring_);
    CreateLayer(fill_);
}

void IceAreaView::CreateLayer(Layer& layer) {
    const std::vector<std::unique_ptr<Mesh>>& meshes = model_->GetMeshes();
    layer.materials.resize(meshes.size());
    layer.refBuffers.resize(meshes.size());
    for (size_t i = 0; i < meshes.size(); ++i) {
        Material* source = model_->GetMaterial(meshes[i]->GetMaterialName());
        Material::MaterialData* sourceData = source->GetMaterialData();

        // ライティングを切り、地面の暗い陰影に埋もれず色がそのまま見えるようにする
        layer.materials[i].Initialize({ 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, sourceData->shininess, false);
        Material::MaterialData* data = layer.materials[i].GetMaterialData();
        data->textureHandle = sourceData->textureHandle;
        data->roughness = sourceData->roughness;
        data->metallic = sourceData->metallic;

        layer.refBuffers[i].Create();
        layer.refBuffers[i].SetBufferMaterial(
            static_cast<uint32_t>(BufferType::kDefalutMaterial), layer.materials[i].GetMaterialSrvIndex());
    }
}

void IceAreaView::DrawDisc(RenderQueue* renderQueue, Layer& layer, const Vector3& center, float radius, float bottom, const Vector3& color) {
    for (Material& material : layer.materials) {
        material.GetMaterialData()->color = { color.x, color.y, color.z, 1.0f };
    }

    // 半径1の球を縦につぶし、底が指定の高さに来るよう中心を上げる
    float safeRadius = (std::max)(radius, 0.01f);
    Vector3 scale = { safeRadius, kThickness, safeRadius };
    Vector3 position = { center.x, bottom + kThickness, center.z };
    worldTransform_.SetWorldMatrix(Math::MakeWorldMatrixFromEulerRotation(position, { 0.0f, 0.0f, 0.0f }, scale));
    renderQueue->SubmitRaytracingModel(model_, worldTransform_, &layer.refBuffers);
}

void IceAreaView::Draw(RenderQueue* renderQueue, const Vector3& center, float radius, const Vector3& color) {
    // 縁取りは塗りの色を白へ寄せる
    Vector3 ringColor = {
        color.x + (1.0f - color.x) * ringWhiteMix_,
        color.y + (1.0f - color.y) * ringWhiteMix_,
        color.z + (1.0f - color.z) * ringWhiteMix_ };

    // 外側に縁取り、その上に少し小さい塗りを重ねる
    DrawDisc(renderQueue, ring_, center, radius, 0.0f, ringColor);
    DrawDisc(renderQueue, fill_, center, radius - ringWidth_, kThickness, color);
}

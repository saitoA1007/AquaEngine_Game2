#include "pch.h"
#include "SceneRenderManager.h"
#include <cassert>
#include "PSO/Core/PSOManager.h"
#include "raytracingPipeline.h"
#include "BufferRefManager.h"
#include "ModelRenderer.h"
#include "DebugRenderer.h"
#include "SpriteRenderer.h"
using namespace GameEngine;

void SceneRenderManager::Initialize(ID3D12GraphicsCommandList4* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController,
	RaytracingPipeline* raytracingPipeline, BufferRefManager* bufferRefManager, RenderQueue* renderQueue) {
    commandList_ = commandList;
    renderPassController_ = renderPassController;
    raytracingPipeline_ = raytracingPipeline;
    srvManager_ = srvManager;
    bufferRefManager_ = bufferRefManager;
	renderQueue_ = renderQueue;

    // 描画パスと実行順を設定
    CreateRenderPasses();

    // PSOを登録
    RegisterPSOs(psoManager);

    // 破片描画用のコマンドルートシグネチャ
    fractureCommandSignature_ = psoManager->GetCommandSignature("DrawIndexedIndirect");
    iceFractureCommandSignature_ = psoManager->GetCommandSignature("IceDrawIndexedIndirect");

    // bufferのsrvIndexのスタート位置を設定
    bufferStartSrvIndex_ = srvManager_->GetStartSrvIndex(SrvHeapType::Buffer);

    // tlasを作成
    tlas_.Create(commandList_, maxRayInstanceNum_);
}

void SceneRenderManager::Begin() {
    // クリア
    renderQueue_->Clear();
    currentPsoName_.clear();
}

void SceneRenderManager::Execute() {
    // 更新
    renderQueue_->Update();

    // レイトレーシング描画コマンドを解放
    RaytracingExecute();

    // ラスタライズ描画コマンドを解放
    RasterizeExecute();

    // レイトレとラスタライズの描画を合成する
    LightingComposite();

    // 半透明描画
    RasterizeTranslucentExecute();

    // 2D描画
    Draw2dExecute();

    // 半透明の描画結果を合成する
    //Composite();

    // 最終的に画面に出すためのパスの設定
    renderPassController_->SetSceneFinalPass(finalPassName_);
    renderPassController_->SetPresentPass(finalPassName_);
}

void SceneRenderManager::RegisterPSO(const std::string& name, PSOManager* psoManager) {
    psoList_[name] = psoManager->GetDrawPsoData(name);
}

void SceneRenderManager::RegisterPSOs(PSOManager* psoManager) {
    const std::string kUsePsoNames[] = {
        // 3D描画用
        "Default3D",
        "Additive3D",
        "Instancing3D",
        "AdditiveInstancing3D",
        "Animation",
        "Skybox",
        "ShadowMap",
        "Grid",
        "Line",

        // 2D描画用
        "DefaultSprite",
        "AdditiveSprite",

        // テキスト描画用
        "Text",

        // 半透明描画用
        "wboit3D",
        "wboitResolve",

        // GPUパーティクル用
        "CSParticle3D",

        // レイトレとラスタライズの合成用
        "LightingComposite",
        // 深度コピー用
        "DepthCopy",
        // カラーコピー
        "ColorCopy",

        // 破片描画用
        "Fracture3D",
        "IceFracture3D",
    };

    for (const auto& psoName : kUsePsoNames) {
        psoList_[psoName] = psoManager->GetDrawPsoData(psoName);
    }
}

void SceneRenderManager::CreateRenderPasses() {
    // レイトレーシングで描画するパス
    renderPassController_->AddPass("RaytracingPass", RenderTextureMode::RtvAndUav);
    // レイトレーシング描画の深度情報を記録する。描画に使用しない
    renderPassController_->AddPass("RaytracingPassDepth", RenderTextureMode::UavOnly, 1280, 720, { 0.0f,0.0f,0.0f,1.0f }, DXGI_FORMAT_R32_FLOAT);

    // レイトレとラスタライズを合成する用のパス
    renderPassController_->AddPass("LightingCompositePass");

    // 影を描画するパス
    renderPassController_->AddPass("ShadowPass", RenderTextureMode::DsvOnly, 2048, 2048);
    // デフォルトで描画するパス
    renderPassController_->AddPass("DefaultPass", RenderTextureMode::RtvAndDsv, 1280, 720, { 0.0f,0.0f,0.0f,0.0f });

    // ラスタライズの最終描画
    rasterizeFinalPassName_ = "DefaultPass";
    // レイトレの最終描画
    raytracingFinalPassName_ = "RaytracingPass";
    // 最終的な描画先を設定
    finalPassName_ = "LightingCompositePass";
    renderPassController_->SetSceneFinalPass(finalPassName_);
    renderPassController_->SetPresentPass(finalPassName_);

    // WBOITに使用するパスを作成。半透明描画に使用
    renderPassController_->AddPass("WBOITAccumulatePass", RenderTextureMode::RtvOnly, 1280, 720, { 0.0f,0.0f,0.0f,0.0f }, DXGI_FORMAT_R16G16B16A16_FLOAT);
    renderPassController_->AddPass("WBOITResolvePass", RenderTextureMode::RtvOnly, 1280, 720, { 1.0f,1.0f,1.0f,1.0f }, DXGI_FORMAT_R8_UNORM);

    renderPassController_->AddPass("CompositePass");

    // 実行順序を設定
    RegisterPassOrder({ "ShadowPass", "DefaultPass" });
}

void SceneRenderManager::PreDraw(const std::string& psoName) {
    if (psoName == "ShadowMap") {
        ModelRenderer::SetCamera(renderQueue_->GetLightResource()->GetResource());
    } else {
        if (renderQueue_->GetUseDebugCamera()) {
            // デバック用のカメラを設定
            ModelRenderer::SetCamera(renderQueue_->GetDebugCameraResource()->GetResource());
        } else {
            ModelRenderer::SetCamera(renderQueue_->GetCameraResource()->GetResource());
        }
    }

    // 前回と同じPSOなら切り替え不要
    if (currentPsoName_ == psoName) { return; }

    auto it = psoList_.find(psoName);
    assert(it != psoList_.end() && "未登録のPSO名です");

    commandList_->SetGraphicsRootSignature(it->second.rootSignature);
    commandList_->SetPipelineState(it->second.graphicsPipelineState);
    currentPsoName_ = psoName;
}

void SceneRenderManager::Execute3dRequest(const Draw3dRequest& request) {
    switch (request.type) {

    case Draw3dType::Default:
    case Draw3dType::DefaultAdd:
        ModelRenderer::DrawLight(renderQueue_->GetLightResource());
        ModelRenderer::Draw(request.model, *request.worldTransform, request.material);
        break;

    case Draw3dType::Instancing:
    case Draw3dType::InstancingAdd:
        ModelRenderer::DrawInstancing(request.model, request.numInstances, *request.worldTransforms, request.material);
        break;

    case Draw3dType::InstancingWboit:
        ModelRenderer::DrawWboitInstancing(request.model, request.numInstances, *request.worldTransforms, renderQueue_->GetWboitResource(), request.material);
        break;

    case Draw3dType::ParticleCS:
        ModelRenderer::DrawParticleCS(request.model, request.numInstances, request.particleCsResource, renderQueue_->GetPerViewResource());
        break;

    case Draw3dType::Animation:
        ModelRenderer::DrawAnimation(request.model, *request.worldTransform, renderQueue_->GetLightResource(), request.material);
        break;

    case Draw3dType::Skybox:
        ModelRenderer::DrawSkybox(request.model, *request.worldTransform, request.material);
        break;

    case Draw3dType::ShadowMap:
        ModelRenderer::DrawShadowMap(request.model, *request.worldTransform);
        break;

    case Draw3dType::Grid:
        ModelRenderer::DrawGrid(request.model, *request.worldTransform);
        break;

    case Draw3dType::DebugLine:
        ModelRenderer::DrawDebugLine(request.debugRenderer_->GetVertexBufferView(), request.debugRenderer_->GetTotalVertices());
        break;

    case Draw3dType::Fracture:
        ModelRenderer::DrawFracture(request.model, *request.fractureInstance, fractureCommandSignature_, renderQueue_->GetLightResource());
        break;

    case Draw3dType::RuntimeCutFragments:
        ModelRenderer::DrawRuntimeCutFragments(*request.fractureInstance, fractureCommandSignature_, renderQueue_->GetLightResource(), request.material);
        break;

    case Draw3dType::RuntimeCutIceFragments:
        ModelRenderer::DrawRuntimeCutFragments(*request.fractureInstance, iceFractureCommandSignature_, renderQueue_->GetLightResource(), request.material);
        break;

    default:
        assert(false && "未対応のDraw3dTypeです");
        break;
    }
}

void SceneRenderManager::Execute2dRequest(const Draw2dRequest& request) {
    switch (request.type)
    {
    case Draw2dType::Normal:
        SpriteRenderer::Draw(request.sprite);
        break;

    case Draw2dType::Add:
        SpriteRenderer::Draw(request.sprite);
        break;

    case Draw2dType::Text:
        SpriteRenderer::Draw(request.text);
        break;

    default:
        assert(false && "未対応のDraw2dTypeです");
        break;
    }
}

void SceneRenderManager::DrawRaytracing() {
    commandList_->SetComputeRootSignature(raytracingPipeline_->GetGlobalRootSignature());
    // TLASのセット
    commandList_->SetComputeRootDescriptorTable(0, tlas_.GetSrvHandleGPU());
    // テスクチャのセット
    commandList_->SetComputeRootDescriptorTable(1, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
    // BufferRefのセット
    commandList_->SetComputeRootDescriptorTable(2, bufferRefManager_->GetSrvHandleGPU());
    // Bufferのセット
    commandList_->SetComputeRootDescriptorTable(3, srvManager_->GetGPUHandle(bufferStartSrvIndex_));
    // カメラのセット
    if (renderQueue_->GetUseDebugCamera()) {
        commandList_->SetComputeRootConstantBufferView(4, renderQueue_->GetDebugCameraResource()->GetGpuVirtualAddress());
    } else {
        commandList_->SetComputeRootConstantBufferView(4, renderQueue_->GetCameraResource()->GetGpuVirtualAddress());
    }
    // ライトのセット
    commandList_->SetComputeRootConstantBufferView(5, renderQueue_->GetLightResource()->GetGpuVirtualAddress());
    // 出力画像を設定
    commandList_->SetComputeRootDescriptorTable(6, srvManager_->GetGPUHandle(renderPassController_->GetUavIndex("RaytracingPass")));
    commandList_->SetComputeRootDescriptorTable(7, srvManager_->GetGPUHandle(renderPassController_->GetUavIndex("RaytracingPassDepth")));
    // 背景画像
    commandList_->SetComputeRootDescriptorTable(8, srvManager_->GetGPUHandle(renderQueue_->GetSkyboxTexture()));

    // レイトレーシングを開始
    commandList_->SetPipelineState1(raytracingPipeline_->GetStateObject());
    commandList_->DispatchRays(&raytracingPipeline_->GetDispatchRayDesc());
}

void SceneRenderManager::RasterizeExecute() {
    // 描画のリセット
    enableDrawRasterize_ = false;

    auto draw3dQueueList = renderQueue_->GetDraw3dQueue();
    auto translucentDrawQueueList = renderQueue_->GetTranslucentQueue();
    auto draw2dQueueList = renderQueue_->GetDraw2dQueue();
    

    for (const auto& passName : passExecuteOrder_) {

        // 不透明、半透明ともにコマンドがなければ飛ばす
        bool hasOpaque = draw3dQueueList.count(passName) > 0;
        bool hasTranslucent = translucentDrawQueueList.count(passName) > 0;
        if (hasTranslucent && passName == "WBOITAccumulatePass") { hasTranslucent = false; }
        //bool has2d = draw2dQueueList.count(passName) > 0;
        if (!hasOpaque && !hasTranslucent) {
            renderPassController_->PrePass(passName);
            renderPassController_->ClearRenderPass(passName);
            renderPassController_->PostPass(passName);
            continue;
        }

        enableDrawRasterize_ = true;
        if (enableDrawRaytracing_ && rasterizeFinalPassName_ == passName) {
            renderPassController_->PrePass(passName);
            renderPassController_->ClearRenderPass(passName);
            renderPassController_->SetOnlyDsvRenderTarget(passName);
            // レイトレの深度値をコピーする
            CopyRaytracingDepth();

            // 深度値をコピーした状態で再びターゲット
            renderPassController_->PrePass(passName);
        } else {
            renderPassController_->PrePass(passName);
            renderPassController_->ClearRenderPass(passName);
        }
        currentPsoName_.clear();

        // 不透明描画コマンドを解放
        if (hasOpaque) {
            for (auto& [layer, psoMap] : draw3dQueueList[passName]) {
                for (auto& [psoName, requests] : psoMap) {
                    if (requests.empty()) { continue; }
                    // 描画前処理
                    PreDraw(psoName);
                    for (const auto& request : requests) {
                        // 描画コマンド解放
                        Execute3dRequest(request);
                    }
                }
            }
        }

        // 半透明描画コマンドを解放
        if (hasTranslucent) {
            auto& translucentList = translucentDrawQueueList[passName];
        
            // カメラの距離でソートをおこなう
            // std::sort(translucentList.begin(), translucentList.end(),
            //     [](const DrawRequest& a, const DrawRequest& b) {
            //         return a.sortKey > b.sortKey;
            //     });
        
            for (const auto& request : translucentList) {
                // 描画前処理
                PreDraw(renderQueue_->Get3dPsoName(request.type));
                // 描画コマンド解放
                Execute3dRequest(request);
            }
        }

        // 2D描画コマンドを解放
        //if (has2d) {
        //    for (auto& [layer, psoMap] : draw2dQueueList[passName]) {
        //        for (auto& [psoName, requests] : psoMap) {
        //            if (requests.empty()) { continue; }
        //            // 描画前処理
        //            PreDraw(psoName);
        //            for (const auto& request : requests) {
        //                // 描画コマンド解放
        //                Execute2dRequest(request);
        //            }
        //        }
        //    }
        //}
        renderPassController_->PostPass(passName);
    }

    // 最終描画先に設定
    if (enableDrawRasterize_) {
        finalPassName_ = rasterizeFinalPassName_;
    }
}

void SceneRenderManager::RasterizeTranslucentExecute() {
    enableDrawRasterizeTranslucent_ = false;

    std::string passName = "WBOITAccumulatePass";

    auto translucentDrawQueueList = renderQueue_->GetTranslucentQueue();

    bool hasTranslucent = translucentDrawQueueList.count(passName) > 0;

    // =========================================
    // パスの名前を参照はしつつおこなうのはコピーのみ

    std::string drawPassName = "LightingCompositePass";

    if (hasTranslucent) {

        // 描画前処理
        if (enableDrawRaytracing_ && enableDrawRasterize_) {
            // レイトレとラスタライズ描画
            renderPassController_->PrePass({ drawPassName }, rasterizeFinalPassName_);
            renderPassController_->SetOnlyDsvRenderTarget(drawPassName);

            // レイトレの深度値をコピーする
            CopyRaytracingDepth();

            // 深度値をコピーした状態で再びターゲット
            renderPassController_->PrePass(drawPassName);

        } else if (enableDrawRaytracing_ && !enableDrawRasterize_) {
            // レイトレのみ
            renderPassController_->PrePass(drawPassName);
            renderPassController_->ClearRenderPass(drawPassName);
            renderPassController_->SetOnlyDsvRenderTarget(drawPassName);

            // レイトレの深度値をコピーする
            CopyRaytracingDepth();

            // 深度値をコピーした状態で再びターゲット
            renderPassController_->PrePass(drawPassName);

            // レイトレのカラーをコピー
            PreDraw("ColorCopy");
            commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList_->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex(raytracingFinalPassName_)));
            commandList_->DrawInstanced(3, 1, 0, 0);
        } else if (!enableDrawRaytracing_ && enableDrawRasterize_) {
            // ラスタライズのみ
            drawPassName = rasterizeFinalPassName_;
            renderPassController_->PrePass(drawPassName);
            renderPassController_->ClearRenderPass(drawPassName);
        }

        // 最終描画パス
        finalPassName_ = drawPassName;

        // 描画
        auto& translucentList = translucentDrawQueueList[passName];
        for (const auto& request : translucentList) {
            // 描画前処理
            PreDraw(renderQueue_->Get3dPsoName(request.type));
            // 描画コマンド解放
            Execute3dRequest(request);
        }

        // 描画後処理
        renderPassController_->PostPass(drawPassName);

        // 透明描画の有効
        enableDrawRasterizeTranslucent_ = true;
    } 
}

void SceneRenderManager::RaytracingExecute() {

    auto raytracingDrawQueueList = renderQueue_->GetRaytracingQueue();

    if (raytracingDrawQueueList.size() != 0) {
        enableDrawRaytracing_ = true;
    } else {
        // 描画するものがなければ早期リターン
        enableDrawRaytracing_ = false;
        return;
    }

    // tlasを更新する
    tlas_.Update(commandList_, raytracingDrawQueueList);

    // UAVに状態を変更
    renderPassController_->SwitchToUAV("RaytracingPass");
    renderPassController_->SwitchToUAV("RaytracingPassDepth");

    // レイトレーシングの描画
    DrawRaytracing();

    // UAV書き込み完了
    renderPassController_->InsertUavBarrier("RaytracingPass");
    renderPassController_->InsertUavBarrier("RaytracingPassDepth");
    renderPassController_->PostPass("RaytracingPass");
    renderPassController_->PostPass("RaytracingPassDepth");

    // 最終描画先に設定
    finalPassName_ = raytracingFinalPassName_;
}

void SceneRenderManager::Draw2dExecute() {

    auto draw2dQueueList = renderQueue_->GetDraw2dQueue();
    
    renderPassController_->PrePass(finalPassName_);
    currentPsoName_.clear();
    for (const auto& passName : passExecuteOrder_) {

        if (draw2dQueueList.count(passName) == 0) { continue; }

        for (auto& [layer, psoMap] : draw2dQueueList[passName]) {
            for (auto& [psoName, requests] : psoMap) {
                if (requests.empty()) { continue; }
                // 描画前処理
                PreDraw(psoName);
                for (const auto& request : requests) {
                    // 描画コマンド解放
                    Execute2dRequest(request);
                }
            }
        }
    }
    renderPassController_->PostPass(finalPassName_);
}

void SceneRenderManager::LightingComposite() {
    // 両方とも描画が有効の場合のみ合成
    if (!enableDrawRaytracing_ || !enableDrawRasterize_) { return; }

    // レイトレとラスタライズの内容を合成する
    renderPassController_->PrePass("LightingCompositePass");
    renderPassController_->ClearRenderPass("LightingCompositePass");
    PreDraw("LightingComposite");
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex(rasterizeFinalPassName_)));
    commandList_->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUHandle(renderPassController_->GetDepthSrvIndex(rasterizeFinalPassName_)));
    commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex("RaytracingPass")));
    commandList_->SetGraphicsRootDescriptorTable(3, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex("RaytracingPassDepth")));
    commandList_->DrawInstanced(3, 1, 0, 0);
    renderPassController_->PostPass("LightingCompositePass");

    finalPassName_ = "LightingCompositePass";
}

void SceneRenderManager::CopyRaytracingDepth() {
    PreDraw("DepthCopy");
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex("RaytracingPassDepth")));
    commandList_->DrawInstanced(3, 1, 0, 0);
}

void SceneRenderManager::Composite() {
    if (!enableDrawRasterizeTranslucent_) { return; }

    renderPassController_->PrePass("CompositePass");
    renderPassController_->ClearRenderPass("CompositePass");
    PreDraw("wboitResolve");
    commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList_->SetGraphicsRootDescriptorTable(0, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex("WBOITAccumulatePass")));
    commandList_->SetGraphicsRootDescriptorTable(1, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex("WBOITResolvePass")));
    commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetGPUHandle(renderPassController_->GetSrvIndex(finalPassName_)));
    commandList_->DrawInstanced(3, 1, 0, 0);
    renderPassController_->PostPass("CompositePass");

    finalPassName_ = "CompositePass";
}

void SceneRenderManager::ClearPassOnly(const std::string& passName) {
    renderPassController_->PrePass(passName);
    renderPassController_->ClearRenderPass(passName);
    renderPassController_->PostPass(passName);
}
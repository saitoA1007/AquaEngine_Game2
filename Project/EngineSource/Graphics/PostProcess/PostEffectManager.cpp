#include "pch.h"
#include "PostEffectManager.h"
#include "PostEffectGraphSerializer.h"
#include "LogManager.h"
using namespace GameEngine;

void PostEffectManager::Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController) {
    commandList_ = commandList;
    srvManager_ = srvManager;
    renderPassController_ = renderPassController;

    // 使い回し用のパスを生成
    for (auto& name : poolPassNames_) {
        EnsurePass(name);
    }
    // 輝度マスクの描画パスを作成
    EnsurePass("HighLumMaskPass");

    // 生成出来るポストエフェクトのタイプを追加する
    registry_.RegisterType<ColorGrading>("ColorGrading", psoManager);
    registry_.RegisterType<HighLumMask>("HighLumMask", psoManager);
    registry_.RegisterType<GaussVertical>("GaussVertical", psoManager);
    registry_.RegisterType<GaussHorizontal>("GaussHorizontal", psoManager);
    registry_.RegisterType<Bloom>("Bloom", psoManager);
    registry_.RegisterType<Dissolve>("Dissolve", psoManager);

    // デフォルトのノードが繋がっている状態
    BuildDefaultGraph();
    // 保存されているノードを呼び出す
    LoadGraph(kGraphFilePath);
}

void PostEffectManager::Execute() {
    // 削除済みエフェクトの遅延破棄
    registry_.Tick();

    if (graph_.dirty) { RebuildOrder(); }

    currentPassName_ = ExecuteGraph(renderPassController_->GetSceneFinalPass());
    renderPassController_->SetPostProcessFinalPass(currentPassName_);
    renderPassController_->SetPresentPass(currentPassName_);
}

std::string PostEffectManager::ExecuteGraph(const std::string& scenePass) {
    frameResources_.Reset();
    const int sceneRes = frameResources_.Add({ renderPassController_->GetSrvIndex(scenePass), -1, 0, &scenePass });

    stepRes_.assign(executeSteps_.size(), sceneRes);

    // 順序構築に失敗したらシーンを出力する
    const std::string* finalPass = &scenePass;

    for (size_t i = 0; i < executeSteps_.size(); ++i) {
        const ExecuteStep& step = executeSteps_[i];
        PostEffectNode* node = step.node;

        if (node->kind == PostEffectNodeKind::kScene) { continue; }

        // 入力
        inRes_.clear();
        for (int src : step.inputSteps) {
            inRes_.push_back(src >= 0 ? stepRes_[src] : sceneRes);
        }

        if (node->kind == PostEffectNodeKind::kOutput) {
            finalPass = frameResources_[inRes_[0]].passName;
            continue;
        }

        IPostEffect* fx = node->effect;
        for (uint32_t s = 0; s < inRes_.size(); ++s) {
            fx->SetInputIndex(s, frameResources_[inRes_[s]].srvIndex);
        }

        // 無効の場合は指定スロットの入力をそのまま抜ける
        if (!fx->IsActive()) {
            const uint32_t pt = fx->GetPassThroughSlot();
            for (uint32_t s = 0; s < inRes_.size(); ++s) {
                if (s != pt) { frameResources_.Release(inRes_[s]); }
            }
            frameResources_[inRes_[pt]].refCount += step.consumers - 1;
            stepRes_[i] = inRes_[pt];
            continue;
        }

        // 出力先の決定。保存パスが無ければプールから借りる
        int slot = -1;
        const std::string* outPass = &node->passName;
        if (outPass->empty()) {
            slot = frameResources_.AcquireSlot();
            if (slot < 0) {
                LogManager::GetInstance().Log("PostEffect: プールのRTが不足しています\n");
                assert(false);
                continue;
            }
            outPass = &poolPassNames_[slot];
        }
        stepRes_[i] = frameResources_.Add({ renderPassController_->GetSrvIndex(*outPass), slot, step.consumers, outPass });

        DrawEffect(fx, *outPass);

        // 入力の解放
        for (int idx : inRes_) {
            frameResources_.Release(idx); 
        }
    }

    return *finalPass;
}

void PostEffectManager::DrawEffect(IPostEffect* fx, const std::string& outPass) {
    fx->Update();
    renderPassController_->PrePass(outPass);
    renderPassController_->ClearRenderPass(outPass);
    PreDraw(fx->GetPsoName());
    fx->Draw(commandList_, srvManager_);
    renderPassController_->PostPass(outPass);
}

void PostEffectManager::PreDraw(const std::string& psoName) {
    const DrawPsoData* pso = registry_.FindPso(psoName);
    assert(pso && "未登録のPSO名です");

    commandList_->SetGraphicsRootSignature(pso->rootSignature);
    commandList_->SetPipelineState(pso->graphicsPipelineState);
}

void PostEffectManager::ResetGraph() {
    BuildDefaultGraph();
    executeOrder_.clear();
    executeSteps_.clear();
}

void PostEffectManager::BuildDefaultGraph() {
    // 古いエフェクトが残ると名前がずれるので全て破棄する
    registry_.RetireAll();
    graph_ = PostEffectGraph{};

    auto* scene = graph_.AddSceneNode();
    auto* output = graph_.AddOutputNode();

    auto* highLum = CreateEffectNode("HighLumMask");
    SetPersistent(highLum->id, true);  // 輝度マスクは保存パスに描く
    auto* gaussV = CreateEffectNode("GaussVertical");
    auto* gaussH = CreateEffectNode("GaussHorizontal");
    auto* bloom = CreateEffectNode("Bloom");
    auto* grading = CreateEffectNode("ColorGrading");
    auto* dissolve = CreateEffectNode("Dissolve");

    // Scene → HighLum → GaussV → GaussH → Bloom(Blur) → Grading → Dissolve → Output
    graph_.AddLink(scene->output.id, highLum->inputs[0].id);
    graph_.AddLink(highLum->output.id, gaussV->inputs[0].id);
    graph_.AddLink(gaussV->output.id, gaussH->inputs[0].id);
    graph_.AddLink(gaussH->output.id, bloom->inputs[0].id);   // Blur
    graph_.AddLink(scene->output.id, bloom->inputs[1].id);   // 元の画像
    graph_.AddLink(bloom->output.id, grading->inputs[0].id);
    graph_.AddLink(grading->output.id, dissolve->inputs[0].id);
    graph_.AddLink(dissolve->output.id, output->inputs[0].id);
}

void PostEffectManager::RebuildOrder() {
    std::vector<int> order;
    if (graph_.BuildOrder(order)) {
        executeOrder_ = std::move(order);
    } else {
        // 循環など。前回の有効な順序を使い続ける
        LogManager::GetInstance().Log("PostEffectGraph: 実行順の構築に失敗しました\n");
    }
    BuildExecuteSteps();
    graph_.dirty = false;
}

void PostEffectManager::BuildExecuteSteps() {
    executeSteps_.clear();

    // 出力ピンID → ステップ添字
    std::unordered_map<int, int> pinToStep;
    for (int nodeId : executeOrder_) {
        PostEffectNode* node = graph_.FindNode(nodeId);
        if (!node) { continue; }

        ExecuteStep step;
        step.node = node;
        for (const Pin& pin : node->inputs) {
            const Link* link = graph_.FindLinkToPin(pin.id);
            auto it = link ? pinToStep.find(link->startPinId) : pinToStep.end();
            step.inputSteps.push_back(it != pinToStep.end() ? it->second : -1);
        }
        pinToStep[node->output.id] = static_cast<int>(executeSteps_.size());
        executeSteps_.push_back(std::move(step));
    }

    // 今回実行されるノードだけを読み手として数える
    for (const Link& l : graph_.links) {
        auto it = pinToStep.find(l.startPinId);
        if (it == pinToStep.end()) { continue; }
        const PostEffectNode* end = graph_.FindNodeByPin(l.endPinId);
        const bool endActive = end && std::ranges::any_of(executeSteps_,
            [&](const ExecuteStep& s) { return s.node == end; });
        if (endActive) { ++executeSteps_[it->second].consumers; }
    }
}

void PostEffectManager::FrameResources::Reset() {
    resources_.clear();
    slotBusy_.fill(false);
}

int PostEffectManager::FrameResources::Add(const Resource& r) {
    resources_.push_back(r);
    return static_cast<int>(resources_.size()) - 1;
}

int PostEffectManager::FrameResources::AcquireSlot() {
    for (int i = 0; i < static_cast<int>(kPoolSize); ++i) {
        if (!slotBusy_[i]) {
            slotBusy_[i] = true;
            return i;
        }
    }
    return -1;
}

void PostEffectManager::FrameResources::Release(int idx) {
    Resource& r = resources_[idx];
    if (--r.refCount == 0 && r.poolSlot >= 0) { slotBusy_[r.poolSlot] = false; }
}

bool PostEffectManager::SaveGraph(const std::string& filePath) {
    return PostEffectGraphSerializer::Save(filePath, graph_, registry_);
}

bool PostEffectManager::LoadGraph(const std::string& filePath) {
    PostEffectGraphSerializer::Loaded loaded;
    if (!PostEffectGraphSerializer::Load(filePath, registry_, poolPassNames_, loaded)) { return false; }

    // ここまで成功したら反映する
    for (auto& pass : loaded.passesToCreate) { EnsurePass(pass); }
    registry_.Replace(std::move(loaded.effects));
    graph_ = std::move(loaded.graph);
    executeOrder_.clear();
    executeSteps_.clear();
    return true;
}

PostEffectNode* PostEffectManager::CreateEffectNode(const std::string& typeName, const std::string& name) {
    std::string instName;
    IPostEffect* fx = registry_.Create(typeName, name, instName);
    if (!fx) { return nullptr; }
    return graph_.AddEffectNode(typeName, instName, "", fx);
}

void PostEffectManager::DestroyEffectNode(int nodeId) {
    PostEffectNode* node = graph_.FindNode(nodeId);
    if (!node || node->kind != PostEffectNodeKind::kEffect) { return; }
    const std::string name = node->name;

    graph_.RemoveNode(nodeId);
    registry_.Destroy(name);
}

void PostEffectManager::SetPersistent(int nodeId, bool enable) {
    PostEffectNode* node = graph_.FindNode(nodeId);
    if (!node || node->kind != PostEffectNodeKind::kEffect) { return; }
    if (enable) {
        // インスタンス名ごとに専用パス
        node->passName = node->name + "Pass";
        EnsurePass(node->passName);
    } else {
        // プールに戻す
        node->passName.clear();
    }
}

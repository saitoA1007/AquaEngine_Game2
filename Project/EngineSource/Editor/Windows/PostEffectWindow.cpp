#include "pch.h"
#include "PostEffectWindow.h"
#include "LogManager.h"
#include "PostProcess/PostEffectManager.h"
#include "TextureManager.h"
#include "RenderPassController.h"

using namespace GameEngine;
namespace ned = ax::NodeEditor;

namespace {
    static ImTextureID ToImTex(TextureManager* tm, uint32_t handle) {
        D3D12_GPU_DESCRIPTOR_HANDLE h = tm->GetTextureSrvHandlesGPU(handle);
        return (ImTextureID)h.ptr;
    }

    // ヘッダー画像に乗算する色
    static ImU32 HeaderTint(PostEffectNodeKind kind, bool active) {
        if (!active) { return IM_COL32(110, 110, 115, 255); }
        switch (kind) {
        case PostEffectNodeKind::kScene:  return IM_COL32(130, 220, 150, 255);
        case PostEffectNodeKind::kOutput: return IM_COL32(248, 140, 120, 255);
        default:                          return IM_COL32(128, 195, 248, 255);
        }
    }

    // ツールバー用のアイコンボタン
    bool IconButton(const char* id, ImTextureID tex, const char* tooltip) {
        const bool pressed = ImGui::ImageButton(id, tex, ImVec2(16, 16));
        if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", tooltip); }
        return pressed;
    }

    // エフェクトノードが有効か
    bool IsNodeOn(const PostEffectNode* n) {
        return n && (n->kind != PostEffectNodeKind::kEffect || n->effect->IsActive());
    }
}

PostEffectWindow::PostEffectWindow(PostEffectManager* postEffectManager, TextureManager* textureManager, RenderPassController* renderPassController) {
    assert(postEffectManager != nullptr);
    assert(textureManager != nullptr);
    assert(renderPassController != nullptr);
    postEffectManager_ = postEffectManager;
    textureManager_ = textureManager;
    renderPassController_ = renderPassController;

    // 画像を取得
    headerBgHandle_ = textureManager_->GetHandleByName("BlueprintBackground.png");
    iconRestoreHandle_ = textureManager_->GetHandleByName("ic_restore_white_24dp.png");
    iconSaveHandle_ = textureManager_->GetHandleByName("saveIcon.png");
    iconLoadHandle_ = textureManager_->GetHandleByName("loadIcon.png");

    ned::Config config;
    // 位置はPostEffectNode::posで管理する
    config.SettingsFile = nullptr;
    context_ = ned::CreateEditor(&config);

    ned::SetCurrentEditor(context_);
    NodeUI::ApplyEditorStyle();
    ned::SetCurrentEditor(nullptr);

    nodeStyle_.width = 240.0f;
    nodeStyle_.iconSize = 24.0f;
    nodeStyle_.headerTexSize = ImVec2(64.0f, 64.0f);
    nodeStyle_.headerTexture = headerBgHandle_ ? ToImTex(textureManager_, headerBgHandle_) : ImTextureID{};
}

PostEffectWindow::~PostEffectWindow() {
    if (context_) { ned::DestroyEditor(context_); }
}

void PostEffectWindow::Draw() {
    ImGui::Begin("PostEffect", &isActive);
    if (!postEffectManager_) { ImGui::TextUnformatted("PostEffectManager is not set."); ImGui::End(); return; }

    PostEffectGraph& graph = postEffectManager_->GetGraph();

    // 前フレームで予約された削除を実行
    ApplyPendingDeletes(graph);

    // ツールバー表示
    DrawToolbar();

    // 左側はインスペクタ
    ImGui::BeginChild("PostEffectInspector", ImVec2(inspectorWidth_, 0), ImGuiChildFlags_Borders);
    DrawInspector(graph);
    ImGui::EndChild();

    // 幅を変えるスプリッター
    ImGui::SameLine();
    ImGui::InvisibleButton("##splitter", ImVec2(4.0f, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemHovered() || ImGui::IsItemActive()) { ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); }
    if (ImGui::IsItemActive()) {
        inspectorWidth_ = std::clamp(inspectorWidth_ + ImGui::GetIO().MouseDelta.x, 160.0f, 600.0f);
    }
    ImGui::SameLine();

    // 右側はノードエディタ
    ImGui::BeginChild("PostEffectCanvas", ImVec2(0, 0), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ned::SetCurrentEditor(context_);
    ned::Begin("PostEffectGraph");

    DrawNodes(graph);
    DrawLinks(graph);
    HandleLinkCreation(graph);
    HandleDeletion(graph);
    UpdateSelection();
    DrawContextMenu();

    ned::End();
    ned::SetCurrentEditor(nullptr);
    ImGui::EndChild();

    ImGui::End();
}

void PostEffectWindow::DrawToolbar() {
    if (IconButton("##reset", ToImTex(textureManager_, iconRestoreHandle_), "Reset")) {
        postEffectManager_->ResetGraph();
        applyPositions_ = true;
        selectedNodeId_ = -1;
    }

    ImGui::SameLine();
    if (ImGui::Button("Fit")) {
        ned::SetCurrentEditor(context_);
        ned::NavigateToContent();
        ned::SetCurrentEditor(nullptr);
    }

    ImGui::SameLine();
    if (IconButton("##save", ToImTex(textureManager_, iconSaveHandle_), "Save")) {
        postEffectManager_->SaveGraph();
    }

    ImGui::SameLine();
    if (IconButton("##load", ToImTex(textureManager_, iconLoadHandle_), "Load")) {
        if (postEffectManager_->LoadGraph()) {
            applyPositions_ = true;
            selectedNodeId_ = -1;
        }
    }

    ImGui::SameLine();
    ImGui::Checkbox("Preview", &showPreview_);
}

void PostEffectWindow::UpdateSelection() {
    ned::NodeId sel[1];
    const int count = ned::GetSelectedNodes(sel, 1);
    selectedNodeId_ = (count > 0) ? static_cast<int>(sel[0].Get()) : -1;
}

void PostEffectWindow::DrawInspector(PostEffectGraph& graph) {
    PostEffectNode* node = (selectedNodeId_ >= 0) ? graph.FindNode(selectedNodeId_) : nullptr;
    if (!node) {
        ImGui::TextDisabled("Not selected");
        return;
    }

    ImGui::Text("%s", node->name.c_str());
    ImGui::Separator();

    if (node->kind != PostEffectNodeKind::kEffect) {
        ImGui::TextDisabled("No parameter");
        return;
    }

    ImGui::TextDisabled("Type: %s", node->typeName.c_str());

    bool active = node->effect->IsActive();
    if (ImGui::Checkbox("Active", &active)) { node->effect->SetIsActive(active); }

    bool keep = !node->passName.empty();
    if (ImGui::Checkbox("Keep pass", &keep)) { postEffectManager_->SetPersistent(node->id, keep); }

    ImGui::Separator();

    ImGui::BeginDisabled(!active);
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
    node->effect->DrawParamUI();
    ImGui::PopItemWidth();
    ImGui::EndDisabled();
}

void PostEffectWindow::DrawNodes(PostEffectGraph& graph) {
    for (auto& node : graph.nodes) {
        // 初回とリセット後は保存位置をエディタへ、それ以外はエディタの位置を保存側へ
        if (applyPositions_) { ned::SetNodePosition(node->id, ImVec2(node->pos.x, node->pos.y)); }

        DrawNode(graph, *node);

        if (!applyPositions_) {
            const ImVec2 p = ned::GetNodePosition(node->id);
            node->pos = { p.x, p.y };
        }
    }
    applyPositions_ = false;
}

void PostEffectWindow::DrawNode(PostEffectGraph& graph, PostEffectNode& node) {
    
    const bool isEffect = node.kind == PostEffectNodeKind::kEffect;
    const bool active = !isEffect || node.effect->IsActive();
    const Pin* mainIn = node.inputs.empty() ? nullptr : &node.inputs[0];
    const bool hasOut = node.kind != PostEffectNodeKind::kOutput;
    const ImU32 flowColor = IM_COL32_WHITE;

    NodeUI::NodeBuilder nb(nodeStyle_);
    nb.Begin(node.id);

    // ヘッダー
    nb.BeginHeader(node.label, HeaderTint(node.kind, active));
    if (isEffect) {
        ImGui::SameLine(0, 16);
        bool a = node.effect->IsActive();
        if (ImGui::Checkbox("##active", &a)) { node.effect->SetIsActive(a); }
    }
    nb.EndHeader();

    // メインの入出力
    if (mainIn && hasOut) {
        nb.PinPair(ned::PinId(mainIn->id), "", NodeUI::PinIconType::Flow, flowColor, graph.IsPinLinked(mainIn->id),
            ned::PinId(node.output.id), "", NodeUI::PinIconType::Flow, flowColor, graph.IsPinLinked(node.output.id));
    } else if (mainIn) {
        nb.InputPin(ned::PinId(mainIn->id), "", NodeUI::PinIconType::Flow, flowColor, graph.IsPinLinked(mainIn->id));
    } else if (hasOut) {
        nb.OutputPin(ned::PinId(node.output.id), "", NodeUI::PinIconType::Flow, flowColor, graph.IsPinLinked(node.output.id));
    }

    // 2番目以降の入力
    for (size_t i = 1; i < node.inputs.size(); ++i) {
        const Pin& pin = node.inputs[i];
        nb.InputPin(ned::PinId(pin.id), pin.name, NodeUI::PinIconType::Data, NodeUI::PinTypeColor(pin.pinType), graph.IsPinLinked(pin.id));
    }

    // パラメータ
    if (isEffect) {
        nb.BeginContent();

        // 保存パスに記入するか
        bool keep = !node.passName.empty();
        if (ImGui::Checkbox("Keep pass", &keep)) {
            postEffectManager_->SetPersistent(node.id, keep);
        }
    }

    // 中間描画結果のプレビュー
    if (showPreview_) {
        ImTextureID tex{};
        const std::string pass = GetPreviewPassName(graph, node);
        if (!pass.empty()) { tex = (ImTextureID)renderPassController_->GetSrvHandle(pass).ptr; }
        nb.Preview(tex, !active);
    }

    nb.End();
}

void PostEffectWindow::DrawLinks(const PostEffectGraph& graph) {
    // Outputに到達する経路上のノードを集める
    std::unordered_set<int> livePath;
    std::vector<int> order;
    if (graph.BuildOrder(order)) { livePath.insert(order.begin(), order.end()); }

    for (const Link& link : graph.links) {
        const Pin* s = graph.FindPin(link.startPinId);
        const Pin* e = graph.FindPin(link.endPinId);

        // 両端のノードが経路上にあり、有効なエフェクトなら流す
        const bool flowing = s && e
            && livePath.contains(s->parentNodeId) && livePath.contains(e->parentNodeId)
            && IsNodeOn(graph.FindNode(s->parentNodeId)) && IsNodeOn(graph.FindNode(e->parentNodeId)) 
            && !IsDeletePending(graph, link);

        const ImColor color = flowing ? ImColor(120, 200, 255, 230) : ImColor(110, 110, 120, 160);
        ned::Link(link.id, link.startPinId, link.endPinId, color, flowing ? 2.5f : 1.5f);

        if (flowing) { ned::Flow(link.id, ned::FlowDirection::Forward); }
    }
}

void PostEffectWindow::HandleLinkCreation(PostEffectGraph& graph) {
    if (ned::BeginCreate()) {
        ned::PinId a, b;
        if (ned::QueryNewLink(&a, &b) && a && b) {
            int startId = static_cast<int>(a.Get());
            int endId = static_cast<int>(b.Get());

            // 入力→出力の順でドラッグされた場合は入れ替えて正規化する
            const Pin* pa = graph.FindPin(startId);
            if (pa && pa->pinKind == PinKind::kInput) { std::swap(startId, endId); }

            if (graph.CanConnect(startId, endId)) {
                if (ned::AcceptNewItem()) { graph.AddLink(startId, endId); }
            } else {
                ned::RejectNewItem(ImColor(255, 80, 80), 2.0f);   // 繋げない場合は赤
            }
        }
    }
    ned::EndCreate();
}

void PostEffectWindow::HandleDeletion(PostEffectGraph& graph) {
    if (ned::BeginDelete()) {
        ned::LinkId linkId;
        while (ned::QueryDeletedLink(&linkId)) {
            pendingLinkDeletes_.push_back(static_cast<int>(linkId.Get()));
            ned::RejectDeletedItem();
        }

        ned::NodeId nodeId;
        while (ned::QueryDeletedNode(&nodeId)) {
            const PostEffectNode* n = graph.FindNode(static_cast<int>(nodeId.Get()));
            // Scene、Outputは消せない
            if (n && n->kind == PostEffectNodeKind::kEffect) {
                pendingNodeDeletes_.push_back(n->id);
            }
            ned::RejectDeletedItem();
        }
    }
    ned::EndDelete();
}

void PostEffectWindow::DrawContextMenu() {
    const ImVec2 openPos = ImGui::GetMousePos();
    ned::Suspend();

    if (ned::ShowBackgroundContextMenu()) {
        popupCanvasPos_ = ned::ScreenToCanvas(openPos);
        ImGui::OpenPopup("AddPostEffectNode");
    }

    if (ImGui::BeginPopup("AddPostEffectNode")) {
        ImGui::TextUnformatted("Add Effect");
        ImGui::Separator();

        const std::vector<std::string> types = postEffectManager_->GetEffectTypeNames();
        for (const std::string& type : types) {
            if (!ImGui::MenuItem(type.c_str())) { continue; }
            if (PostEffectNode* added = postEffectManager_->CreateEffectNode(type)) {
                added->pos = { popupCanvasPos_.x, popupCanvasPos_.y };
                ned::SetNodePosition(added->id, popupCanvasPos_);
            }
        }
        if (types.empty()) { ImGui::TextDisabled("(no effect types)"); }
        ImGui::EndPopup();
    }

    ned::Resume();
}

void PostEffectWindow::ApplyPendingDeletes(PostEffectGraph& graph) {
    if (pendingNodeDeletes_.empty() && pendingLinkDeletes_.empty()) { return; }

    // 消えるノードが選択されたままにならないようにする
    ned::SetCurrentEditor(context_);
    ned::ClearSelection();
    ned::SetCurrentEditor(nullptr);
    selectedNodeId_ = -1;

    for (int id : pendingNodeDeletes_) { postEffectManager_->DestroyEffectNode(id); }
    for (int id : pendingLinkDeletes_) { graph.RemoveLink(id); }
    pendingNodeDeletes_.clear();
    pendingLinkDeletes_.clear();
}

bool PostEffectWindow::IsDeletePending(const PostEffectGraph& graph, const Link& link) const {
    auto has = [](const std::vector<int>& v, int id) {
        return std::find(v.begin(), v.end(), id) != v.end();
        };
    if (has(pendingLinkDeletes_, link.id)) { return true; }

    // 削除予約のノードにつながるリンクも対象
    const Pin* s = graph.FindPin(link.startPinId);
    const Pin* e = graph.FindPin(link.endPinId);
    return (s && has(pendingNodeDeletes_, s->parentNodeId))
        || (e && has(pendingNodeDeletes_, e->parentNodeId));
}

std::string PostEffectWindow::GetPreviewPassName(const PostEffectGraph& graph, const PostEffectNode& node) const {
    switch (node.kind) {
        // 元のシーン画像
    case PostEffectNodeKind::kScene:
        return renderPassController_->GetSceneFinalPass();

        // 各エフェクトのパス
    case PostEffectNodeKind::kEffect:
        return node.passName;    

    case PostEffectNodeKind::kOutput: {
        return postEffectManager_->GetFinalPassName();
    }
    }
    return {};
}
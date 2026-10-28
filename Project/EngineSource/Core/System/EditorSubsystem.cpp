#include "pch.h"
#ifdef USE_IMGUI
#include "EditorSubsystem.h"
#include "ResourceSubsystem.h"
#include "GraphicsSubsystem.h"
#include "SceneSubsystem.h"
#include "InputSubsystem.h"
#include "ResourceSubsystem.h"

// デバック機能
#include "EditorMenu/EditorWindowManager.h"
#include "EditorMenu/EditorMenuBar.h"
#include "EditorMenu/EditorLayout.h"
#include "EditorMenu/EditorToolBar.h"
#include "EditorMenu/SceneMenuBar.h"
#include "EditorMenu/ViewOptionsBar.h"
#include "EditorMenu/AddObjectBar.h"

// デバックウィンドウ
#include "Windows/SceneWIndow.h"
#include "Windows/AssetWindow.h"
#include "Windows/ConsoleWindow.h"
#include "Windows/HierarchyWindow.h"
#include "Windows/InspectorWindow.h"
#include "Windows/PerformanceWindow.h"
#include "Windows/MaterialNodeWindow.h"
#include "Windows/PixWindow.h"
#include "Windows/EffectEditorWindow.h"
#include "Windows/PostEffectWindow.h"

using namespace GameEngine;

EditorSubsystem::EditorSubsystem() = default;
EditorSubsystem::~EditorSubsystem() = default;

void EditorSubsystem::Initialize() {
    auto* graphics = context_.graphics;
    auto* resource = context_.resource;
    auto* scene = context_.scene;

    auto gridModel = resource->GetModelManager()->GetNameByModel("Grid");

    // エディタ機能
    windowManager_ = std::make_unique<EditorWindowManager>();
    menuBar_ = std::make_unique<EditorMenuBar>();
    editorLayout_ = std::make_unique<EditorLayout>();
    editorToolBar_ = std::make_unique<EditorToolBar>(resource->GetTextureManager());
    sceneMenuBar_ = std::make_unique<SceneMenuBar>(scene->GetSceneChangeRequest());
    viewOptionsBar_ = std::make_unique<ViewOptionsBar>(context_.input->GetInput(), graphics->GetRenderQueue(), graphics->GetDebugRenderer(), gridModel);
    addObjectBar_ = std::make_unique<AddObjectBar>(scene->GetStaticObjectManager(),
        graphics->GetRenderQueue(), viewOptionsBar_->GetDebugCamera(), resource->GetGameParamEditor());

    // ウィンドウの内容を登録する
    windowManager_->RegisterWindow(std::make_unique<SceneWindow>(graphics->GetRenderPassCtrl(), addObjectBar_.get()));
    windowManager_->RegisterWindow(std::make_unique<AssetWindow>(resource->GetTextureManager()));
    windowManager_->RegisterWindow(std::make_unique<HierarchyWindow>(resource->GetGameParamEditor()));
    windowManager_->RegisterWindow(std::make_unique<InspectorWindow>(resource->GetGameParamEditor(), resource->GetTextureManager()));
    windowManager_->RegisterWindow(std::make_unique<ConsoleWindow>());
    windowManager_->RegisterWindow(std::make_unique<PerformanceWindow>());
    windowManager_->RegisterWindow(std::make_unique<MaterialNodeWindow>(graphics->GetPSOManager(), resource->GetTextureManager()));
    windowManager_->RegisterWindow(std::make_unique<PixWindow>());
    windowManager_->RegisterWindow(std::make_unique<EffectEditorWindow>(resource->GetTextureManager(), 
        resource->GetModelManager(), resource->GetGameParamEditor(), resource->GetEffectsManager()));
    windowManager_->RegisterWindow(std::make_unique<PostEffectWindow>(graphics->GetPostEffectManager(), resource->GetTextureManager(), graphics->GetRenderPassCtrl()));

    // レイアウトのデータを取得する
    editorLayout_->LoadLayout(windowManager_->GetWindows());  
}

void EditorSubsystem::Update() {
    BeginDockSpace();

    sceneMenuBar_->Run();
    viewOptionsBar_->Run();
    addObjectBar_->Run();
    menuBar_->Run(windowManager_.get());
    editorToolBar_->Run();
    windowManager_->DrawAllWindows();
}

void EditorSubsystem::Finalize() {
    // レイアウトデータを保存する
    editorLayout_->SaveLayout(windowManager_->GetWindows());
}

void EditorSubsystem::SceneReset() {
    addObjectBar_->Clear();
}

bool EditorSubsystem::IsActiveUpdate() const {
    return editorToolBar_->GetIsActiveUpdate();
}

bool EditorSubsystem::IsPause() const {
    return editorToolBar_->GetIsPauce();
}

void EditorSubsystem::BeginDockSpace() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpace Window", nullptr, window_flags);
    ImGui::PopStyleVar(2);
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();
}
#endif
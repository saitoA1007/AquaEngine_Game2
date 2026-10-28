#include "pch.h"
#include "AddObjectBar.h"
#include <filesystem>
#include "MyMath.h"
#include "DebugCamera.h"
#include "Command/EditorCommand.h"
#include "GameParamEditor.h"
#include "Windows/IEditorWindow.h"
using namespace GameEngine;

AddObjectBar::AddObjectBar(StaticGameObjectManager* staticObjectManager, RenderQueue* renderQueue, DebugCamera* debugCamera, GameParamEditor* paramEditor) {
	staticObjectManager_ = staticObjectManager;
	renderQueue_ = renderQueue;
    debugCamera_ = debugCamera;
    paramEditor_ = paramEditor;  
}

AddObjectBar::~AddObjectBar() {

}

void AddObjectBar::Run() {

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Object")) {
            // Cubeオブジェクトを追加
			if (ImGui::MenuItem("Cube")) {
                std::string objectName = "CubeObject_" + std::to_string(addedObjectCount_++);
                // オブジェクトの追加コマンドを実行
                commandHistory_.Execute(std::make_unique<AddObjectCommand>(staticObjectManager_, objectName, "cube.obj"));
			}
            // Sphereオブジェクトを追加
            if (ImGui::MenuItem("Sphere")) {
                std::string objectName = "SphereObject_" + std::to_string(addedObjectCount_++);
                // オブジェクトの追加コマンドを実行
                commandHistory_.Execute(std::make_unique<AddObjectCommand>(staticObjectManager_, objectName, "sphere.obj"));
            }
            // Undo
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, commandHistory_.CanUndo())) {
                commandHistory_.Undo();
                ClearSelection();
            }
            // Redo
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, commandHistory_.CanRedo())) {
                commandHistory_.Redo();
                ClearSelection();
            }
            // Load
            if (ImGui::MenuItem("Load", "Ctrl+L", false)) {
                const std::string& activeSceneName = paramEditor_->GetActiveScene();
                staticObjectManager_->LoadSceneObject(activeSceneName);
                commandHistory_.Clear();
            }
            // Save
            if (ImGui::MenuItem("Save", "Ctrl+S", false)) {
                const std::string& activeSceneName = paramEditor_->GetActiveScene();
                staticObjectManager_->SaveSceneObject(activeSceneName);
            }
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}	

    // キーボードショートカット
    ImGuiIO& io = ImGui::GetIO();
    // 独自のショートカットを持つウィンドウがフォーカス中は無効にする
    if (!io.WantTextInput && !EditorShortcut::IsLocalWindowFocused(ImGui::GetFrameCount())) {
        // undo
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            commandHistory_.Undo();
            ClearSelection();
        }
        // redo
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
            commandHistory_.Redo();
            ClearSelection();
        }
        // Load
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_L, false)) {
            const std::string& activeSceneName = paramEditor_->GetActiveScene();
            staticObjectManager_->LoadSceneObject(activeSceneName);
            commandHistory_.Clear();
        }
        // Save
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            const std::string& activeSceneName = paramEditor_->GetActiveScene();
            staticObjectManager_->SaveSceneObject(activeSceneName);
        }
    }
}

void AddObjectBar::ApplyGuizmo() {

    ImGuiIO& io = ImGui::GetIO();

    // 直前に描画したImGui::Imageの左上座標
    ImVec2 imageMin = ImGui::GetItemRectMin();
    ImVec2 imageSize = ImGui::GetItemRectSize();

    // ギズモの選択状態を表示
    if (selectObject_ != nullptr) {
        ImGui::SetNextWindowPos(ImVec2(imageMin.x + 10.0f, imageMin.y + 10.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.6f);

        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("##GizmoToolbar", nullptr, toolbarFlags)) {
            // ボタンサイズ
            ImVec2 btnSize(32.0f, 32.0f);

            /// 移動
            bool isTranslate = (currentOperation_ == ImGuizmo::TRANSLATE);
            if (isTranslate) {
                // 選択中の場合はボタンの色を変える
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }
            if (ImGui::Button("T", btnSize)) {
                currentOperation_ = ImGuizmo::TRANSLATE;
            }
            if (isTranslate) ImGui::PopStyleColor();

            // マウスホバー時に説明を出す
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Translate");

            /// 回転
            bool isRotate = (currentOperation_ == ImGuizmo::ROTATE);
            if (isRotate) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }
            if (ImGui::Button("R", btnSize)) {
                currentOperation_ = ImGuizmo::ROTATE;
            }
            if (isRotate) ImGui::PopStyleColor();

            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Rotate");

            /// 拡縮
            bool isScale = (currentOperation_ == ImGuizmo::SCALE);
            if (isScale) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
            }
            if (ImGui::Button("S", btnSize)) {
                currentOperation_ = ImGuizmo::SCALE;
            }
            if (isScale) ImGui::PopStyleColor();

            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale");
        }
        ImGui::End();
    }
    
    bool isLeftClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool isRightClick = ImGui::IsMouseClicked(ImGuiMouseButton_Right);

    // マウスがクリックされた時
    if ((isLeftClick || isRightClick) && !ImGuizmo::IsUsing() && ImGui::IsItemHovered()) {

        // スクリーン座標のマウス位置を取得
        Vector2 mousePosGlobal = Vector2(io.MousePos.x, io.MousePos.y);

        float relX = mousePosGlobal.x - imageMin.x;
        float relY = mousePosGlobal.y - imageMin.y;

        // マウスがゲーム画面の範囲内にいるときだけ処理
        if (relX >= 0.0f && relX <= imageSize.x &&
            relY >= 0.0f && relY <= imageSize.y) {

            Vector2 mousePosInGame;
            mousePosInGame.x = (relX / imageSize.x) * 1280.0f;
            mousePosInGame.y = (relY / imageSize.y) * 720.0f;

            selectedId_ = -1;

            if (renderQueue_->GetUseDebugCamera()) {
                selectedId_ = staticObjectManager_->SelectObject(mousePosInGame, debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix(), debugCamera_->GetWorldPosition(), 1280.0f, 720.0f);
            } else {
                Camera& camera = renderQueue_->GetMainCamera();
                selectedId_ = staticObjectManager_->SelectObject(mousePosInGame, camera.GetViewMatrix(), camera.GetProjectionMatrix(), camera.GetWorldPosition(), 1280.0f, 720.0f);
            }

            if (selectedId_ <= -1) {
                // リセット
                if (isLeftClick) {
                    ClearSelection();
                }
            } else {
                selectObject_ = staticObjectManager_->GetStaticObject(static_cast<uint32_t>(selectedId_));

                // 右クリックでポップメニューを開く
                if (isRightClick) {
                    ImGui::OpenPopup("ObjectContextMenu");
                }
            }
        }
    }

    // 右クリック操作
    if (selectObject_ != nullptr) {
        if (ImGui::BeginPopup("ObjectContextMenu")) {
            // 「Delete」項目がクリックされたら削除
            if (ImGui::MenuItem("Delete")) {
                if (selectedId_ != -1) {
                    // オブジェクトの削除コマンドを実行
                    commandHistory_.Execute(std::make_unique<DeleteObjectCommand>(staticObjectManager_, static_cast<uint32_t>(selectedId_)));
                    // 削除したので選択状態をクリアする
                    ClearSelection();
                }
            }
            ImGui::EndPopup();
        }
    }

    // ギズモで操作
    if (selectObject_ != nullptr) {

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);
        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

        // 選択しているオブジェクトのワールド行列を取得
        WorldTransform& worldTransform = selectObject_->GetWorldTransform();
        Matrix4x4 objectMatrix = worldTransform.GetWorldMatrix();

        // ギズモで編集するための配列にコピー
        float gizmoMatrix[16];
        std::memcpy(gizmoMatrix, &objectMatrix, sizeof(float) * 16);

        Matrix4x4 viewMat;
        Matrix4x4 projMat;
        if (renderQueue_->GetUseDebugCamera()) {
            viewMat = debugCamera_->GetViewMatrix();
            projMat = debugCamera_->GetProjectionMatrix();
        } else {
            Camera& camera = renderQueue_->GetMainCamera();
            viewMat = camera.GetViewMatrix();
            projMat = camera.GetProjectionMatrix();
        }

        // ドラッグ中でない間は、現在の値を保持する
        if (!wasUsingGizmo_) {
            transformBeforeGizmo_.translate = worldTransform.transform_.translate;
            transformBeforeGizmo_.rotate = worldTransform.transform_.rotate;
            transformBeforeGizmo_.scale = worldTransform.transform_.scale;
        }

        // ギズモを動かしたかどうかの判定
        bool isManipulated = ImGuizmo::Manipulate(
            reinterpret_cast<const float*>(&viewMat),
            reinterpret_cast<const float*>(&projMat),
            currentOperation_,
            ImGuizmo::WORLD,
            gizmoMatrix
        );

        if (isManipulated) {
            float translate[3], rotate[3], scale[3];
            ImGuizmo::DecomposeMatrixToComponents(gizmoMatrix, translate, rotate, scale);

            worldTransform.transform_.translate = { translate[0], translate[1], translate[2] };
            constexpr float kToRadian = 3.14159265f / 180.0f;
            worldTransform.transform_.rotate = {
                rotate[0] * kToRadian,
                rotate[1] * kToRadian,
                rotate[2] * kToRadian
            };
            worldTransform.transform_.scale = { scale[0], scale[1], scale[2] };

            // 行列の更新
            worldTransform.UpdateTransformMatrix();
        }

        bool isUsingGizmoNow = ImGuizmo::IsUsing();

        // ドラッグが終了した瞬間に、操作前と操作後の差分を1つのUndo履歴として積む
        if (!isUsingGizmoNow && wasUsingGizmo_) {
            Transform transformAfterGizmo;
            transformAfterGizmo.translate = worldTransform.transform_.translate;
            transformAfterGizmo.rotate = worldTransform.transform_.rotate;
            transformAfterGizmo.scale = worldTransform.transform_.scale;

            // 位置を登録
            if (selectedId_ >= 0) {
                commandHistory_.Execute(std::make_unique<TransformObjectCommand>(
                    staticObjectManager_, static_cast<uint32_t>(selectedId_),
                    transformBeforeGizmo_, transformAfterGizmo));
            }  
        }

        wasUsingGizmo_ = isUsingGizmoNow;
    }
}

void AddObjectBar::AddObjectFromPath(const std::string& filePath) {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string();

    // 拡張子を小文字に統一
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // モデルファイル以外がドロップされた場合は何もしない
    if (ext != ".obj" && ext != ".gltf") {
        return;
    }

    // 拡張子を含んだファイル名
    std::string modelName = path.filename().string();

    // 同じモデルを複数配置してもオブジェクト名が重複しないように番号を振る
    static int objectCount = 0;
    std::string objectName = modelName + "_" + std::to_string(objectCount++);

    // オブジェクトの追加コマンドを実行
    commandHistory_.Execute(std::make_unique<AddObjectCommand>(staticObjectManager_, objectName, modelName));
}

void AddObjectBar::ClearSelection() {
    selectObject_ = nullptr;
    selectedId_ = -1;
}
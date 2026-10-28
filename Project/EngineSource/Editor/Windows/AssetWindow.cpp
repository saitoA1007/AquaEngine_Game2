#include "pch.h"
#include "AssetWindow.h"
#include "ImGuiManager.h"

#include "TextureManager.h"

using namespace GameEngine;

AssetWindow::AssetWindow(TextureManager* textureManager) {
    textureManager_ = textureManager;

    uint32_t texHandle = textureManager_->GetHandleByName("errorIcon.png");
    errorTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(texHandle);

    uint32_t folderIconGH = textureManager_->GetHandleByName("folderIcon.png");
    uint32_t meshIconGH = textureManager_->GetHandleByName("meshIcon.png");
    uint32_t jsonIconGH = textureManager_->GetHandleByName("jsonIcon.png");
    folderIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(folderIconGH);
    meshIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(meshIconGH);
    jsonIconTextureHandle_ = textureManager_->GetTextureSrvHandlesGPU(jsonIconGH);

    // Resourceフォルダがなければ自動生成
    if (!std::filesystem::exists(resourcesPath)) {
        std::filesystem::create_directory(resourcesPath);
    }
}

void AssetWindow::Draw() {
    ImGui::Begin("Asset", &isActive);

    // 2列のテーブルを作成して画面を左右に分割する
    if (ImGui::BeginTable("AssetSplitter", 2, ImGuiTableFlags_Resizable)) {

        // 列の初期設定
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn("Contents", ImGuiTableColumnFlags_WidthStretch);

        // 右側のフォルダツリー
        ImGui::TableNextColumn();

        ImGuiTreeNodeFlags rootFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
        if (selectedPath == resourcesPath) { rootFlags |= ImGuiTreeNodeFlags_Selected; }

        if (ImGui::TreeNodeEx("Resources", rootFlags)) {
            if (ImGui::IsItemClicked()) { selectedPath = resourcesPath; }
            RenderDirectoryTree(resourcesPath);
            ImGui::TreePop();
        }

        //  右側の選択中のフォルダ内アセット一覧
        ImGui::TableNextColumn();

        // 上部に現在のパスを表示
        std::string relativePath = std::filesystem::relative(selectedPath, resourcesPath.parent_path()).string();
        ImGui::Text("Path: %s", relativePath.c_str());
        ImGui::Separator();

        // グリッド表示の実行
        RenderContentArea();

        ImGui::EndTable();
    }
    ImGui::End();
}

void AssetWindow::RenderDirectoryTree(const std::filesystem::path& path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        // ファイルは飛ばす
        if (!entry.is_directory()) { continue; } 

        std::string folderName = entry.path().filename().string();

        // 各ノードの表示フラグを設定
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if (selectedPath == entry.path()) {
            flags |= ImGuiTreeNodeFlags_Selected; // 選択中ならハイライト
        }

        // 中にサブフォルダがあるか確認
        bool hasSubDir = false;
        for (const auto& subEntry : std::filesystem::directory_iterator(entry.path())) {
            if (subEntry.is_directory()) { hasSubDir = true; break; }
        }
        if (!hasSubDir) { flags |= ImGuiTreeNodeFlags_Leaf; }

        // ツリーノードを作成
        bool open = ImGui::TreeNodeEx(folderName.c_str(), flags);

        // クリックされたら右ペインの表示対象をこのフォルダに切り替える
        if (ImGui::IsItemClicked()) {
            selectedPath = entry.path();
        }

        if (open) {
            // 再帰呼び出し
            RenderDirectoryTree(entry.path());
            ImGui::TreePop();
        }
    }
}

void AssetWindow::RenderContentArea() {
    if (!std::filesystem::exists(selectedPath)) return;

    // 1アイテムの横幅を 80px として列数を計算
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = (int)(panelWidth / 80.0f);
    if (columnCount < 1) { columnCount = 1; }

    if (ImGui::BeginTable("AssetGrid", columnCount)) {
        for (const auto& entry : std::filesystem::directory_iterator(selectedPath)) {
           
            std::string filename = entry.path().filename().string();
            std::string ext = entry.path().extension().string();

            // 拡張子を小文字に統一
            for (char& c : ext) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }

            // 特定の拡張子は飛ばす
            if (ext == ".mtl" || ext == ".bin" || ext == ".dds" || ext == ".wav" || ext == ".mp3") {
                continue;
            }

            // テクスチャハンドルを設定
            D3D12_GPU_DESCRIPTOR_HANDLE currentGpuHandle = errorTextureHandle_;

            if (entry.is_directory()) {
                // フォルダアイコン
                currentGpuHandle = folderIconTextureHandle_;
            } else if (ext == ".png" || ext == ".jpg") {
                uint32_t textureIndex = textureManager_->GetHandleByName(filename);
                currentGpuHandle = textureManager_->GetTextureSrvHandlesGPU(textureIndex);
            } else if (ext == ".obj" || ext == ".gltf") {
               // メッシュアイコン
                currentGpuHandle = meshIconTextureHandle_;
            } else if (ext == ".json") {
                // jsonデータ
                currentGpuHandle = jsonIconTextureHandle_;
            }

            // ハンドルがNullの場合はエラー画像
            if (currentGpuHandle.ptr == 0) {
                currentGpuHandle = errorTextureHandle_;
            }

            // DX12のハンドルをImGuiのIDにキャスト
            ImTextureID textureID = (ImTextureID)currentGpuHandle.ptr;
            // 画像の表示サイズ
            ImVec2 iconSize(64, 64); 

            // 次のセルへ移動
            ImGui::TableNextColumn();

            // アイテムごとにImGuiのIDが衝突しないようグループ化
            ImGui::PushID(filename.c_str());
            ImGui::BeginGroup();

            // それぞれの画像ボタンを表示
            std::string buttonID = "##Btn_" + filename;
            ImGui::ImageButton(buttonID.c_str(), textureID, iconSize);

            // ドラッグ判定
            if (!entry.is_directory()) {
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    std::string pathStr = entry.path().string();
                    ImGui::SetDragDropPayload("CONTENT_PATH", pathStr.c_str(), pathStr.size() + 1);
                    ImGui::Text("%s", filename.c_str());
                    ImGui::EndDragDropSource();
                }
            }

            // 画像の下にファイル名を表示
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 70.0f);
            ImGui::Text("%s", filename.c_str());
            ImGui::PopTextWrapPos();

            ImGui::EndGroup();
            ImGui::PopID();

            // ダブルクリック、ドラッグ&ドロップ
            if (entry.is_directory()) {
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    selectedPath = entry.path();
                    ImGui::EndTable();
                    return;
                }
            }

        }
        ImGui::EndTable();
    }
}
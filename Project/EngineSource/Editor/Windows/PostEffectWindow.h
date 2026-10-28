#pragma once
#include "IEditorWindow.h"
#include "ImGuiManager.h"
#include "NodeSystem/PostEffectGraph.h"
#include "NodeSystem/NodeBuilder.h"

namespace GameEngine {

	// 前方宣言
	class PostEffectManager;
	class TextureManager;
	class RenderPassController;

	class PostEffectWindow : public IEditorWindow {
	public:
		PostEffectWindow(PostEffectManager* postEffectManager, TextureManager* textureManager, RenderPassController* renderPassController);
		~PostEffectWindow();

		void Draw() override;
		std::string GetName() const override { return "PostEffect"; };

	private:
		// ポストエフェクト管理
		PostEffectManager* postEffectManager_ = nullptr;
		// テクスチャ管理
		TextureManager* textureManager_ = nullptr;
		// 描画のテクスチャ
		RenderPassController* renderPassController_ = nullptr;

		ax::NodeEditor::EditorContext* context_ = nullptr;

		// ノードの描画スタイル
		NodeUI::NodeStyle nodeStyle_;

		// 初回とリセット後にノード位置をエディタへ反映する
		bool applyPositions_ = true;
		// 右クリックしたキャンバス座標
		ImVec2 popupCanvasPos_{};

		bool showPreview_ = true;

		// 画像
		uint32_t headerBgHandle_ = 0;
		uint32_t iconRestoreHandle_ = 0;
		uint32_t iconSaveHandle_ = 0;
		uint32_t iconLoadHandle_ = 0;

		// 削除予約
		std::vector<int> pendingNodeDeletes_;
		std::vector<int> pendingLinkDeletes_;

		// 選択中のノード
		int selectedNodeId_ = -1;         
		// 左パネルの幅
		float inspectorWidth_ = 180.0f;   

	private:

		void DrawToolbar();

		void DrawInspector(PostEffectGraph& graph);
		void UpdateSelection();

		void DrawNodes(PostEffectGraph& graph);
		void DrawNode(PostEffectGraph& graph, PostEffectNode& node);
		void DrawLinks(const PostEffectGraph& graph);
		void HandleLinkCreation(PostEffectGraph& graph);
		void HandleDeletion(PostEffectGraph& graph);
		void DrawContextMenu();

		void ApplyPendingDeletes(PostEffectGraph& graph);
		bool IsDeletePending(const PostEffectGraph& graph, const Link& link) const;

		std::string GetPreviewPassName(const PostEffectGraph& graph, const PostEffectNode& node) const;
	};
}
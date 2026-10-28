#pragma once
#include <functional>
#include <filesystem>
#include "IEditorWindow.h"
#include "NodeSystem/MaterialGraph.h"
#include "NodeSystem/NodeBuilder.h" 
#include "ImGuiManager.h"
namespace ned = ax::NodeEditor;

namespace GameEngine {

	// 前方宣言
	class PSOManager;
	class TextureManager;

	class MaterialNodeWindow : public IEditorWindow {
	public:
		MaterialNodeWindow(PSOManager* psoManager, TextureManager* textureManager);
		~MaterialNodeWindow() = default;

		void Draw() override;
		std::string GetName() const override { return "MaterialNodeWindow"; }

		void Render(MaterialGraph& graph);

	private:
		PSOManager* psoManager_ = nullptr;
		TextureManager* textureManager_ = nullptr;
		ned::EditorContext* context_ = nullptr;

		NodeUI::NodeStyle nodeStyle_;
		uint32_t headerBgHandle_ = 0;

		MaterialGraph* currentGraph_ = nullptr;
		std::string currentMaterialName_ = "";
		// ディレクトリ内のマテリアル名一覧
		std::vector<std::string> materialList_;

		// マテリアルノードデータが保存されている
		std::unordered_map<std::string, MaterialGraph> graphData_;

		// マテリアルノードデータが保存されているディレクトリ
		const std::string kDirectoryPath = "EngineSource/Resources/Json/";

		// 接続開始ピン
		int newLinkPin_ = -1;

		bool dirtyFlag_ = false;

		// 登録されているノード
		std::unordered_map<std::string, std::function<void(MaterialGraph&)>> registerNode_;

		// 左側の表示範囲
		float leftPanelWidth_ = 240.0f;
		// 最小幅
		const float kMinPanelWidth_ = 150.0f;  
		// 最大幅
		const float kMaxPanelWidth_ = 600.0f;  

	private:

		void HandleLinkCreation(MaterialGraph& graph);

		void HandleLinkDeletion(MaterialGraph& graph);

		void HandleContextMenu(MaterialGraph& graph);

		// ノードを描画
		void DrawNode(MaterialGraph& graph, IMaterialNode& node);

		// マテリアルノード用のツールバー
		void DrawMaterialToolbar();

		void InitializeAndLoadAllMaterials();
		void UpdateMaterialList();
		void CreateNewMaterial(const std::string& name);
		void SelectMaterial(const std::string& name);
		void SaveCurrentMaterial();

		// ノードを登録
		template<typename T>
		void RegisterNode(std::string nodeName) {
			registerNode_[nodeName] = [](MaterialGraph& graph) {
				graph.nodes.push_back(std::make_unique<T>(graph));
			};
		}
	};
}


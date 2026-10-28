#pragma once
#include <array>
#include <unordered_set>
#include "ConstantBuffer.h"
#include "SrvManager.h"
#include "PSO/Core/PSOManager.h"
#include "RenderPass/RenderPassController.h"
#include "IPostEffect.h"
#include "PostEffectData.h"
#include "PostEffectRegistry.h"
#include "NodeSystem/PostEffectGraph.h"
#include "JsonSerializer.h"

namespace GameEngine {

	class PostEffectManager {
	public:

		// 初期化処理
		void Initialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController);

		// 描画コマンドの解放
		void Execute();

	public:

		template <class T>
		T* GetPostEffect(const std::string& name) {
			return dynamic_cast<T*>(registry_.Find(name));
		}

		// 追加メニュー用
		std::vector<std::string> GetEffectTypeNames() const { return registry_.GetTypeNames(); }

		const std::unordered_map<std::string, std::unique_ptr<IPostEffect>>& GetEffects() const { return registry_.GetAll(); }

		// ノードデータを受け取る
		PostEffectGraph& GetGraph() { return graph_; }

		// ノードデータをリセット
		void ResetGraph();

		// ノードを作る
		PostEffectNode* CreateEffectNode(const std::string& typeName, const std::string& name = "");
		// ノードを消す
		void DestroyEffectNode(int nodeId);
		// 保存パスの切り替え
		void SetPersistent(int nodeId, bool enable);

		// ノードの保存
		bool SaveGraph(const std::string& filePath = kGraphFilePath);
		// ノードのロード
		bool LoadGraph(const std::string& filePath = kGraphFilePath);

		// パス
		const std::string& GetFinalPassName() const { return currentPassName_; }

	private:

		// 保存先
		static constexpr const char* kGraphFilePath = "Resources/Json/PostEffect/PostEffectGraph.json";

		// 使い回し用のパス
		static constexpr uint32_t kPoolSize = 3;
		const std::array<std::string, kPoolSize> poolPassNames_ = {
			"PostProcess_RT0", "PostProcess_RT1", "PostProcess_RT2"
		};

	private:

		// 実行順の1ノード分。グラフの構造が変わった時だけ作り直す
		struct ExecuteStep {
			PostEffectNode* node = nullptr;
			std::vector<int> inputSteps; // 各入力の元になるステップ。-1はシーン
			int consumers = 0;           // 出力を読む実行対象ノードの数
		};

		// 描画結果1つ分
		struct Resource {
			uint32_t srvIndex = 0;
			int poolSlot = -1;
			int refCount = 0;
			const std::string* passName = nullptr; // 実際に描かれたパス名
		};

		// 1フレーム分の描画結果とプールの空き状況。容量を残して使い回す
		class FrameResources {
		public:
			void Reset();
			int Add(const Resource& r);
			Resource& operator[](int idx) { return resources_[idx]; }
			// 空いているプールのスロットを確保する。無ければ-1
			int AcquireSlot();
			// 読み手が居なくなったらプールに返す
			void Release(int idx);

		private:
			std::vector<Resource> resources_;
			std::array<bool, kPoolSize> slotBusy_{};
		};

	private:
		ID3D12GraphicsCommandList* commandList_ = nullptr;
		SrvManager* srvManager_ = nullptr;
		RenderPassController* renderPassController_ = nullptr;

		std::string currentPassName_ = "";

		// エフェクトの種類、インスタンス、PSOを管理
		PostEffectRegistry registry_;

		// 作られている描画パスを管理
		std::unordered_set<std::string> createdPasses_;

		// ノードデータ
		PostEffectGraph graph_;
		// ノードIDの実行順
		std::vector<int> executeOrder_;
		// executeOrder_から作った実行計画
		std::vector<ExecuteStep> executeSteps_;

		// 毎フレームの作業領域
		FrameResources frameResources_;
		std::vector<int> stepRes_;
		std::vector<int> inRes_;

	private:

		// 作成していなければ描画パスを作る
		void EnsurePass(const std::string& name) {
			if (createdPasses_.insert(name).second) {
				renderPassController_->AddPass(name);
			}
		}

		void BuildDefaultGraph();
		void RebuildOrder();
		// executeOrder_から入力元と読み手の数を求める
		void BuildExecuteSteps();

		// グラフを実行して最終的な出力パス名を返す
		std::string ExecuteGraph(const std::string& scenePass);

		// エフェクトを1つ描画する
		void DrawEffect(IPostEffect* fx, const std::string& outPass);

		// 文字列キーでPSOをセット
		void PreDraw(const std::string& psoName);
	};
}

#pragma once
#include <vector>
#include <map>
#include <unordered_map>

#include "PSO/Core/DrawPSOData.h"
#include "DrawRequest.h"
#include "RenderPass/RenderPassController.h"
#include "RenderQueue.h"
#include "TLAS.h"

namespace GameEngine {

	// 前方宣言
	class PSOManager;
	class BufferRefManager;
	class RaytracingPipeline;

	class SceneRenderManager {
	public:

		void Initialize(ID3D12GraphicsCommandList4* commandList, SrvManager* srvManager, PSOManager* psoManager, RenderPassController* renderPassController,
			RaytracingPipeline* raytracingPipeline, BufferRefManager* bufferRefManager, RenderQueue* renderQueue);

		// フレーム開始前処理
		void Begin();

		// 描画コマンドを解放する
		void Execute();

	private:
		ID3D12GraphicsCommandList4* commandList_ = nullptr;
		RenderPassController* renderPassController_ = nullptr;
		RaytracingPipeline* raytracingPipeline_ = nullptr;
		SrvManager* srvManager_ = nullptr;
		BufferRefManager* bufferRefManager_ = nullptr;
		RenderQueue* renderQueue_ = nullptr;

		// 破片の描画に使用するコマンドルートシグネチャ。後で配置の変更をする
		ID3D12CommandSignature* fractureCommandSignature_ = nullptr;
		ID3D12CommandSignature* iceFractureCommandSignature_ = nullptr;

		// 描画パスの実行順
		std::vector<std::string> passExecuteOrder_;

		// psoのリスト
		std::unordered_map<std::string, DrawPsoData> psoList_;

		// レイトレーシング用の描画モデル管理
		TLAS tlas_;

		// bufferが存在しているsrvのスタート位置
		uint32_t bufferStartSrvIndex_ = 0;

		// レイトレーシングでの最大描画数
		uint32_t maxRayInstanceNum_ = 200;

		// 現在のpso
		std::string currentPsoName_;

		// 最終的に画面に描画させるパスの名前
		std::string finalPassName_ = "";
		// ラスタライズ描画で最終的に描画させるパス
		std::string rasterizeFinalPassName_ = "";
		// レイトレ描画で最終的に描画させるパス
		std::string raytracingFinalPassName_ = "";

		// 描画の有効状態
		bool enableDrawRaytracing_ = false;
		bool enableDrawRasterize_ = false;
		bool enableDrawRasterizeTranslucent_ = false;

	private:

		// パスの実行順を登録
		void RegisterPassOrder(const std::vector<std::string>& order) {
			passExecuteOrder_ = order;
		}

		/// <summary>
		/// PSOManagerから名前を指定して動的に登録する。
		/// </summary>
		void RegisterPSO(const std::string& name, PSOManager* psoManager);

		/// <summary>
		/// 使用するPSOを登録する
		/// </summary>
		/// <param name="psoManager"></param>
		void RegisterPSOs(PSOManager* psoManager);

		/// <summary>
		/// 使用するレンダーパスを作成する
		/// </summary>
		void CreateRenderPasses();

		// 文字列キーでPSOをセット
		void PreDraw(const std::string& psoName);

		// 描画コマンドを解放
		void Execute3dRequest(const Draw3dRequest& request);
		void Execute2dRequest(const Draw2dRequest& request);

		// レイトレーシングの描画
		void DrawRaytracing();

		// ラスタライズの描画コマンドを解放
		void RasterizeExecute();

		// ラスタライズの半透明描画コマンドを解放
		void RasterizeTranslucentExecute();

		// レイトレの描画コマンドを解放
		void RaytracingExecute();

		// 2Dのスプライトを描画
		void Draw2dExecute();

		// レイトレとラスタライズの描画を合成する
		void LightingComposite();

		// 深度値をコピーする
		void CopyRaytracingDepth();

		void Composite();

		// ヘルパー

		// 描画コマンドが無いパスをクリア
		void ClearPassOnly(const std::string& passName);
	};
}

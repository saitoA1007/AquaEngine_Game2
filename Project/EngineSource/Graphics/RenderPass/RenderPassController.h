#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include "RenderPass.h"
#include "RenderTextureManager.h"
#include "Debug/PixMarker.h"

namespace GameEngine {

	class RenderPassController final {
	public:
		RenderPassController() = default;
		~RenderPassController() = default;

		// 初期化処理
		void Initialize(RenderTextureManager* renderTextureManager, ID3D12GraphicsCommandList* commandList);

		// パスを作成する
		void AddPass(const std::string& name, RenderTextureMode mode = RenderTextureMode::RtvAndDsv,uint32_t wid = 1280,uint32_t hei = 720,
			Vector4 clearColor = { 0.2f,0.2f,0.2f,1.0f },DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);

		// 描画前に呼び出す
		void PrePass(const std::string& name);
		void PrePass(std::vector<std::string> names,const std::string dsvName = "");
		void PostPass(const std::string& name);
		void SwitchToUAV(const std::string& name);
		void InsertUavBarrier(const std::string& name);
		// 深度をコピーするため
		void SetOnlyDsvRenderTarget(const std::string& name);
		// レンダーパスのクリアをおこなう
		void ClearRenderPass(const std::string& name);

		// 描画の最終パスの設定
		void SetSceneFinalPass(const std::string& name);
		const std::string& GetSceneFinalPass() const { return sceneFinalPassName_; }

		// ポストエフェクトの最終パスの設定
		void SetPostProcessFinalPass(const std::string& name);
		const std::string& GetPostProcessFinalPass() const { return postProcessFinalPassName_; }

		// 最終的に画面に出すためのパスの設定
		void SetPresentPass(const std::string& name);
		const std::string& GetPresentPass() const { return presentPassName_; }

		// 描画範囲を設定
		void SetDrawRange(const std::string& name, const uint32_t& width, const uint32_t& height, const uint32_t& left = 0, const uint32_t& top = 0);

		CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(const std::string& name);
		uint32_t GetSrvIndex(const std::string& name);
		uint32_t GetUavIndex(const std::string& name);
		uint32_t GetDepthSrvIndex(const std::string& name);

		/// <summary>
		/// 開きっぱなしのPIXイベントを全て閉じる。コマンドリストをCloseする直前に必ず呼ぶこと。
		/// </summary>
		void CloseAllPixEvents();

	private:

		// PIXイベントを開いてスタックに積む
		void BeginPixEvent(const std::string& name);
		// 指定名のPIXイベントを閉じる
		void EndPixEvent(const std::string& name);

	private:
		RenderPassController(const RenderPassController&) = delete;
		RenderPassController& operator=(const RenderPassController&) = delete;

		RenderTextureManager* renderTextureManager_ = nullptr;
		ID3D12GraphicsCommandList* commandList_ = nullptr;

		std::unordered_map<std::string, std::unique_ptr<RenderPass>> renderPassList_;

		// 描画の最終パス
		std::string sceneFinalPassName_ = "";
		CD3DX12_GPU_DESCRIPTOR_HANDLE sceneFinalPassSrvHandle_;
		// ポストエフェクトの最終パス
		std::string postProcessFinalPassName_ = "";
		CD3DX12_GPU_DESCRIPTOR_HANDLE postProcessFinalPassSrvHandle_;
		// 最終的に画面に出すためのパス
		std::string presentPassName_ = "";
		CD3DX12_GPU_DESCRIPTOR_HANDLE presentPassSrvHandle_;

		// 現在開いているPIXイベント名のスタック
		std::vector<std::string> openPixEvents_;

		uint32_t width_ = 0;
		uint32_t height_ = 0;
	};
}


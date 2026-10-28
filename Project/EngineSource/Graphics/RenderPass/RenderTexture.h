#pragma once
#include "SrvResource.h"
#include "RtvManager.h"
#include "DsvManager.h"
#include "Vector4.h"
#include "Debug/DebugName.h"
#include <string>

namespace GameEngine {

	// 描画タイプ
	enum class RenderTextureMode {
		RtvOnly,    // RTVのみ
		DsvOnly,    // Dsvのみ
		RtvAndDsv,  // RTVとDSV
		UavOnly,    // ComputeShader/Raytracing用
		RtvAndUav,  // 描画情報とUav両方

		MaxCount
	};

	// リソースの現在の状態を追跡する列挙型
	enum class ColorResourceState {
		ShaderResource,   // D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		RenderTarget,     // D3D12_RESOURCE_STATE_RENDER_TARGET
		UnorderedAccess,  // D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	};

	class RenderTexture : public SrvResource {
	public:
		RenderTexture() = default;
		~RenderTexture();

		static void StaticInitialize(RtvManager* rtvManager, DsvManager* dsvManager) {
			rtvManager_ = rtvManager;
			dsvManager_ = dsvManager;
		}

		void Create(
			uint32_t width, uint32_t height,
			RenderTextureMode mode = RenderTextureMode::RtvAndDsv,
			DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
			Vector4 clearColor = {0.2f,0.2f,0.2f,1.0f}
		);

		// RTV状態へ遷移
		void TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList);
		// SRV状態へ遷移
		void TransitionToShaderResource(ID3D12GraphicsCommandList* commandList);
		// UAV状態へ遷移
		void TransitionToUnorderedAccess(ID3D12GraphicsCommandList* commandList);

		// UAV書き込み後にGPU側の書き込みを完了させるバリアを挿入する
		void InsertUavBarrier(ID3D12GraphicsCommandList* commandList);

		D3D12_CPU_DESCRIPTOR_HANDLE& GetRtvHandle() { return rtvHandle_; }
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() { return srvGpuHandle_; }
		D3D12_CPU_DESCRIPTOR_HANDLE& GetDsvHandle() { return dsvHandle_; }
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetUavGpuHandle() const { return uavGpuHandle_; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetUavCpuHandle() const { return uavCpuHandle_; }

		uint32_t GetRtvIndex() const { return rtvIndex_; }
		uint32_t GetSrvIndex() { return srvIndex_; }
		uint32_t GetUavIndex() const { return uavIndex_; }
		uint32_t GetDepthSrvIndex() const { return depthSrvIndex_; }

		uint32_t GetWidth() const { return width_; }
		uint32_t GetHeight() const { return height_; }

		RenderTextureMode GetMode() const { return mode_; }
		ColorResourceState GetColorState() const { return colorState_; }

		/// <summary>
		/// PIX、デバッグレイヤー上で識別できるように名前を設定する
		/// </summary>
		/// <param name="name">レンダーパス名</param>
		void SetDebugNames(const std::string& name);
	private:
		RenderTexture(const RenderTexture&) = delete;
		RenderTexture& operator=(const RenderTexture&) = delete;
		RenderTexture(RenderTexture&&) = delete;
		RenderTexture& operator=(RenderTexture&&) = delete;

		static RtvManager* rtvManager_;
		static DsvManager* dsvManager_;

		// カラーリソース
		uint32_t rtvIndex_ = 0;
		uint32_t srvIndex_ = 0;
		// rtvハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_;
		// srvハンドル
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle_;
		// クリアカラー
		Vector4 clearColor_ = { 0.2f,0.2f,0.2f,1.0f };

		// UAVリソース
		uint32_t uavIndex_ = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE   uavCpuHandle_ = {};
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavGpuHandle_ = {};

		// 深度リソース
		Microsoft::WRL::ComPtr<ID3D12Resource> depthResource_ = nullptr;
		uint32_t dsvIndex_ = 0;
		uint32_t depthSrvIndex_ = 0;
		// dsvハンドル
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle_;
		CD3DX12_GPU_DESCRIPTOR_HANDLE   depthSrvGpuHandle_ = {};

		uint32_t width_ = 0;
		uint32_t height_ = 0;
		// 通常描画
		RenderTextureMode mode_ = RenderTextureMode::RtvAndDsv;
		ColorResourceState colorState_ = ColorResourceState::ShaderResource;
		// 深度リソース用の状態フラグ
		bool isDepthTarget_ = false;
	private:

		/// <summary>
		/// カラーリソースとそのRTV,SRVを作成する
		/// </summary>
		/// <param name="width"></param>
		/// <param name="height"></param>
		/// <param name="format"></param>
		void CreateColorTarget(uint32_t width, uint32_t height, DXGI_FORMAT format);

		/// <summary>
		/// UAV専用リソースとUAV,SRVを作成する
		/// </summary>
		void CreateUavTarget(uint32_t width, uint32_t height, DXGI_FORMAT format);

		/// <summary>
		/// 深度リソースとそのDSV,SRVを作成する
		/// </summary>
		/// <param name="width"></param>
		/// <param name="height"></param>
		void CreateDepthTarget(uint32_t width, uint32_t height);

		/// <summary>
		/// SRGBフォーマットをUAV互換のリニアフォーマットに変換する。
		/// </summary>
		static DXGI_FORMAT ToUavCompatibleFormat(DXGI_FORMAT format);
	};
}
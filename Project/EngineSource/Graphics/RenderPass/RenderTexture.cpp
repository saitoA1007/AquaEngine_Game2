#include "pch.h"
#include "RenderTexture.h"
#include <cassert>
#include "DepthStencilTexture.h"
#include "CreateBufferResource.h"
using namespace GameEngine;

RtvManager* RenderTexture::rtvManager_ = nullptr;
DsvManager* RenderTexture::dsvManager_ = nullptr;

RenderTexture::~RenderTexture() {
	// カラーRTのデスクリプタ解放
	if (mode_ != RenderTextureMode::DsvOnly) {
		// RTVインデックス解放
		if (rtvIndex_ != 0) {
			rtvManager_->ReleaseIndex(rtvIndex_);
		}
		// SRVインデックス解放
		if (srvIndex_ != 0) {
			srvManager_->ReleaseIndex(srvIndex_);
		}
		// UAVインデックス解放
		if (uavIndex_ != 0) {
			srvManager_->ReleaseIndex(uavIndex_);
		}
	}

	// 深度RTのデスクリプタ解放
	if (mode_ != RenderTextureMode::RtvOnly &&
		mode_ != RenderTextureMode::UavOnly &&
		mode_ != RenderTextureMode::RtvAndUav)
	{
		if (dsvIndex_ != 0) {
			dsvManager_->ReleaseIndex(dsvIndex_);
		}

		// RtvAndDsvの場合、深度用SRVインデックスはsrvIndexとは別
		if (mode_ == RenderTextureMode::RtvAndDsv && depthSrvIndex_ != 0) {
			srvManager_->ReleaseIndex(depthSrvIndex_);
		}
	}
}

void RenderTexture::Create(uint32_t width, uint32_t height, RenderTextureMode mode, DXGI_FORMAT colorFormat, Vector4 clearColor) {
	width_ = width;
	height_ = height;
	mode_ = mode;
	clearColor_ = clearColor;

	switch (mode_) {
	case RenderTextureMode::RtvOnly:
		CreateColorTarget(width, height, colorFormat);
		colorState_ = ColorResourceState::ShaderResource;
		break;

	case RenderTextureMode::DsvOnly:
		CreateDepthTarget(width, height);
		// DsvOnlyでは深度SRVをメインSRVとして扱う
		srvIndex_ = depthSrvIndex_;
		srvGpuHandle_ = depthSrvGpuHandle_;
		break;

	case RenderTextureMode::RtvAndDsv:
		CreateColorTarget(width, height, colorFormat);
		CreateDepthTarget(width, height);
		colorState_ = ColorResourceState::ShaderResource;
		break;

	case RenderTextureMode::UavOnly:
		// UAV専用SRGB非対応フォーマットへ変換してリソース生成
		CreateUavTarget(width, height, ToUavCompatibleFormat(colorFormat));
		colorState_ = ColorResourceState::UnorderedAccess;
		break;

	case RenderTextureMode::RtvAndUav:
		// RTVとUAVを同一リソースにして両フラグを付与して生成する
		CreateColorTarget(width, height, ToUavCompatibleFormat(colorFormat));
		CreateUavTarget(width, height, ToUavCompatibleFormat(colorFormat));
		colorState_ = ColorResourceState::ShaderResource;
		break;

	default:
		assert(false && "未対応の RenderTextureMode です");
		break;
	}
}

void RenderTexture::TransitionToRenderTarget(ID3D12GraphicsCommandList* commandList) {
	assert((mode_ == RenderTextureMode::RtvOnly || mode_ == RenderTextureMode::RtvAndDsv ||
		mode_ == RenderTextureMode::RtvAndUav || mode_ == RenderTextureMode::DsvOnly) &&
		"このモードはRTV遷移をサポートしていません");

	// 既にRTV状態
	if (colorState_ == ColorResourceState::RenderTarget) { return; }

	if (mode_ != RenderTextureMode::DsvOnly) {
		D3D12_RESOURCE_STATES stateBefore;
		if (colorState_ == ColorResourceState::UnorderedAccess) {
			stateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		} else {
			stateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		}
		// rtvに遷移するバリア
		TransitionResource(commandList, resource_.Get(), stateBefore, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}

	colorState_ = ColorResourceState::RenderTarget;

	// 深度リソースのRTV遷移
	if ((mode_ == RenderTextureMode::RtvAndDsv || mode_ == RenderTextureMode::DsvOnly) && depthResource_ && !isDepthTarget_) {
		TransitionResource(commandList, depthResource_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		isDepthTarget_ = true;
	}
}

void RenderTexture::TransitionToShaderResource(ID3D12GraphicsCommandList* commandList) {
	// 既にSRV状態
	if (colorState_ == ColorResourceState::ShaderResource && !isDepthTarget_) {return; }

	// カラー,UAVリソースのSRV遷移
	if (mode_ != RenderTextureMode::DsvOnly) {
		if (colorState_ != ColorResourceState::ShaderResource && resource_) {
			D3D12_RESOURCE_STATES stateBefore;
			if (colorState_ == ColorResourceState::UnorderedAccess) {
				stateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			} else {
				stateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
			}
			// srvに遷移するバリア
			TransitionResource(commandList, resource_.Get(), stateBefore, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}

	colorState_ = ColorResourceState::ShaderResource;

	// 深度リソースのSRV遷移
	if ((mode_ == RenderTextureMode::DsvOnly || mode_ == RenderTextureMode::RtvAndDsv) && depthResource_ && isDepthTarget_) {
		TransitionResource(commandList, depthResource_.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		isDepthTarget_ = false;
	}
}

void RenderTexture::TransitionToUnorderedAccess(ID3D12GraphicsCommandList* commandList) {
	assert((mode_ == RenderTextureMode::UavOnly || mode_ == RenderTextureMode::RtvAndUav) &&
		"このモードはUAV遷移をサポートしていません");

	// 既にUAV状態
	if (colorState_ == ColorResourceState::UnorderedAccess) { return; }

	D3D12_RESOURCE_STATES stateBefore;
	if (colorState_ == ColorResourceState::RenderTarget) {
		stateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	} else {
		stateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	}

	// uavに遷移するバリア
	TransitionResource(commandList, resource_.Get(), stateBefore, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	colorState_ = ColorResourceState::UnorderedAccess;
}

void RenderTexture::InsertUavBarrier(ID3D12GraphicsCommandList* commandList) {
	assert((mode_ == RenderTextureMode::UavOnly || mode_ == RenderTextureMode::RtvAndUav) &&
		"このモードはUAVバリアをサポートしていません");

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.UAV.pResource = resource_.Get();
	commandList->ResourceBarrier(1, &barrier);
}

void RenderTexture::SetDebugNames(const std::string& name) {
	SetDebugName(resource_.Get(), "RT_" + name);
	SetDebugName(depthResource_.Get(), "Depth_" + name);
}

void RenderTexture::CreateColorTarget(uint32_t width, uint32_t height, DXGI_FORMAT format) {
	// カラーリソース作成
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	// RtvAndUavでは同一リソースにUAVフラグも立てる
	if (mode_ == RenderTextureMode::RtvAndUav) {
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	// クリアカラー
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = format;
	clearValue.Color[0] = clearColor_.x;
	clearValue.Color[1] = clearColor_.y;
	clearValue.Color[2] = clearColor_.z;
	clearValue.Color[3] = clearColor_.w;

	// リソース作成
	D3D12_RESOURCE_DESC desc = CreateTexture2dDesc(width, height, format, flags);
	resource_ = CreateResource(device_, desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue);

	// rtvの完成
	rtvIndex_ = rtvManager_->CreateView(resource_.Get(), format);
	rtvHandle_ = rtvManager_->GetCPUHandle(rtvIndex_);

	// SRVの作成
	srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::System);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	D3D12_CPU_DESCRIPTOR_HANDLE srvCPU = srvManager_->GetCPUHandle(srvIndex_);
	srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
	device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCPU);
}

void RenderTexture::CreateUavTarget(uint32_t width, uint32_t height, DXGI_FORMAT format) {

	// UavOnlyのみ新規リソースを作成する
	if (mode_ == RenderTextureMode::UavOnly) {

		// リソース作成
		D3D12_RESOURCE_DESC desc = CreateTexture2dDesc(width, height, format, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		resource_ = CreateResource(device_, desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// SRV作成
		srvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::System);
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;

		D3D12_CPU_DESCRIPTOR_HANDLE srvCPU = srvManager_->GetCPUHandle(srvIndex_);
		srvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(srvIndex_));
		device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvCPU);
	}

	// UAVビューを作成する
	assert(resource_ && "UAVビュー作成前にリソースが存在しません");

	// uav作成
	uavIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::System);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	uavCpuHandle_ = srvManager_->GetCPUHandle(uavIndex_);
	uavGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(uavIndex_));
	device_->CreateUnorderedAccessView(resource_.Get(), nullptr, &uavDesc, uavCpuHandle_);
}

void RenderTexture::CreateDepthTarget(uint32_t width, uint32_t height) {

	// 深度リソース作成
	D3D12_RESOURCE_DESC desc = CreateTexture2dDesc(width, height, DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	// クリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	// 深度リソース作成
	depthResource_ = CreateResource(device_, desc, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &depthClearValue);

	// dsvの作成
	dsvIndex_ = dsvManager_->CreateView(depthResource_.Get(), DXGI_FORMAT_D32_FLOAT);
	dsvHandle_ = dsvManager_->GetCPUHandle(dsvIndex_);

	// 深度SRV作成
	depthSrvIndex_ = srvManager_->AllocateSrvIndex(SrvHeapType::System);
	D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
	depthSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	depthSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	depthSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;// 2Dテクスチャ
	depthSrvDesc.Texture2D.MipLevels = 1;
	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvCPU = srvManager_->GetCPUHandle(depthSrvIndex_);
	depthSrvGpuHandle_ = static_cast<CD3DX12_GPU_DESCRIPTOR_HANDLE>(srvManager_->GetGPUHandle(depthSrvIndex_));
	device_->CreateShaderResourceView(depthResource_.Get(), &depthSrvDesc, depthSrvCPU);
}

DXGI_FORMAT RenderTexture::ToUavCompatibleFormat(DXGI_FORMAT format) {
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:  return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8A8_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:  return DXGI_FORMAT_B8G8R8X8_UNORM;
	default: return format;
	}
}

#include "pch.h"
#include"DXDepthStencil.h"
#include"DepthStencilTexture.h"
#include"LogManager.h"
using namespace GameEngine;


void DXDepthStencil::Initialize(ID3D12Device* device, ID3D12DescriptorHeap* dsvHeap, uint32_t width, uint32_t height, SrvManager* srvManager) {

	LogManager::GetInstance().Log("Start　Create DepthStencil");

	// DepthStencilTextureをウィンドウのサイズで作成DXGI_FORMAT_D24_UNORM_S8_UINT
	depthStencilResource_ = CreateDepthStencilTextureResource(device, width, height);

	// DSVの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	// DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// 深度値を読み込めるようにSRVを作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;// 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;

	uint32_t index = srvManager->AllocateSrvIndex(SrvHeapType::System);
	D3D12_CPU_DESCRIPTOR_HANDLE srvCPUHandle = srvManager->GetCPUHandle(index);
	depthSRVHandle_ = srvManager->GetGPUHandle(index);

	// オブジェクト描画用SRV
	device->CreateShaderResourceView(depthStencilResource_.Get(), &srvDesc, srvCPUHandle);

	LogManager::GetInstance().Log("End　Create DepthStencil");
}


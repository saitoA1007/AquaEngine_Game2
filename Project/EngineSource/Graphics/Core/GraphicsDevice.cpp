#include "pch.h"
#include "GraphicsDevice.h"
#include <cassert>
#include <format>
#include "LogManager.h"
#include "ResourceGarbageCollector.h"
using namespace GameEngine;

#ifdef _DEBUG
namespace {

	// GPUが実行していたコマンドの種類を読める名前に変換する
	const char* BreadcrumbOpToString(D3D12_AUTO_BREADCRUMB_OP op) {
		switch (op) {
		case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:              return "DrawInstanced";
		case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:       return "DrawIndexedInstanced";
		case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:            return "ExecuteIndirect";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:                   return "Dispatch";
		case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:           return "CopyBufferRegion";
		case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:          return "CopyTextureRegion";
		case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:               return "CopyResource";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:      return "ClearRenderTargetView";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:   return "ClearUnorderedAccessView";
		case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:      return "ClearDepthStencilView";
		case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:            return "ResourceBarrier";
		case D3D12_AUTO_BREADCRUMB_OP_PRESENT:                    return "Present";
		case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE: return "BuildRaytracingAccelerationStructure";
		case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:  return "CopyRaytracingAccelerationStructure";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:               return "DispatchRays";
		case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:          return "SetPipelineState1";
		case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:               return "DispatchMesh";
		default:                                                  return "Other";
		}
	}

	// GPUがどのコマンドで止まったか、どのリソース付近でフォルトしたかをログへ出す
	void LogDREDInfo(ID3D12Device5* device) {
		LogManager& log = LogManager::GetInstance();

		Microsoft::WRL::ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
		if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dred)))) {
			log.Log("[DRED] DRED interface is not available");
			return;
		}

		// 実行したコマンドの履歴
		D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs{};
		if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&breadcrumbs))) {
			const D3D12_AUTO_BREADCRUMB_NODE1* node = breadcrumbs.pHeadAutoBreadcrumbNode;
			while (node != nullptr) {
				uint32_t completed = (node->pLastBreadcrumbValue != nullptr) ? *node->pLastBreadcrumbValue : 0;

				// 全部完了しているコマンドリストは、ハングの原因ではないので飛ばす
				if (completed < node->BreadcrumbCount) {
					log.Log(std::format("[DRED] CommandList \"{}\" stopped at {}/{}",
						node->pCommandListDebugNameA ? node->pCommandListDebugNameA : "(no name)",
						completed, node->BreadcrumbCount));

					// 完了した次のコマンドが、GPUを止めた原因のコマンド
					log.Log(std::format("[DRED]   >>> hung on: {}",
						BreadcrumbOpToString(node->pCommandHistory[completed])));

					// 直前に何をしていたかも数件出す
					uint32_t begin = (completed >= 5) ? completed - 5 : 0;
					for (uint32_t i = begin; i < completed; ++i) {
						log.Log(std::format("[DRED]   before[{}]: {}", i, BreadcrumbOpToString(node->pCommandHistory[i])));
					}
				}
				node = node->pNext;
			}
		}

		// ページフォルトの情報
		D3D12_DRED_PAGE_FAULT_OUTPUT1 pageFault{};
		if (SUCCEEDED(dred->GetPageFaultAllocationOutput1(&pageFault)) && pageFault.PageFaultVA != 0) {
			log.Log(std::format("[DRED] PageFault VA = 0x{:016X}", pageFault.PageFaultVA));

			for (const D3D12_DRED_ALLOCATION_NODE1* n = pageFault.pHeadExistingAllocationNode; n != nullptr; n = n->pNext) {
				log.Log(std::format("[DRED]   existing allocation: \"{}\"", n->ObjectNameA ? n->ObjectNameA : "(no name)"));
			}
			// 解放済みリソースがここに出たら、GPU実行中に破棄してしまったのが原因
			for (const D3D12_DRED_ALLOCATION_NODE1* n = pageFault.pHeadRecentFreedAllocationNode; n != nullptr; n = n->pNext) {
				log.Log(std::format("[DRED]   recently freed: \"{}\"", n->ObjectNameA ? n->ObjectNameA : "(no name)"));
			}
		}
	}
}
#endif

void GraphicsDevice::Initialize(HWND hwnd, uint32_t width, uint32_t height) {
    // 初期化を開始するログ
    LogManager::GetInstance().Log("GraphicsDevice Class start Initialize");

    // デバイスを生成
    device_ = std::make_unique<DXDevice>();
    device_->Initialize();

#ifdef _DEBUG
    debugger_ = std::make_unique<DXDebugger>();
    debugger_->InitializeDebugLayer(device_->GetDevice());
#endif

    // コマンドの初期化
    command_ = std::make_unique<DXCommand>();
    command_->Initialize(device_->GetDevice());

    // スワップチェーンの作成
    swapChain_ = std::make_unique<DXSwapChain>();
    swapChain_->Initialize(hwnd, width, height, device_->GetFactory(), command_->GetQueue());

    // SRVを生成する
    srvManager_ = std::make_unique<SrvManager>();
    srvManager_->Initialize(device_->GetDevice());

    // RTVシステムを生成
    rtvManager_ = std::make_unique<RtvManager>();
    rtvManager_->Initialize(device_->GetDevice());

    // dsvを生成する
    dsvManager_ = std::make_unique<DsvManager>();
    dsvManager_->Initialize(device_->GetDevice());

    // RTV、DSVの生成
    renderTarget_ = std::make_unique<DXRenderTarget>();
    renderTarget_->Initialize(device_->GetDevice(), swapChain_->GetSwapChain());

    // 深度ステンシルの初期化
    depthStencil_ = std::make_unique<DXDepthStencil>();
    depthStencil_->Initialize(device_->GetDevice(), renderTarget_->GetDSVHeap(), width, height, srvManager_.get());

    // フェンスの生成
    fence_ = std::make_unique<DXFence>();
    fence_->Initialize(device_->GetDevice());

    // フェンスを取得
    ResourceGarbageCollector::GetInstance().SetFence(fence_.get());

    // ビューポート、シザー矩形を生成
    viewportState_ = std::make_unique<DXViewportState>();
    viewportState_->Initialize(width, height);

    // 初期化を終了するログ
    LogManager::GetInstance().Log("GraphicsDevice Class end Initialize\n");
}

void GraphicsDevice::CheckDeviceStatus() {
    HRESULT hr = device_->GetDevice()->GetDeviceRemovedReason();

    if (!SUCCEEDED(hr)) {
        LogManager::GetInstance().Log(std::format("Device removed. reason = 0x{:08X}", static_cast<uint32_t>(hr)));
#ifdef _DEBUG
        // 落ちる直前にGPUが何を実行していたかをログへ出す
        LogDREDInfo(device_->GetDevice());
#endif
        // デバイスの異常状態
        assert(false && "Remove Device");
    }
}

void GraphicsDevice::CloseCommandList() {
    command_->Close();
}

void GraphicsDevice::ExecuteCommand() {
    command_->Execute();
}

void GraphicsDevice::ResetCommandList() {
    command_->Reset();
}

void GraphicsDevice::WaitForGPU() {
    fence_->WaitForGPU(command_->GetQueue());
}

void GraphicsDevice::Present() {
    swapChain_->Present();
}
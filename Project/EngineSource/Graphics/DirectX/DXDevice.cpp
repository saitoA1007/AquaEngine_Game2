#include "pch.h"
#include"DXDevice.h"
#include<cassert>
#include"LogManager.h"
#include"ConvertString.h"

using namespace Microsoft::WRL;
using namespace GameEngine;

void DXDevice::Initialize() {

#ifdef _DEBUG
	// デバックレイヤーを生成
	CreateDebugLayer();
	// DREDを有効化する
	EnableDRED();
#endif

	// ファクトリーを生成
	CreateFactory();
	// デバイスを生成
	CreateDevice();
}

void DXDevice::CreateFactory() {

    LogManager::GetInstance().Log("Start　Create Factory");

    // DXGIファクトリーの生成
    // HRESULTはWindow系のエラーコードであり、
    // 関数が成功したかどうかをSUCCEEDEDマクロで判定出来る
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
    assert(SUCCEEDED(hr));

    LogManager::GetInstance().Log("End　Create Factory");
}

void DXDevice::CreateDevice() {

	LogManager::GetInstance().Log("Start　Create Device");

    HRESULT hr;

    // 使用するアダプタ用の変数。最初にnullptrを入れておく
    ComPtr<IDXGIAdapter4> useAdapter = nullptr;
    // 良い順にアダプタを頼む
    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i,
        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
        DXGI_ERROR_NOT_FOUND; ++i) {
        // アダプターの情報を取得する
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));   // 取得出来ないのが一大事
        // ソフトウェアアダプタでなければ採用
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            // 採用したアダプタの情報をログに出力。wstringの方なので注意
            LogManager::GetInstance().Log(ConvertString(std::format(L"Use Adapater:{}", adapterDesc.Description)));
            break;
        }
        useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
    }
    assert(useAdapter != nullptr);

    // 機能レベルとログを出力用の文字列
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,D3D_FEATURE_LEVEL_12_1,D3D_FEATURE_LEVEL_12_0
    };
    const char* featureLevelString[] = { "12.2","12.1","12.0" };
    // 高い順に生成できるかを試していく
    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        // 採用したアダプターでデバイスを生成
        hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
        // 指定した機能レベルでデバイスが生成できたかを確認
        if (SUCCEEDED(hr)) {
            // 生成できたログを出力を行ってループを抜ける
            LogManager::GetInstance().Log(std::format("FeatureLevel : {}", featureLevelString[i]));
            break;
        }
    }
    // デバイスの生成がうまくいかなかったので軌道できない
    assert(device_ != nullptr);

    // レイトレーシングが使用出来るか確認
    CheckRaytracingEnable();

    LogManager::GetInstance().Log("End　Create Device");
}

void DXDevice::CheckRaytracingEnable() {
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 option = {};
    HRESULT hr = device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &option, sizeof(option));

    if (FAILED(hr) || option.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
        assert(false && "dont support raytracing");
    }

    LogManager::GetInstance().Log("Raytracing is supported");
}

#ifdef _DEBUG
void DXDevice::CreateDebugLayer() {
	ComPtr<ID3D12Debug1> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
}

void DXDevice::EnableDRED() {
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> dredSettings;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings)))) {
		// 実行したコマンドの履歴と、ページフォルト時のリソース情報を記録させる
		dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		LogManager::GetInstance().Log("DRED enabled");
	} else {
		LogManager::GetInstance().Log("DRED is not available");
	}
}
#endif

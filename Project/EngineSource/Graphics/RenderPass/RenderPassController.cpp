#include "pch.h"
#include "RenderPassController.h"
#include <cassert>
using namespace GameEngine;

void RenderPassController::Initialize(RenderTextureManager* renderTextureManager, ID3D12GraphicsCommandList* commandList) {
	commandList_ = commandList;
	// レンダーを取得する
	renderTextureManager_ = renderTextureManager;
}

void RenderPassController::AddPass(const std::string& name, RenderTextureMode mode, uint32_t wid, uint32_t hei, Vector4 clearColor, DXGI_FORMAT colorFormat) {
	// すでに登録されている場合、早期リターン
	auto getName = renderPassList_.find(name);
	if (getName != renderPassList_.end()) {
		return;
	}

	// renderTextureを作成
	renderTextureManager_->Create(name, wid, hei,mode, colorFormat, clearColor);
	RenderTexture* renderTex = renderTextureManager_->GetRenderTexture(name);

	// レンダーパスを作成
	std::unique_ptr<RenderPass> tmp = std::make_unique<RenderPass>(name, commandList_, renderTex, clearColor);

	// 登録
	renderPassList_[name] = std::move(tmp);
}

void RenderPassController::PrePass(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		std::string errorStr = "Not found RenderPass : name[" + name + "]";
		assert(false && errorStr.c_str());
	}

	// PIXイベント開始
	BeginPixEvent(name);

	// 描画前処理
	render->second->PrePass();
	render->second->SetRenderTarget();
}

void RenderPassController::PrePass(std::vector<std::string> names, const std::string dsvName) {

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderHandleList;
	renderHandleList.resize(names.size());

	for (uint32_t i = 0; i < names.size(); ++i) {

		// 登録されていなければエラー
		auto render = renderPassList_.find(names[i]);
		if (render == renderPassList_.end()) {
			std::string errorStr = "Not found RenderPass : name[" + names[i] + "]";
			assert(false && errorStr.c_str());
		}

		// PIXイベント開始
		BeginPixEvent(names[i]);

		// 描画前処理
		render->second->PrePass();
		// ハンドルを取得
		renderHandleList[i] = render->second->GetRtvHandle();
	}

	// 参照する深度があれば使用
	if (dsvName != "") {
		// 登録されていなければエラー
		auto render = renderPassList_.find(dsvName);
		if (render == renderPassList_.end()) {
			std::string errorStr = "Not found RenderPass : name[" + dsvName + "]";
			assert(false && errorStr.c_str());
		}

		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
		dsvHandle = render->second->GetDsvHandle();

		// マルチレンダーターゲットを設定
		commandList_->OMSetRenderTargets(renderHandleList.size(), renderHandleList.data(), false, &dsvHandle);
	} else {
		// マルチレンダーターゲットを設定
		commandList_->OMSetRenderTargets(renderHandleList.size(), renderHandleList.data(), false, nullptr);
	}
}

void RenderPassController::PostPass(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	// 描画後処理
	render->second->PostPass();

	// PIXイベント終了
	EndPixEvent(name);
}

//==============================================================================
// PIXイベント制御
//==============================================================================

void RenderPassController::BeginPixEvent(const std::string& name) {
	// 同じパスが二重に開かれるのを防ぐ
	for (const auto& openName : openPixEvents_) {
		if (openName == name) { return; }
	}

	PixBeginEvent(commandList_, PixColor::Pass, name.c_str());
	openPixEvents_.push_back(name);
}

void RenderPassController::EndPixEvent(const std::string& name) {
	// スタック内から対象を探す
	int32_t target = -1;
	for (int32_t i = static_cast<int32_t>(openPixEvents_.size()) - 1; i >= 0; --i) {
		if (openPixEvents_[i] == name) {
			target = i;
			break;
		}
	}

	// 開かれていなければ何もしない
	if (target < 0) { return; }

	// PIXイベントはスタック構造なので、対象より上に積まれたものから順に閉じる
	while (static_cast<int32_t>(openPixEvents_.size()) > target) {
		PixEndEvent(commandList_);
		openPixEvents_.pop_back();
	}
}

void RenderPassController::CloseAllPixEvents() {
	while (!openPixEvents_.empty()) {
		PixEndEvent(commandList_);
		openPixEvents_.pop_back();
	}
}

void RenderPassController::SwitchToUAV(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	render->second->SwitchToUnorderedAccess();
}

void RenderPassController::InsertUavBarrier(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	render->second->InsertUavBarrier();
}

void RenderPassController::SetOnlyDsvRenderTarget(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	render->second->SetOnlyDsvRenderTarget();
}

void RenderPassController::ClearRenderPass(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	render->second->ClearRenderPass();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE RenderPassController::GetSrvHandle(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	return render->second->GetSrvHandle();
}

uint32_t RenderPassController::GetSrvIndex(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	return render->second->GetSrvIndex();
}

uint32_t RenderPassController::GetUavIndex(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	return render->second->GetUavIndex();
}

uint32_t RenderPassController::GetDepthSrvIndex(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	return render->second->GetDepthSrvIndex();
}

void RenderPassController::SetSceneFinalPass(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	sceneFinalPassName_ = name;
	sceneFinalPassSrvHandle_ = render->second->GetSrvHandle();
}

void RenderPassController::SetPostProcessFinalPass(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	postProcessFinalPassName_ = name;
	postProcessFinalPassSrvHandle_ = render->second->GetSrvHandle();
}

void RenderPassController::SetPresentPass(const std::string& name) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	presentPassName_ = name;
	presentPassSrvHandle_ = render->second->GetSrvHandle();
}

void RenderPassController::SetDrawRange(const std::string& name, const uint32_t& width, const uint32_t& height, const uint32_t& left, const uint32_t& top) {
	// 登録されていなければエラー
	auto render = renderPassList_.find(name);
	if (render == renderPassList_.end()) {
		assert(false && "Not found RenderPass");
	}

	render->second->SetDrawRange(width, height, left, top);
}
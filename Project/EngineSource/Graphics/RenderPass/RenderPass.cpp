#include "pch.h"
#include "RenderPass.h"
using namespace GameEngine;

RenderPass::RenderPass(const std::string& name, ID3D12GraphicsCommandList* commandList, RenderTexture* renderTexture, Vector4 clearColor) {
	name_ = name;
	renderTexture_ = renderTexture;
	commandList_ = commandList;
	clearColor_[0] = clearColor.x;
	clearColor_[1] = clearColor.y;
	clearColor_[2] = clearColor.z;
	clearColor_[3] = clearColor.w;

	// 画面サイズを取得
	uint32_t width = renderTexture_->GetWidth();
	uint32_t height = renderTexture_->GetHeight();

	// Viewportを作成
	viewport_.Width = static_cast<FLOAT>(width);
	viewport_.Height = static_cast<FLOAT>(height);
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
	// Scissorを作成
	scissorRect_.left = 0;
	scissorRect_.right = static_cast<LONG>(viewport_.Width);
	scissorRect_.top = 0;
	scissorRect_.bottom = static_cast<LONG>(viewport_.Height);

	// モードを取得する
	mode_ = renderTexture_->GetMode();
}

void RenderPass::PrePass() {
	switch (mode_)
	{
	case GameEngine::RenderTextureMode::RtvOnly: {
		// 書き込み状態に遷移
		renderTexture_->TransitionToRenderTarget(commandList_);
		break;
	}

	case GameEngine::RenderTextureMode::DsvOnly: {
		// 書き込み状態に遷移
		renderTexture_->TransitionToRenderTarget(commandList_);
		break;
	}

	case GameEngine::RenderTextureMode::RtvAndDsv: {
		// 書き込み状態に遷移
		renderTexture_->TransitionToRenderTarget(commandList_);
		break;
	}

	case RenderTextureMode::UavOnly: {
		// UAV状態へ遷移
		renderTexture_->TransitionToUnorderedAccess(commandList_);
		break;
	}

	case RenderTextureMode::RtvAndUav: {
		// まずRTV状態で描画パスを開始する
		renderTexture_->TransitionToRenderTarget(commandList_);
		break;
	}
	}
}

void RenderPass::SetRenderTarget() {
	switch (mode_)
	{
	case GameEngine::RenderTextureMode::RtvOnly: {
		// RTVのみセットする
		commandList_->OMSetRenderTargets(1, &renderTexture_->GetRtvHandle(), false, nullptr);
		break;
	}

	case GameEngine::RenderTextureMode::DsvOnly: {
		// DSVのみセットする
		commandList_->OMSetRenderTargets(0, nullptr, false, &renderTexture_->GetDsvHandle());
		break;
	}

	case GameEngine::RenderTextureMode::RtvAndDsv: {
		// RTVとDSVをセットする
		commandList_->OMSetRenderTargets(1, &renderTexture_->GetRtvHandle(), false, &renderTexture_->GetDsvHandle());
		break;
	}

	case RenderTextureMode::RtvAndUav: {
		// RTVのみセットする
		commandList_->OMSetRenderTargets(1, &renderTexture_->GetRtvHandle(), false, nullptr);
		break;
	}
	}

	if (mode_ != RenderTextureMode::UavOnly) {
		// Viewportを設定
		commandList_->RSSetViewports(1, &viewport_);
		// Scirssorを設定
		commandList_->RSSetScissorRects(1, &scissorRect_);
	}
}

void RenderPass::PostPass() {
	// 読み込み状態に遷移
	renderTexture_->TransitionToShaderResource(commandList_);
}

void RenderPass::SwitchToUnorderedAccess() {
	commandList_->OMSetRenderTargets(0, nullptr, false, nullptr);
	renderTexture_->TransitionToUnorderedAccess(commandList_);
}

void RenderPass::InsertUavBarrier() {
	renderTexture_->InsertUavBarrier(commandList_);
}

void RenderPass::SetOnlyDsvRenderTarget() {
	// DSVのみをセット
	commandList_->OMSetRenderTargets(0, nullptr, false, &renderTexture_->GetDsvHandle());
}

void RenderPass::ClearRenderPass() {
	switch (mode_)
	{
	case GameEngine::RenderTextureMode::RtvOnly: {
		// 指定した色で画面全体をクリアする
		commandList_->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor_, 0, nullptr);
		break;
	}

	case GameEngine::RenderTextureMode::DsvOnly: {
		// 深度クリア
		commandList_->ClearDepthStencilView(renderTexture_->GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		break;
	}

	case GameEngine::RenderTextureMode::RtvAndDsv: {
		// 指定した色で画面全体をクリアする
		commandList_->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor_, 0, nullptr);
		// 指定した深度で画面全体をクリアする
		commandList_->ClearDepthStencilView(renderTexture_->GetDsvHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		break;
	}
												 
	case RenderTextureMode::RtvAndUav: {
		// 指定した色で画面全体をクリアする
		commandList_->ClearRenderTargetView(renderTexture_->GetRtvHandle(), clearColor_, 0, nullptr);
		break;
	}
	}
}

CD3DX12_GPU_DESCRIPTOR_HANDLE RenderPass::GetSrvHandle() {
	return renderTexture_->GetSrvGpuHandle();
}

void RenderPass::SetDrawRange(const uint32_t& width, const uint32_t& height,const uint32_t& left,const uint32_t& top) {
	// Viewportを作成
	viewport_.Width = static_cast<FLOAT>(width);
	viewport_.Height = static_cast<FLOAT>(height);
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
	// Scissorを作成
	scissorRect_.left = left;
	scissorRect_.right = static_cast<LONG>(viewport_.Width);
	scissorRect_.top = top;
	scissorRect_.bottom = static_cast<LONG>(viewport_.Height);
}
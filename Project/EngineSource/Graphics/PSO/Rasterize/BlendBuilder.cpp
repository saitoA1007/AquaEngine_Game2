#include "pch.h"
#include "BlendBuilder.h"

using namespace GameEngine;

void BlendBuilder::Initialize() {

	for (uint32_t i = 0; i < BlendMode::kCountOfBlendMode; ++i) {
		// すべての色要素を書き込む
		blendDesc_[i].RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// ブレンドモードの有効化
		if (i != kBlendModeNone) {
			blendDesc_[i].RenderTarget[0].BlendEnable = TRUE; // ブレンドを有効化
			blendDesc_[i].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO; // アルファ値のソース
			blendDesc_[i].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファ値の加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE; // アルファ値のデスティネーション
		}

		switch (i) {

		case kBlendModeNormal:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // (1-SrcA)
			break;

		case kBlendModeAdd:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeSubtract:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeMultily:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR; // (1-SrcA)
			break;

		case kBlendModeScreen:
			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeNormalAndSaveObjectAlpha:
			blendDesc_[i].RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE; // アルファ値のソース
			blendDesc_[i].RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファ値の加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; // アルファ値のデスティネーション

			blendDesc_[i].RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc_[i].RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc_[i].RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // (1-SrcA)
			break;

		case kNoBlend:
			// カラー書き込みをしない
			blendDesc_[i].RenderTarget[0].RenderTargetWriteMask = 0;
			break;
		}
	}
}

D3D12_BLEND_DESC BlendBuilder::GetBlendDesc(BlendMode blendMode) {
	return blendDesc_[blendMode];
}

D3D12_BLEND_DESC BlendBuilder::CreateBlendDesc(std::vector<BlendMode> blendModes) {
	D3D12_BLEND_DESC blendDesc{};

	if (blendModes.size() > 1) {
		blendDesc.IndependentBlendEnable = true;
		blendDesc.AlphaToCoverageEnable = false;
	}

	for (uint32_t i = 0; i < blendModes.size(); ++i) {

		// すべての色要素を書き込む
		blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// ブレンドモードの有効化
		if (blendModes[i] != kBlendModeNone) {
			blendDesc.RenderTarget[i].BlendEnable = TRUE; // ブレンドを有効化
			blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ZERO; // アルファ値のソース
			blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファ値の加算ブレンド
			blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ONE; // アルファ値のデスティネーション
		}

		switch (blendModes[i]) {

		case kBlendModeNormal:
			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // (1-SrcA)
			break;

		case kBlendModeAdd:
			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeSubtract:
			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; // 加算ブレンド
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeMultily:
			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_SRC_COLOR; // (1-SrcA)
			break;

		case kBlendModeScreen:
			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_INV_DEST_COLOR; // SrcA
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeNormalAndSaveObjectAlpha:
			blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE; // アルファ値のソース
			blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファ値の加算ブレンド
			blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; // アルファ値のデスティネーション

			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_ALPHA; // (1-SrcA)
			break;

		case kBlendModeAddAndSaveObjectAlpha:
			blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE; // アルファ値のソース
			blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD; // アルファ値の加算ブレンド
			blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA; // アルファ値のデスティネーション

			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_SRC_ALPHA; // SrcA
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD; // 加算ブレンド
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ONE; // (1-SrcA)
			break;

		case kBlendModeWboitAccumulation:
			blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_ONE;

			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_ONE;
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_ONE;
			break;

		case kBlendModeWboitRevealage:
			blendDesc.RenderTarget[i].SrcBlendAlpha = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[i].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[i].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;

			blendDesc.RenderTarget[i].SrcBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[i].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[i].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
			break;

		case kNoBlend:
			// カラー書き込みをしない
			blendDesc.RenderTarget[i].RenderTargetWriteMask = 0;
			break;
		}

	}

	return blendDesc;
}
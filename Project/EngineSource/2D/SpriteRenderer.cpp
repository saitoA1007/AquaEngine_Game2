#include "pch.h"
#include "SpriteRenderer.h"
#include <cassert>

using namespace GameEngine;

ID3D12GraphicsCommandList* SpriteRenderer::commandList_ = nullptr;
SrvManager* SpriteRenderer::srvManager_ = nullptr;

void SpriteRenderer::StaticInitialize(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) {
	commandList_ = commandList;
	srvManager_ = srvManager;
}

void SpriteRenderer::Draw(const Sprite* sprite) {
	// Spriteの描画。
	commandList_->IASetVertexBuffers(0, 1, &sprite->GetVertexBufferView());
	commandList_->IASetIndexBuffer(&sprite->GetIndexBufferView());// IBVを設定
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->SetGraphicsRootConstantBufferView(0, sprite->GetResource()->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(1, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
	// 描画
	commandList_->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void SpriteRenderer::Draw(const Text* text) {
	if (text->GetGlyphCount() == 0) { return; }

	// 頂点はVSでSV_VertexIDから生成するので頂点バッファは使わない
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList_->SetGraphicsRootConstantBufferView(0, text->GetResource()->GetGPUVirtualAddress());
	commandList_->SetGraphicsRootDescriptorTable(1, text->GetGlyphSrvHandle());
	commandList_->SetGraphicsRootDescriptorTable(2, srvManager_->GetSRVHeap()->GetGPUDescriptorHandleForHeapStart());
	// 描画
	commandList_->DrawInstanced(6, text->GetGlyphCount(), 0, 0);
}
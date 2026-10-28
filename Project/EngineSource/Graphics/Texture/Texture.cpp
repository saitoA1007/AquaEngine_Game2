#include "pch.h"
#include "Texture.h"
#include "CreateBufferResource.h"
#include "TextureLoader.h"
#include "LogManager.h"
#include "ResourceGarbageCollector.h"
using namespace GameEngine;

Texture::~Texture() {
	resource_.Reset();
	// srvの解放
	if (srvManager_) {
		srvManager_->ReleaseIndex(srvIndex_);
	}
}

void Texture::Create(const std::string& filePath, ID3D12GraphicsCommandList* cmdList, bool isSRGB, bool useCache) {
	// テクスチャ名を記録
	fileName_ = filePath;

	// テクスチャーの読み込みを開始するログ
	LogManager::GetInstance().Log("Start LoadTexture : " + fileName_);

	/// ファイルから画像データを読み込む

	// テクスチャを読み込む
	if (useCache) {
		mipImage_ = TextureLoader::LoadFromFileCached(fileName_, isSRGB);
	} else {
		mipImage_ = TextureLoader::LoadFromFile(fileName_, isSRGB);
	}
	if (!mipImage_.GetImages()) {
		LogManager::GetInstance().Log("Failed to load texture: " + fileName_);
		assert(false);
	}
	const DirectX::TexMetadata* metadata = &mipImage_.GetMetadata();

	/// GPUにテクスチャリソースを作成

	// テクスチャリソースを作成
	resource_ = TextureLoader::CreateTextureResource(device_, *metadata);
	if (!resource_) {
		LogManager::GetInstance().Log("Failed to create textureResource for: " + fileName_);
		assert(false);
	}

	/// CPUからGPUへデータを送る

	// テクスチャデータをアップロード
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResources = TextureLoader::UploadTextureData(device_, cmdList, resource_.Get(), mipImage_);
	if (!intermediateResources) {
		LogManager::GetInstance().Log("Failed to upload texture data for: " + fileName_);
		assert(false);
	}

	// リソースの破棄を登録する
	ResourceGarbageCollector::GetInstance().Add(intermediateResources);

	/// SRVを作成

	// srvインデックスを取得
	uint32_t index = srvManager_->AllocateSrvIndex(SrvHeapType::Texture);
	srvIndex_ = index;

	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata->format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (metadata->IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;// 2Dテクスチャ
		srvDesc.Texture2D.MipLevels = UINT(metadata->mipLevels);
	}

	// SRVを作成するDescriptorHeapの場所を決める。
	srvHandleCPU_ = srvManager_->GetCPUHandle(index);
	srvHandleGPU_ = srvManager_->GetGPUHandle(index);
	// SRVを作成
	device_->CreateShaderResourceView(resource_.Get(), &srvDesc, srvHandleCPU_);

	// テクスチャーの読み込みを完了するログ
	LogManager::GetInstance().Log("End LoadTexture : " + fileName_ + "\n");
}
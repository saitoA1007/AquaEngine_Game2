#include "pch.h"
#include "TextureLoader.h"
#include "ConvertString.h"
#include "CreateBufferResource.h"
using namespace GameEngine;

namespace {

	DXGI_FORMAT SelectCompressFormat(const DirectX::TexMetadata& meta, bool isSRGB) {
		return isSRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
	}
}

[[nodiscard]]
DirectX::ScratchImage TextureLoader::LoadFromFile(const std::string& filePath, bool isSRGB) {
	// テクスチャファイルを読み込んでプログラムを扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr;
	if (filePathW.ends_with(L".dds")) {
		// .ddsの場合
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		// データテクスチャはガンマ補正をかけずにそのまま読み込む
		DirectX::WIC_FLAGS wicFlags = isSRGB ? DirectX::WIC_FLAGS_FORCE_SRGB : DirectX::WIC_FLAGS_IGNORE_SRGB;
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), wicFlags, nullptr, image);
	}
	assert(SUCCEEDED(hr));

	// ミップマップの作成
	DirectX::ScratchImage mipImages{};

	if (DirectX::IsCompressed(image.GetMetadata().format) || image.GetMetadata().mipLevels > 1) {
		// 圧縮フォーマットならそのまま使用する
		mipImages = std::move(image);
	} else {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), isSRGB ? DirectX::TEX_FILTER_SRGB : DirectX::TEX_FILTER_DEFAULT, 0, mipImages);
	}
	assert(SUCCEEDED(hr));

	// ミップマップ付きのデータを返す
	return mipImages;
}

DirectX::ScratchImage TextureLoader::LoadFromFileCached(const std::string& filePath, bool isSRGB) {
	namespace fs = std::filesystem;
	fs::path src(filePath);

	// 既にddsならそのまま
	if (src.extension() == ".dds") {
		return LoadFromFile(filePath, isSRGB);
	}

	// キャッシュ先を設定Resources/Textures/Cache/xxx.dds
	fs::path cacheDir = src.parent_path() / "Cache";
	// sRGB有無で名前を分ける
	fs::path cachePath = cacheDir / (src.stem().string() + (isSRGB ? "_srgb" : "_lin") + ".dds");

	// キャッシュが存在し、元画像より新しければDDSを読む
	if (fs::exists(cachePath) &&
		fs::last_write_time(cachePath) >= fs::last_write_time(src)) {
		return LoadFromFile(cachePath.string(), isSRGB);
	}

	// 変換処理
	DirectX::ScratchImage image{};
	HRESULT hr = DirectX::LoadFromWICFile(
		ConvertString(filePath).c_str(),
		isSRGB ? DirectX::WIC_FLAGS_FORCE_SRGB : DirectX::WIC_FLAGS_IGNORE_SRGB,
		nullptr, image);
	if (FAILED(hr)) { 
		assert(false);
		return {}; 
	}

	// ミップ生成
	DirectX::ScratchImage mip{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(),
		isSRGB ? DirectX::TEX_FILTER_SRGB : DirectX::TEX_FILTER_DEFAULT, 0, mip);
	if (FAILED(hr)) {
		assert(false);
		return {}; 
	}

	// BC圧縮
	DirectX::ScratchImage out{};
	const auto& m = mip.GetMetadata();
	if (m.width % 4 == 0 && m.height % 4 == 0) {
		hr = DirectX::Compress(mip.GetImages(), mip.GetImageCount(), m, SelectCompressFormat(m, isSRGB),
			DirectX::TEX_COMPRESS_PARALLEL, 1.0f, out);
		if (FAILED(hr)) {
			out = std::move(mip);
		}
	} else {
		out = std::move(mip);
	}

	// 保存
	fs::create_directories(cacheDir);
	DirectX::SaveToDDSFile(out.GetImages(), out.GetImageCount(), out.GetMetadata(), DirectX::DDS_FLAGS_NONE, cachePath.wstring().c_str());

	return out;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> TextureLoader::CreateTextureResource(ID3D12Device5* device,const DirectX::TexMetadata& metadata) {
	// metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width); // Textureの幅
	resourceDesc.Height = UINT(metadata.height); // Textureの高さ
	resourceDesc.MipLevels = UINT16(metadata.mipLevels); // mipmapの数
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行 or 配列Textureの配列数 
	resourceDesc.Format = metadata.format; // TextureのFormat
	resourceDesc.SampleDesc.Count = 1; // ダンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // Textureの次元数。

	// 利用するHeapの設定。
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	// Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // Heapの設定
		D3D12_HEAP_FLAG_NONE, // Heapの特殊な設定。特になし。
		&resourceDesc, // Resourceの設定。
		D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
		nullptr, // Clear最適値。使わないのでnullptr
		IID_PPV_ARGS(&resource)); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	// デバック用に名前を設定
	resource->SetName(L"TextureResource_Debug");

	return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> TextureLoader::UploadTextureData(ID3D12Device5* device, ID3D12GraphicsCommandList* commandList, 
	ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
	uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device, intermediateSize);
	UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());
	// Textureへ転送後は利用出来るよう、D3D12_RESOURCE_STATE_COPY_DESTからD3D12_RESOURCE_STATE_GENERIC_READへResourceStateを変更する
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}
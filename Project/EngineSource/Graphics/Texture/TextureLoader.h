#pragma once
#include <d3d12.h>
#include <string>
#include "Externals/DirectXTex/DirectXTex.h"
#include "Externals/DirectXTex/d3dx12.h"

namespace GameEngine {

	/// <summary>
	/// テクスチャの作成をおこなう
	/// </summary>
	class TextureLoader {
	public:

		/// <summary>
		/// ファイルから画像を読み込んでミップマップを生成する
		/// </summary>
		/// <param name="filePath">ファイルパス</param>
		/// <param name="isSRGB">sRGBとして読み込むか</param>
		/// <returns>読み込んだ画像データ</returns>
		[[nodiscard]]
		static DirectX::ScratchImage LoadFromFile(const std::string& filePath, bool isSRGB = true);

		/// <summary>
		/// pngやjpgを読み込んでBC圧縮＋ミップ生成してddsとして保存、読み込みをおこなう
		/// </summary>
		/// <param name="filePath"></param>
		/// <param name="isSRGB"></param>
		/// <returns></returns>
		static DirectX::ScratchImage LoadFromFileCached(const std::string& filePath, bool isSRGB = true);

		/// <summary>
		/// メタデータにより、DirectX12のテクスチャリソースを作成する
		/// </summary>
		/// <param name="metadata">画像メタデータ</param>
		/// <returns>作成されたリソース</returns>
		[[nodiscard]]
		static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device5* device,const DirectX::TexMetadata& metadata);

		/// <summary>
		/// テクスチャデータをCPUメモリからGPUメモリに転送する
		/// </summary>
		/// <param name="texture">転送先のリソース</param>
		/// <param name="mipImages">転送元の画像データ</param>
		/// <returns>中間リソース</returns>
		[[nodiscard]]
		static Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Device5* device,ID3D12GraphicsCommandList* commandList,
			ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);
	};
}
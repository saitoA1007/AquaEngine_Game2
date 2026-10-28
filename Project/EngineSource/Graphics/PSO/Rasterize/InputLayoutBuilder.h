#pragma once
#include <d3d12.h>
#include<dxcapi.h>
#include<vector>
#include<string>
#include <wrl.h>

namespace GameEngine {

	class InputLayoutBuilder {
	public:

		/// <summary>
		/// 入力要素を登録する
		/// </summary>
		/// <param name="name"></param>
		/// <param name="index"></param>
		/// <param name="format"></param>
		void CreateInputElement(const std::string& name, uint32_t index, uint32_t slotIndex, DXGI_FORMAT format);

		/// <summary>
		/// 登録した入力要素を確定する
		/// </summary>
		void CreateInputLayoutDesc();

		/// <summary>
		/// リセットする
		/// </summary>
		void Reset();

		// 通常のオブジェクト用
		void CreateDefaultObjElement();
		// 通常の画像用
		void CreateDefaultSpriteElement();
		// 通常の線用
		void CreateDefaultLineElement();
		// 通常のアニメーション用
		void CreateDefaultAnimationElement();
		// グリッド描画用
		void CreateGridElement();
		// inputLayoutを使用しない
		void CreateNone();

		/// <summary>
		/// シェーダーリフレクションから入力レイアウトを自動生成する
		/// </summary>
		/// <param name="vsBlob"></param>
		void CreateInputLayoutFromReflection(IDxcUtils* utils, IDxcBlob* vsBlob);

		D3D12_INPUT_LAYOUT_DESC GetInputLayoutDesc() const { return inputLayoutDesc_; }

	private:

		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs_;
		std::vector<std::string> semanticNames_;
		D3D12_INPUT_LAYOUT_DESC inputLayoutDesc_{};

	private:

		/// <summary>
		/// 手動で指定する場合、保存していた名前を適応させる
		/// </summary>
		void SetSemanticName();

	};
}


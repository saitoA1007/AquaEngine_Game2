#pragma once
#include <iostream>
#include "Vector4.h"
#include "Vector2.h"
#include "Matrix4x4.h"
#include "Transform.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ConstantBuffer.h"
#include "WorldTransform.h"

namespace GameEngine {

	class Sprite final {
	public:

		// 頂点データ
		struct VertexPosUv {
			Vector4 position;
			Vector2 texcoord;
		};

		// 定数バッファ
		struct ConstBufferData {
			Vector4 color;
			Matrix4x4 uvTransform;
			Matrix4x4 WVP;
			uint32_t textureHandle;
		};

	public:
		Sprite(const Vector2& position = {0.0f,0.0f}, const Vector2& size = {64.0f,64.0f}, const Vector2& anchorPoint = {0.0f,0.0f}, const Vector4& color = {1, 1, 1, 1},
			const Vector2& leftTop = { 0.0f,0.0f }, const Vector2& textureSize = { 1.0f,1.0f }, const Vector2& textureMaxSize = { 1.0f,1.0f });
		~Sprite();

		/// <summary>
		/// 静的初期化
		/// </summary>
		/// <param name="device">デバイス</param>
		/// <param name="window_width">画面幅</param>
		/// <param name="window_height">画面高さ</param>
		static void StaticInitialize(int32_t width, int32_t height);

		// 2D描画用の射影行列を取得
		static const Matrix4x4& GetOrthoMatrix() { return orthoMatrix_; }

	public:

		/// <summary>
		/// 更新処理
		/// </summary>
		void Update();

		/// <summary>
		/// 座標の設定
		/// </summary>
		/// <param name="position">座標</param>
		void SetPosition(const Vector2& position);

		/// <summary>
		/// サイズの設定
		/// </summary>
		/// <param name="size">サイズ</param>
		void SetSize(const Vector2& size);

		/// <summary>
		/// 色の設定
		/// </summary>
		/// <param name="color">色</param>
		void SetColor(const Vector4& color);

		/// <summary>
		/// 透明度の設定
		/// </summary>
		/// <param name="alpha"></param>
		void SetAlpha(const float& alpha) { constBufferData_->color.w = alpha; }

		/// <summary>
		/// uvMatrixの設定
		/// </summary>
		/// <param name="transform"></param>
		void SetUvMatrix(const Transform& transform);

		const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBuffer_.GetView(); }
		const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBuffer_.GetView(); }

		ID3D12Resource* GetResource() const { return constBuffer_.GetResource(); }

		// 親を設定
		void SetParent(WorldTransform* parent) {
			parent_ = parent;
		}

	public: // 変数

		// 座標
		Vector2 position_{};
		// 回転
		float rotate_ = 0.0f;
		// スケール
		Vector2 scale_ = { 1.0f,1.0f };
		// スプライト幅、高さ
		Vector2 size_ = { 100.0f, 100.0f };

		// 色
		Vector4 color_;

		Vector2 textureLeftTop_ = { 0.0f,0.0f };
		Vector2 textureSize_ = { 100.0f,100.0f };
		Vector2 textureMaxeSize_ = {};

		// テクスチャハンドル
		uint32_t textureHandle_ = 0;

		// アンカーポイント
		Vector2 anchorPoint_{};

	private:

		// 射影行列
		static Matrix4x4 orthoMatrix_;

		// 親
		WorldTransform* parent_ = nullptr;
	
		// ワールド行列
		Matrix4x4 worldMatrix_;

		VertexBuffer<VertexPosUv> vertexBuffer_;
		IndexBuffer indexBuffer_;
		VertexPosUv* vertexData_ = nullptr;

		ConstantBuffer<ConstBufferData> constBuffer_;
		// マテリアルにデータを書き込む
		ConstBufferData* constBufferData_ = nullptr;

	private:

		/// <summary>
		/// メッシュを作成
		/// </summary>
		void CreateMesh();

		/// <summary>
		/// マテリアルを作成
		/// </summary>
		/// <param name="color"></param>
		void CreateConstBufferData(const Vector4& color);
	};
}

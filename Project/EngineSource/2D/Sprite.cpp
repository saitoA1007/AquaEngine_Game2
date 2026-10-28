#include "pch.h"
#include "Sprite.h"
#include "MyMath.h"
using namespace GameEngine;

Matrix4x4 Sprite::orthoMatrix_;

Sprite::Sprite(const Vector2& position, const Vector2& size, const Vector2& anchorPoint, const Vector4& color,
	const Vector2& leftTop, const Vector2& textureSize, const Vector2& textureMaxSize) {
	// 座標と大きさを取得
	position_ = position;
	size_ = size;
	anchorPoint_ = anchorPoint;
	// 座標を元にワールド行列の生成
	worldMatrix_ = Matrix4x4::MakeTranslateMatrix({ position.x,position.y,0.0f });

	// テクスチャのサイズを取得
	textureLeftTop_ = leftTop;
	textureSize_ = textureSize;
	textureMaxeSize_ = textureMaxSize;

	// メッシュを作成
	CreateMesh();

	// マテリアルを作成
	color_ = color;
	CreateConstBufferData(color);
}

Sprite::~Sprite() {

}

void Sprite::StaticInitialize(int32_t width, int32_t height) {
	orthoMatrix_ = Matrix4x4::MakeIdentity() * Matrix4x4::MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 100.0f);
}

void Sprite::Update() {

	// 位置の更新
	SetPosition(position_);

	// 大きさの更新
	SetSize(size_);

	constBufferData_->color = color_;
	constBufferData_->textureHandle = textureHandle_;

	// 座標を元にワールド行列の生成
	worldMatrix_ = Matrix4x4::MakeAffineMatrix(Vector3(scale_.x, scale_.y, 0.0f), Vector3(0.0f,0.0f,rotate_), Vector3(position_.x, position_.y, 0.0f));
	if (parent_) {
		worldMatrix_ *= parent_->GetWorldMatrix();
	}
	// 座標を適用 
	constBufferData_->WVP = worldMatrix_ * orthoMatrix_;
}

void Sprite::SetPosition(const Vector2& position) {
	position_ = position;
	// 座標を元にワールド行列の生成
	worldMatrix_ = Matrix4x4::MakeTranslateMatrix({ position.x,position.y,0.0f });
	if (parent_) {
		worldMatrix_ *= parent_->GetWorldMatrix();
	}
	// 座標を適用 
	constBufferData_->WVP = worldMatrix_ * orthoMatrix_;
}

void Sprite::SetSize(const Vector2& size) {
	size_ = size;
	// 画像のサイズを決める	
	float left = -anchorPoint_.x * size_.x;
	float right = (1.0f - anchorPoint_.x) * size_.x;
	float top = -anchorPoint_.y * size_.y;
	float bottom = (1.0f - anchorPoint_.y) * size_.y;

	// 頂点インデックス
	vertexData_[0].position = { left,bottom,0.0f,1.0f }; // 左下
	vertexData_[1].position = { left,top,0.0f,1.0f }; // 左上
	vertexData_[2].position = { right,bottom,0.0f,1.0f }; // 右下
	vertexData_[3].position = { right,top,0.0f,1.0f }; // 左上
}

void Sprite::SetColor(const Vector4& color) {
	color_ = color;
	// 色の設定
	constBufferData_->color = color;
}

void Sprite::SetUvMatrix(const Transform& transform) {
	// uv行列の設定
	constBufferData_->uvTransform = Matrix4x4::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
}

void Sprite::CreateMesh() {

	std::vector<VertexPosUv> vertices(4);

	// 画像のサイズを決める	
	float left = -anchorPoint_.x * size_.x;
	float right = (1.0f - anchorPoint_.x) * size_.x;
	float top = -anchorPoint_.y * size_.y;
	float bottom = (1.0f - anchorPoint_.y) * size_.y;

	// 頂点インデックス
	vertices[0].position = { left,  bottom, 0.0f, 1.0f }; // 左下
	vertices[1].position = { left,  top,    0.0f, 1.0f }; // 左上
	vertices[2].position = { right, bottom, 0.0f, 1.0f }; // 右下
	vertices[3].position = { right, top,    0.0f, 1.0f }; // 右上

	// uv座標指定
	float leftTex = textureLeftTop_.x / textureMaxeSize_.x;
	float rightTex = (textureLeftTop_.x + textureSize_.x) / textureMaxeSize_.x;
	float topTex = textureLeftTop_.y / textureMaxeSize_.y;
	float bottomTex = (textureLeftTop_.y + textureSize_.y) / textureMaxeSize_.y;

	vertices[0].texcoord = { leftTex,bottomTex };
	vertices[1].texcoord = { leftTex,topTex };
	vertices[2].texcoord = { rightTex,bottomTex };
	vertices[3].texcoord = { rightTex,topTex };

	vertexBuffer_.Create(vertices);

	// データを取得
	vertexData_ = vertexBuffer_.GetVertexData();

	// インデックスバッファを作成
	std::vector<uint32_t> indices = { 0, 1, 2, 1, 3, 2 };
	indexBuffer_.Create(indices);
}

void Sprite::CreateConstBufferData(const Vector4& color) {
	// 定数バッファの作成
	constBuffer_.Create();
	constBufferData_ = constBuffer_.GetData();
	// 色の設定
	constBufferData_->color = color;
	// UVTransform行列を初期化
	constBufferData_->uvTransform = Matrix4x4::MakeIdentity();
	// wvp行列を初期化
	constBufferData_->WVP = worldMatrix_ * orthoMatrix_;
	// テクスチャ
	constBufferData_->textureHandle = 0;
}
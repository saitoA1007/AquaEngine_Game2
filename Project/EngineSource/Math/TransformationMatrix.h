#pragma once
#include"Matrix4x4.h"
#include"Vector4.h"
#include"Vector3.h"
#include<cstdint>

struct alignas(16) TransformationMatrix {
	Matrix4x4 World;
	Matrix4x4 worldInverseTranspose;
};

struct alignas(16) ParticleForGPU {
	Matrix4x4 World;
	Vector4 color;
	uint32_t textureHandle;
	float padding[3];
};

struct alignas(16) CameraForGPU {
	Vector3 worldPosition;
	float pad;
	Matrix4x4 vpMatrix;
	Matrix4x4 mtxViewInv; // ビュー逆行列
	Matrix4x4 mtxProjInv; // プロジェクション逆行列
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
};
#pragma once
#include"Vector3.h"

struct Plane {
	Vector3 normal; // 法線
	float distance; // 距離
};

// 直線(無限に続く)
struct Line {
	Vector3 origin; // 始点
	Vector3 diff; // 終点への差分ベクトル
};

// 半直線(始点から無限に続く)
struct Ray {
	Vector3 origin; // 始点
	Vector3 diff; // 終点への差分ベクトル
};

// 線分(始点から終点まで)
struct Segment {
	Vector3 origin; // 始点
	Vector3 diff; // 終点への差分ベクトル
};

struct Triangle {
	Vector3 vertices[3]; // 頂点
};

struct Sphere {
	Vector3 center; // 中心
	float radius;   // 半径
};

struct AABB {
	Vector3 min; // 最小点
	Vector3 max; // 最大点
};

struct OBB {
	Vector3 center; // 中心点
	Vector3 orientations[3]; // 座標軸
	Vector3 size; // 座標軸方向の長さの半分。中心から面までの距離
};

struct Box {
	Vector3 vertices[8]; // 頂点
};
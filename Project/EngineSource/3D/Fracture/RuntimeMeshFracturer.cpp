#include "pch.h"
#define NOMINMAX
#include <algorithm>
#include "RuntimeMeshFracturer.h"
#include "Geometry.h"
#include "MyMath.h"
#include "RandomGenerator.h"
using namespace GameEngine;

namespace {

	// 三角形追加
	void AddTriangle(std::vector<VertexData>& verts, std::vector<uint32_t>& indices, const VertexData v[3]) {
		uint32_t base = static_cast<uint32_t>(verts.size());
		verts.push_back(v[0]);
		verts.push_back(v[1]);
		verts.push_back(v[2]);
		indices.push_back(base);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
	}

	// 平面をまたぐ三角形の分割
	void SplitStraddlingTriangle(const VertexData v[3], const float d[3],
		const Vector3& planeNormal, float planeDist,
		ClipResult& result, std::vector<std::pair<VertexData, VertexData>>& cutEdges) {

		(void)planeDist;

		auto Lerp = [](const VertexData& a, const VertexData& b, float t) {
			VertexData r;
			r.position = a.position + (b.position - a.position) * t;
			r.normal = Vector3::Normalize(a.normal + (b.normal - a.normal) * t);
			r.texcoord = a.texcoord + (b.texcoord - a.texcoord) * t;
			return r;
			};

		// 3頂点のうち他の2つと逆側にいる1頂点を探す
		int loneIndex = -1;
		bool loneIsFront = false;
		for (int i = 0; i < 3; ++i) {
			int o1 = (i + 1) % 3, o2 = (i + 2) % 3;
			bool iFront = d[i] >= 0.0f;
			bool o1Front = d[o1] >= 0.0f;
			bool o2Front = d[o2] >= 0.0f;
			if (iFront != o1Front && iFront != o2Front) {
				loneIndex = i;
				loneIsFront = iFront;
				break;
			}
		}
		if (loneIndex < 0) { return; }

		int a = loneIndex, b = (loneIndex + 1) % 3, c = (loneIndex + 2) % 3;

		float tAB = d[a] / (d[a] - d[b]);
		float tAC = d[a] / (d[a] - d[c]);
		VertexData pAB = Lerp(v[a], v[b], tAB);
		VertexData pAC = Lerp(v[a], v[c], tAC);

		VertexData loneTri[3] = { v[a], pAB, pAC };
		VertexData quad1[3] = { pAB, v[b], v[c] };
		VertexData quad2[3] = { pAB, v[c], pAC };

		if (loneIsFront) {
			AddTriangle(result.frontVerts, result.frontIndices, loneTri);
			AddTriangle(result.backVerts, result.backIndices, quad1);
			AddTriangle(result.backVerts, result.backIndices, quad2);
			cutEdges.push_back({ pAB, pAC });
		} else {
			AddTriangle(result.backVerts, result.backIndices, loneTri);
			AddTriangle(result.frontVerts, result.frontIndices, quad1);
			AddTriangle(result.frontVerts, result.frontIndices, quad2);
			cutEdges.push_back({ pAC, pAB });
		}
	}

	// 切断エッジを繋いでキャップ面を生成
	void CapCutFace(const std::vector<std::pair<VertexData, VertexData>>& cutEdges,
		const Vector3& planeNormal, ClipResult& result) {

		if (cutEdges.empty()) { return; }

		std::vector<std::pair<VertexData, VertexData>> remaining(cutEdges.begin(), cutEdges.end());
		std::vector<VertexData> loop;

		loop.push_back(remaining.front().first);
		VertexData current = remaining.front().second;
		remaining.erase(remaining.begin());

		constexpr float kEps = 1e-4f;
		while (!remaining.empty()) {
			loop.push_back(current);
			bool found = false;
			for (size_t i = 0; i < remaining.size(); ++i) {
				if (Vector4::Distance(remaining[i].first.position, current.position) < kEps) {

					current = remaining[i].second;
					remaining.erase(remaining.begin() + i);
					found = true;
					break;
				}
				if (Vector4::Distance(remaining[i].second.position, current.position) < kEps) {
					current = remaining[i].first;
					remaining.erase(remaining.begin() + i);
					found = true;
					break;
				}
			}
			if (!found) { break; }
		}
		if (loop.size() < 3) { return; }

		// front面
		for (size_t i = 1; i + 1 < loop.size(); ++i) {
			VertexData tri[3] = { loop[0], loop[i], loop[i + 1] };
			for (auto& vtx : tri) {
				vtx.normal = planeNormal;
			}
			AddTriangle(result.frontVerts, result.frontIndices, tri);
		}
		// back面
		for (size_t i = 1; i + 1 < loop.size(); ++i) {
			VertexData tri[3] = { loop[0], loop[i + 1], loop[i] };
			for (auto& vtx : tri) {
				vtx.normal = planeNormal * -1.0f;
			}
			AddTriangle(result.backVerts, result.backIndices, tri);
		}
	}

	// 平面によるメッシュクリッピング
	ClipResult ClipMeshByPlane(const std::vector<VertexData>& verts,
		const std::vector<uint32_t>& indices,
		const Vector3& planeNormal, float planeDist) {

		ClipResult result;
		// キャッピング用の切断エッジ
		std::vector<std::pair<VertexData, VertexData>> cutEdges;

		auto SignedDist = [&](const Vector3& p) {
			return planeNormal.Dot(p) - planeDist;
			};

		for (size_t i = 0; i < indices.size(); i += 3) {
			VertexData v[3] = { verts[indices[i]], verts[indices[i + 1]], verts[indices[i + 2]] };
			float d[3] = { SignedDist(Vector3(v[0].position.x, v[0].position.y, v[0].position.z)),
				SignedDist(Vector3(v[1].position.x, v[1].position.y, v[1].position.z)),
				SignedDist(Vector3(v[2].position.x, v[2].position.y, v[2].position.z)) };

			// 3頂点が全部同じ側であればそのまま片方に追加
			if (d[0] >= 0 && d[1] >= 0 && d[2] >= 0) {
				AddTriangle(result.frontVerts, result.frontIndices, v);
				continue;
			}

			if (d[0] < 0 && d[1] < 0 && d[2] < 0) {
				AddTriangle(result.backVerts, result.backIndices, v);
				continue;
			}

			// 平面をまたぐ三角形からエッジとの交点を計算し、front,backに三角形分割して振り分け
			SplitStraddlingTriangle(v, d, planeNormal, planeDist, result, cutEdges);
		}

		// 切断エッジ群をつないで多角形化から三角形ファンでキャップし、front,back両方に追加
		CapCutFace(cutEdges, planeNormal, result);

		return result;
	}

	AABB ComputeBounds(const std::vector<VertexData>& verts) {
		AABB aabb{};
		if (verts.empty()) { return aabb; }

		aabb.min = Vector3(verts[0].position.x, verts[0].position.y, verts[0].position.z);
		aabb.max = Vector3(verts[0].position.x, verts[0].position.y, verts[0].position.z);
		for (const auto& v : verts) {
			aabb.min.x = std::min(aabb.min.x, v.position.x);
			aabb.min.y = std::min(aabb.min.y, v.position.y);
			aabb.min.z = std::min(aabb.min.z, v.position.z);
			aabb.max.x = std::max(aabb.max.x, v.position.x);
			aabb.max.y = std::max(aabb.max.y, v.position.y);
			aabb.max.z = std::max(aabb.max.z, v.position.z);
		}
		return aabb;
	}

	std::vector<Vector3> GenerateVoronoiSites(const AABB& bounds, const Vector3& impactPos, int numSites) {
		std::vector<Vector3> sites;
		sites.reserve(numSites);

		Vector3 extent = bounds.max - bounds.min;
		float maxRadius = extent.Length() * 0.5f;

		for (int i = 0; i < numSites; ++i) {
			// tが0に近いほど衝撃点の近くに配置される
			float t = RandomGenerator::Get(0.0f, 1.0f);
			float radius = maxRadius * (t * t);

			Vector3 dir = RandomGenerator::GetVector3(-1.0f, 1.0f);
			dir.Normalize();

			Vector3 pos = impactPos + dir * radius;

			// バウンディングボックス外だとクリップが噛み合わずセルが消えやすいのでクランプ
			pos.x = std::clamp(pos.x, bounds.min.x, bounds.max.x);
			pos.y = std::clamp(pos.y, bounds.min.y, bounds.max.y);
			pos.z = std::clamp(pos.z, bounds.min.z, bounds.max.z);

			sites.push_back(pos);
		}
		return sites;
	}

	void VoronoiFracture(const std::vector<VertexData>& verts, const std::vector<uint32_t>& indices,
		const Vector3& impactPos, int numSites, std::vector<Fragment>& outFragments) {

		if (numSites <= 1) {
			outFragments.push_back({ verts, indices });
			return;
		}

		AABB bounds = ComputeBounds(verts);
		std::vector<Vector3> sites = GenerateVoronoiSites(bounds, impactPos, numSites);

		for (size_t i = 0; i < sites.size(); ++i) {
			std::vector<VertexData> cellVerts = verts;
			std::vector<uint32_t> cellIndices = indices;

			for (size_t j = 0; j < sites.size(); ++j) {
				if (i == j) { continue; }

				Vector3 normal = sites[j] - sites[i];
				float len = normal.Length();
				// ほぼ同位置のサイトは飛ばす
				if (len < 1e-5f) { continue; }
				normal = normal / len;

				Vector3 midpoint = (sites[i] + sites[j]) * 0.5f;
				float planeDist = normal.Dot(midpoint);

				ClipResult clipped = ClipMeshByPlane(cellVerts, cellIndices, normal, planeDist);
				cellVerts = clipped.backVerts;
				cellIndices = clipped.backIndices;

				// 他のサイトに完全に削られた
				if (cellIndices.empty()) {
					break;
				}
			}

			if (!cellIndices.empty()) {
				outFragments.push_back({ std::move(cellVerts), std::move(cellIndices) });
			}
		}
	}

	std::vector<Vector3> GenerateSpherePlaneNormals(int count) {
		std::vector<Vector3> normals;
		normals.reserve(count);

		if (count <= 1) {
			normals.push_back(Vector3(0.0f, 1.0f, 0.0f));
			return normals;
		}

		const float goldenAngle = 3.14159265f * (3.0f - std::sqrt(5.0f));
		for (int i = 0; i < count; ++i) {
			float y = 1.0f - (static_cast<float>(i) / static_cast<float>(count - 1)) * 2.0f;
			float radiusAtY = std::sqrt(std::max(0.0f, 1.0f - y * y));
			float theta = goldenAngle * static_cast<float>(i);

			float x = std::cos(theta) * radiusAtY;
			float z = std::sin(theta) * radiusAtY;
			normals.push_back(Vector3(x, y, z));
		}
		return normals;
	}

	// srcのメッシュをdstの末尾に、インデックスをオフセットしながら結合する
	void AppendMesh(std::vector<VertexData>& dstVerts, std::vector<uint32_t>& dstIndices,
		const std::vector<VertexData>& srcVerts, const std::vector<uint32_t>& srcIndices) {
		uint32_t base = static_cast<uint32_t>(dstVerts.size());
		dstVerts.insert(dstVerts.end(), srcVerts.begin(), srcVerts.end());
		dstIndices.reserve(dstIndices.size() + srcIndices.size());
		for (uint32_t idx : srcIndices) {
			dstIndices.push_back(idx + base);
		}
	}

	// メッシュから球状の領域を取り除く
	void CarveImpactCrater(std::vector<VertexData>& verts, std::vector<uint32_t>& indices,
		const Vector3& impactPos, float craterRadius, int planeCount) {

		std::vector<Vector3> normals = GenerateSpherePlaneNormals(planeCount);

		std::vector<VertexData> survivorVerts;
		std::vector<uint32_t> survivorIndices;

		std::vector<VertexData> remainderVerts = verts;
		std::vector<uint32_t> remainderIndices = indices;

		for (const Vector3& n : normals) {
			if (remainderIndices.empty()) { break; }

			// 球面上の接点を通り、外向き法線nを持つ平面
			Vector3 planePoint = impactPos + n * craterRadius;
			float planeDist = n.Dot(planePoint);

			ClipResult clipped = ClipMeshByPlane(remainderVerts, remainderIndices, n, planeDist);

			// 球の外側は確定で生き残る
			AppendMesh(survivorVerts, survivorIndices, clipped.frontVerts, clipped.frontIndices);

			// 球の内側にいるかもしれない部分だけを、次の平面でさらに絞り込む
			remainderVerts = std::move(clipped.backVerts);
			remainderIndices = std::move(clipped.backIndices);
		}

		verts = std::move(survivorVerts);
		indices = std::move(survivorIndices);
	}
}

std::vector<Fragment> GameEngine::RuntimeMeshFracturer::Fracture(
	const Fragment& source, const Vector3& impactPos,
	float craterRadius, int numSites, int planeCount,
	size_t minTriangleCount) {

	std::vector<VertexData> carvedVerts = source.vertices;
	std::vector<uint32_t> carvedIndices = source.indices;

	// 衝撃点周りを、衝突形状に合わせて凹ませる
	CarveImpactCrater(carvedVerts, carvedIndices, impactPos, craterRadius, planeCount);

	// クレーターで全部消えてしまった場合は元のメッシュにフォールバック
	if (carvedIndices.empty()) {
		carvedVerts = source.vertices;
		carvedIndices = source.indices;
	}

	int maxReasonableSites = std::max(2, static_cast<int>((carvedIndices.size() / 3) / minTriangleCount));
	int clampedSites = std::min(numSites, maxReasonableSites);

	std::vector<Fragment> fragments;
	VoronoiFracture(carvedVerts, carvedIndices, impactPos, clampedSites, fragments);
	return fragments;
}

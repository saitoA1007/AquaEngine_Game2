#pragma once
#include <unordered_map>
#include <vector>
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexData.h"

namespace GameEngine {

	/// <summary>
	/// ランタイムで分割した破片
	/// </summary>
    class RuntimeFractureBuffer {
    public:
        // 切断で生まれた断片群をアップロードし、GeometryRangeを返す
        std::vector<GeometryRange> Upload(const std::vector<Fragment>& fragments) {
            std::vector<VertexData> packedVerts;
            std::vector<uint32_t> packedIndices;
            std::vector<GeometryRange> ranges;

            for (auto& frag : fragments) {
                GeometryRange range;
                range.vertexOffset = static_cast<uint32_t>(packedVerts.size());
                range.indexOffset = static_cast<uint32_t>(packedIndices.size());

                packedVerts.insert(packedVerts.end(), frag.vertices.begin(), frag.vertices.end());
                // フラグメント内では0始まりのローカルインデックスからパック後のグローバルオフセットに変換
                for (uint32_t idx : frag.indices) {
                    packedIndices.push_back(idx + range.vertexOffset);
                }

                range.vertexCount = static_cast<uint32_t>(frag.vertices.size());
                range.indexCount = static_cast<uint32_t>(frag.indices.size());
                ranges.push_back(range);
            }

            vertexBuffer_.Create(packedVerts);
            indexBuffer_.Create(packedIndices);
            return ranges;
        }

        // 描画バインド用
        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return vertexBuffer_.GetView(); }
        const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return indexBuffer_.GetView(); }

    private:
        VertexBuffer<VertexData> vertexBuffer_;
        IndexBuffer indexBuffer_;
    };
}
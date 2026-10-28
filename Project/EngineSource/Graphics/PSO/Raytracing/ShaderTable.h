#pragma once
#define NOMINMAX
#include "ShaderRecord.h"
#include <vector>

namespace GameEngine {

    class ShaderTable {
    public:
        // レコードを追加する
        void AddRecord(ShaderRecord record) {
            maxRecordSize_ = (std::max)(maxRecordSize_, record.Size());
            records_.push_back(std::move(record));
        }

        // レコード数
        UINT RecordCount() const {
            return static_cast<UINT>(records_.size());
        }

        // 1レコードのサイズ（D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT に揃える）
        UINT RecordStride() const {
            return Align(static_cast<UINT>(maxRecordSize_),
                D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        }

        // テーブル全体のバイト数
        UINT TableSize() const {
            return RecordCount() * RecordStride();
        }

        // dst に全レコードを書き込む（stride 分の padding も含む）
        void WriteAll(uint8_t* dst) const {
            UINT stride = RecordStride();
            for (const auto& record : records_) {
                std::memcpy(dst, record.Data().data(), record.Size());
                // stride の余白はゼロ埋め
                if (record.Size() < stride) {
                    std::memset(dst + record.Size(), 0, stride - record.Size());
                }
                dst += stride;
            }
        }

    private:
        std::vector<ShaderRecord> records_;
        size_t maxRecordSize_ = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

    private:

        static UINT Align(UINT size, UINT alignment) {
            return (size + alignment - 1) & ~(alignment - 1);
        }
    };

}
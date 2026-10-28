#pragma once
#include <d3d12.h>
#include <vector>
#include <cstring>
#include <cassert>

namespace GameEngine {

    class ShaderRecord {
    public:
        // シェーダー識別子を設定する
        ShaderRecord& SetIdentifier(const void* shaderId) {
            data_.resize(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            std::memcpy(data_.data(), shaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            return *this;
        }

        // GPUディスクリプターハンドルを末尾に追記する
        void AppendDescriptor(D3D12_GPU_DESCRIPTOR_HANDLE handle) {
            AppendRaw(&handle, sizeof(handle));
        }

        // GPU仮想アドレスを末尾に追記する
        void AppendGPUAddress(D3D12_GPU_VIRTUAL_ADDRESS address) {
            AppendRaw(&address, sizeof(address));
        }

        // 定数などのデータを追記する
        template<class T>
        void AppendData(const T& value) {
            AppendRaw(&value, sizeof(T));
        }

        // レコードの生バイト列を取得する
        const std::vector<uint8_t>& Data() const { return data_; }
        size_t Size() const { return data_.size(); }

    private:
        std::vector<uint8_t> data_;

        void AppendRaw(const void* src, size_t size) {
            if (data_.size() < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES) {
                assert(false && "ShaderRecord: Call SetIdentifier() before appending data");
            }
            const auto* bytes = reinterpret_cast<const uint8_t*>(src);
            data_.insert(data_.end(), bytes, bytes + size);
        }
    };
}
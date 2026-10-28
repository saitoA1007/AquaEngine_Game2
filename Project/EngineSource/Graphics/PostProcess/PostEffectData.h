#pragma once
#include <cstdint>
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "IPostEffect.h"

namespace GameEngine {

    /// <summary>
    /// 色調補正、画面装飾の処理
    /// </summary>
    class ColorGrading : public IPostEffect {
    public:
        struct alignas(16) ColorGradingData {
            uint32_t textureHandle;
            float padding[3];

            uint32_t enableGrayscale;
            uint32_t enableSepia;
            uint32_t enableRandom;
            uint32_t enableVignetting;

            float vignettingIntensity; // ぼかさない円の範囲
            float vignettingTime; // ぼかしぐわい

            float randomIntensity;
            float randomTime;
        };
    public:
        ColorGrading();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

        void SetEnableGrayscale(bool isActive) {
            buffer_.GetData()->enableGrayscale = isActive;
        }

        // パラメータを操作
        void DrawParamUI() override;

        // パラメータの保存
        void SaveParams(nlohmann::json& j) override;
        // パラメータの読み込み
        void LoadParams(const nlohmann::json& j) override;

    private:
        ConstantBuffer<ColorGradingData> buffer_;
    };

    /// <summary>
    /// ラジアルブラー
    /// </summary>
    class RadialBlur : public IPostEffect {
    public:
        struct alignas(16) RadialBlurData {
            Vector2 centerPos; // 中心点
            int32_t numSamles; // サンプリング数。大きい程滑らか
            float blurWidth; // ぼかしの幅
            uint32_t textureHandle; // 加工する画像
            float padding[3];
        };
    public:
        RadialBlur();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

    private:
        ConstantBuffer<RadialBlurData> buffer_;
    };

    // 輝度マスク
    class HighLumMask : public IPostEffect {
        struct HighLumMaskData {
            uint32_t textureHandle;
            float highLumMask; // マスク範囲
            float padding[2];
        };

    public:
        HighLumMask();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

        // パラメータを操作
        void DrawParamUI() override;

        // パラメータの保存
        void SaveParams(nlohmann::json& j) override;
        // パラメータの読み込み
        void LoadParams(const nlohmann::json& j) override;

    private:
        ConstantBuffer<HighLumMaskData> buffer_;
    };

    // 縦のぼかし
    class GaussVertical : public IPostEffect {
        struct GaussianBlurData {
            uint32_t textureHandle;
            float sd; // 標準偏差
            float padding[2];
        };

    public:
        GaussVertical();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

        // パラメータを操作
        void DrawParamUI() override;

        // パラメータの保存
        void SaveParams(nlohmann::json& j) override;
        // パラメータの読み込み
        void LoadParams(const nlohmann::json& j) override;

    private:
        ConstantBuffer<GaussianBlurData> buffer_;
    };

    // 横のぼかし
    class GaussHorizontal : public IPostEffect {
        struct GaussianBlurData {
            uint32_t textureHandle;
            float sd; // 標準偏差
            float padding[2];
        };

    public:
        GaussHorizontal();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

        // パラメータを操作
        void DrawParamUI() override;

        // パラメータの保存
        void SaveParams(nlohmann::json& j) override;
        // パラメータの読み込み
        void LoadParams(const nlohmann::json& j) override;

    private:
        ConstantBuffer<GaussianBlurData> buffer_;
    };

    // ブルーム
    class Bloom : public IPostEffect {
        struct BloomData {
            uint32_t blurTextureHandle;
            uint32_t gameTextureHandle;
            float intensity;
            float pad;
        };

    public:
        Bloom();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->blurTextureHandle = index;
        }

        void SetGamePassIndex(uint32_t index) {
            buffer_.GetData()->gameTextureHandle = index;
        }

        uint32_t GetInputCount() const override { return 2; }
        const char* GetInputName(uint32_t slot) const override { return slot == 0 ? "Blur" : "Scene"; }
        void SetInputIndex(uint32_t slot, uint32_t srvIndex) override {
            if (slot == 0) { buffer_.GetData()->blurTextureHandle = srvIndex; } else { buffer_.GetData()->gameTextureHandle = srvIndex; }
        }
        const char* GetDisplayName() const override { return "Bloom"; }
        uint32_t GetPassThroughSlot() const override { return 1; }

        // パラメータを操作
        void DrawParamUI() override;

        // パラメータの保存
        void SaveParams(nlohmann::json& j) override;
        // パラメータの読み込み
        void LoadParams(const nlohmann::json& j) override;

    private:
        ConstantBuffer<BloomData> buffer_;
    };

    // ディゾルブ
    class Dissolve : public IPostEffect {
        struct DissolveData {
            uint32_t gameTextureHandle;
            uint32_t noiseTextureHandle;
            float threshold;
            float pad;
        };

    public:
        Dissolve();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->gameTextureHandle = index;
        }

        void SetNoiseTextureIndex(uint32_t index) {
            buffer_.GetData()->noiseTextureHandle = index;
        }

        void SetThreshold(float threshold) {
            buffer_.GetData()->threshold = threshold;
        }

        // パラメータを操作
        void DrawParamUI() override;

        // パラメータの保存
        void SaveParams(nlohmann::json& j) override;
        // パラメータの読み込み
        void LoadParams(const nlohmann::json& j) override;

    private:
        ConstantBuffer<DissolveData> buffer_;
    };

    // アウトライン
    class OutLine : public IPostEffect {
        struct OutLineData {
            uint32_t textureHandle;
            float sd; // 標準偏差
            float padding[2];
        };

    public:
        OutLine();

        void Draw(ID3D12GraphicsCommandList* commandList, SrvManager* srvManager) override;

        void SetPassIndex(const uint32_t& index) override {
            buffer_.GetData()->textureHandle = index;
        }

    private:
        ConstantBuffer<OutLineData> buffer_;
    };

}

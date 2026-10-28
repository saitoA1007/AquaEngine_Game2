#pragma once
#include "MaterialGraph.h"

namespace GameEngine {

    // 数式ノードの演算タイプ
    enum class MathOperation {
        kAdd,       // 加算
        kSubtract,  // 減算
        kMultiply,  // 乗算
        kDivide     // 除算
    };

    // 数学ノード
    class MathNode : public IMaterialNode {
    public:
        MathNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

        // ノードUIの描画
        void DrawNodeUI() override;

        bool IsVariableType() const override { return true; }
        void OnConnectTypePropagate(PinType newType) override;

    private:
        MathOperation operation_ = MathOperation::kAdd;
        // 現在の使用する型
        PinType valueType_ = PinType::kFloat4;
    };

    // 定数ノード
    class ConstantNode : public IMaterialNode {
    public:
        ConstantNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

        // ノードUI描画
        void DrawNodeUI() override;

    private:
        float value_ = 0.0f; // 保持する数値
    };

    // カラーノード
    class ColorNode : public IMaterialNode {
    public:
        ColorNode(MaterialGraph& g);

        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

        void DrawNodeUI() override;

    private:
        float color_[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    // テクスチャノード
    class TextureSampleNode : public IMaterialNode {
    public:
        TextureSampleNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

        void DrawNodeUI() override;

    private:
        // テクスチャ名
        std::string textureName;
        // テクスチャハンドル
        uint32_t textureHandle_ = 0;
        // 入力されたテクスチャ名
        std::string texFile_ = "";
        // 有効なテクスチャを選択済みか
        bool hasTexture_ = false; 

    private:

        void ResolveTexture();
        void SetTexture(const std::string& fileName);
    };

    // マスターサーフェスノード
    class PBROutputNode : public IMaterialNode {
    public:
        PBROutputNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override {
            // 出力ノードは値を収集するだけ
            pinVars;
            return "";
        }
    };

    // カメラ
    class CameraNode : public IMaterialNode {
    public:
        CameraNode(MaterialGraph& g);

        // HLSL生成
        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;
    };

    // ライトタイプ
    enum class LightType {
        kDirectional,
        kPoint,
        kSpot
    };

    // ライトノード
    class LightNode : public IMaterialNode {
    public:
        LightNode(MaterialGraph& g);

        std::string GenerateHLSL(const std::unordered_map<int, std::string>& pinVars) const override;

        void DrawNodeUI() override;

    private:
        LightType lightType_ = LightType::kDirectional;
    };

    // ヘルパー関数
    std::string GetPinVar(const std::unordered_map<int, std::string>& pinVars, int pinId, const std::string& defaultVal);
}
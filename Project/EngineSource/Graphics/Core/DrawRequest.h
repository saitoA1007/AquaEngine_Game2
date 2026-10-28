#pragma once
#include <string>

namespace GameEngine {

    class Model;
    class Sprite;
    class Text;
    class WorldTransform;
    class WorldTransforms;
    class GpuResource;
    class SrvResource;
    class DebugRenderer;
    class FractureInstance;

    // 描画レイヤー（実行優先順位。数値が小さいほど先に描画）
    enum class RenderLayer : uint32_t {
        Shadow = 0,
        Skybox = 10,
        Grid = 20,
        Opaque = 30,
        Animation = 40,
        Translucent = 100,
        Debug = 200,
        Sprite = 250,
    };

    // 3D描画のモード
    enum class Draw3dType {
        Default,        // 通常モデルを描画用
        DefaultAdd,
        Instancing,     // インスタンシング描画用
        InstancingAdd,
        InstancingWboit,
        ParticleCS,     // GPUパーティクル
        Animation,      // アニメーション描画用
        Skybox,         // スカイボックスの描画用
        ShadowMap,      // シャドウマップ用
        Grid,           // デバックのグリッド描画用
        DebugLine,      // デバックのライン描画用
        Fracture,       // 破片の描画
        RuntimeCutFragments, // 動的な破片
        RuntimeCutIceFragments, // 氷の動的な破片
    };

    enum class Draw2dType {
        Normal,
        Add,
        Text,
    };

    // 描画に使用するリソース
    struct Draw3dRequest {
        Draw3dType type = Draw3dType::Default;
        RenderLayer layer = RenderLayer::Opaque;
        std::string passName = "DefaultPass";

        const Model* model = nullptr;
        WorldTransform* worldTransform = nullptr;
        // インスタンシング描画
        uint32_t numInstances = 1;
        WorldTransforms* worldTransforms = nullptr;

        // 破片のデータ
        FractureInstance* fractureInstance = nullptr;

        // マテリアル
        const GpuResource* material = nullptr;

        // GPUパーティクル
        const SrvResource* particleCsResource = nullptr;

        // デバック用のラインデータ
        const DebugRenderer* debugRenderer_ = nullptr;
    };

    // 2D描画に使用するリソース
    struct Draw2dRequest {
        Draw2dType type = Draw2dType::Normal;
        RenderLayer layer = RenderLayer::Sprite;
        std::string passName = "DefaultPass";

        const Sprite* sprite = nullptr;
        const Text* text = nullptr;
    };
}

 
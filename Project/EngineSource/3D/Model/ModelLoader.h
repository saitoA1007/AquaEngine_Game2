#pragma once
#include "AnimationData.h"
#include "TextureManager.h"
#include "SrvManager.h"
#include "FractureData.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace GameEngine {

    class Model;

	class ModelLoader final {
    public:
        // 1枚の三角形の判定に使う形状情報
        struct TriangleGeometry {
            Vector3 p[3];
            Vector3 normal;
            float planeDist = 0.0f;
            bool isDegenerate = false;
        };

	public:
		ModelLoader() = default;
		~ModelLoader() = default;

        // 初期化処理
		void Initialize(ID3D12GraphicsCommandList4* cmdList, TextureManager* textureManager, SrvManager* srvManager);

    public: // 生成処理

        /// <summary>
        /// gltfやobjファイルからモデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreateModel(const std::string& objFilename, const std::string& filename);

        /// <summary>
        /// ファイルを読み込んでCPU上のデータにする
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<ModelData> ParseModel(const std::string& objFilename, const std::string& filename);

        /// <summary>
        /// パース済みデータからGPUリソースを作ってモデルを組み立てる
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> BuildModel(const std::string& objFilename, std::unique_ptr<ModelData> parsed);

        /// <summary>
        /// 球モデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreateSphere(uint32_t subdivision);

        /// <summary>
        /// 平面モデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreatePlane(const Vector2& size);

        /// <summary>
        /// グリッド平面モデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreateGridPlane(const Vector2& size);

        /// <summary>
        /// リングモデルを生成する
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreateRing(uint32_t ringDivide, float outerRadius, float innerRadius);

        /// <summary>
        /// 円柱モデルを生成
        /// </summary>
        [[nodiscard]]
        std::unique_ptr<Model> CreateCylinder(uint32_t cylinderDivide, float topRadius, float bottomRadius, float height);

        /// <summary>
        /// アニメーションデータを読み込み
        /// </summary>
        [[nodiscard]]
        AnimationData LoadAnimationFile(const std::string& objFilename,
            const std::string& filename);

        /// <summary>
        /// アニメーションデータを読み込む
        /// </summary>
        /// <param name="objFilename"></param>
        /// <param name="filename"></param>
        /// <returns></returns>
        [[nodiscard]]
        static std::map<std::string, AnimationData> LoadAnimationsFile(const std::string& objFilename, const std::string& filename);

        /// <summary>
        /// スケルトンを生成
        /// </summary>
        [[nodiscard]]
        SkeletonData CreateSkeleton(const Node& rootNode);

    private:
        ModelLoader(const ModelLoader&) = delete;
        ModelLoader& operator=(const ModelLoader&) = delete;

        ID3D12GraphicsCommandList4* cmdList_ = nullptr;
        TextureManager* textureManager_ = nullptr;
        SrvManager* srvManager_ = nullptr;

        static inline const std::string kDirectoryPath_ = "Resources/Models";

        // ノード名にこの文字列が含まれていればチャンクと判断する
        static inline const std::string kFractureChunkMarker_ = "_cell";

        // 破片ノード同士がこの距離以内ならAABBが近接/重なっているとみなす
        static constexpr float kFractureAdjacencyThreshold_ = 0.01f;

        // アンカー判定：グループの全高に対して、最下端からこの割合以内にあるチャンクを地面固定とみなす
        static constexpr float kFractureAnchorHeightRatio_ = 0.1f;

    private:

        /// <summary>
        /// モデルデータのファイル読み込み
        /// </summary>
        /// <param name="directoryPath"></param>
        /// <param name="objFilename"></param>
        /// <param name="filename"></param>
        /// <returns></returns>
        [[nodiscard]]
        ModelData LoadModelFile(const std::string& directoryPath, const std::string& objFilename, const std::string& filename);

        /// <summary>
        /// Node情報を取得
        /// </summary>
        /// <param name="node"></param>
        /// <returns></returns>
        [[nodiscard]]
        Node ReadNode(aiNode* node);

        /// <summary>
        /// ジョイントを作成する
        /// </summary>
        /// <param name="node"></param>
        /// <param name="parent"></param>
        /// <param name="joints"></param>
        /// <returns></returns>
        [[nodiscard]]
        int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

        /// <summary>
        /// ノード階層を調べて、名前に_cellが書いてあるノードをチャンクとして取得する
        /// </summary>
        /// <param name="node"></param>
        /// <param name="parentTransform"></param>
        /// <param name="outChunkInfoByMeshIndex"></param>
        /// <param name="outNodeTransformByMeshIndex"></param>
        /// <param name="chunkCounterByGroup"></param>
        void DetectFractureChunks(aiNode* node, const aiMatrix4x4& parentTransform,
            std::vector<std::optional<FractureChunkInfo>>& outChunkInfoByMeshIndex,
            std::vector<std::optional<aiMatrix4x4>>& outNodeTransformByMeshIndex,
            std::unordered_map<std::string, uint32_t>& chunkCounterByGroup);

        /// <summary>
        /// ノード名から破壊グループ名を抽出する。_cellを含まない場合はfalseを返す
        /// </summary>
        [[nodiscard]]
        bool TryExtractFractureGroupName(const std::string& nodeName, std::string& outGroupName);

        /// <summary>
        /// 各チャンクのAABBから、同じグループ内で近接、重なっているチャンク同士を隣接として登録する
        /// </summary>
        void BuildFractureAdjacency(ModelData& modelData, float threshold = kFractureAdjacencyThreshold_);

        /// <summary>
        /// 各チャンクの三角形が元の外側の表面か、どの隣接チャンクと接している内部の切断面かを判定する
        /// </summary>
        void ClassifyFractureFaces(ModelData& modelData, float threshold = kFractureAdjacencyThreshold_);

	};
}
#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <queue>

namespace GameEngine {

    // 前方宣言
    class DXFence;

	/// <summary>
	/// 一時的に作成したGPUリソースなどをGPUの実行が完了してから破棄するクラス
	/// </summary>
	class ResourceGarbageCollector {
	public:
		static ResourceGarbageCollector& GetInstance() {
			static ResourceGarbageCollector instance;
			return instance;
		}

        // フェンス機能を取得
        void SetFence(DXFence* fence) {
            fence_ = fence;
        }

        /// <summary>
        /// 破棄したいリソースを登録する
        /// </summary>
        /// <param name="resource">破棄するリソース</param>
        void Add(Microsoft::WRL::ComPtr<ID3D12Resource> resource);

        /// <summary>
        /// 毎フレーム呼び出し、安全になったリソースを解放する
        /// </summary>
        void ProcessCompletedResources();

    private:
        ResourceGarbageCollector() = default;
        ~ResourceGarbageCollector() = default;

        DXFence* fence_ = nullptr;

        struct ReleaseItem {
            Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            uint64_t fenceValue;
        };

        std::queue<ReleaseItem> releaseQueue_;
	};
}
#pragma once
#include <d3d12.h>
#include <string>

#if defined(USE_PIX)
#include <Windows.h>
#include <pix3.h>
#endif

namespace GameEngine {

	//==========================================================================
	// pix上でイベントカラー
	//==========================================================================
	namespace PixColor {

		/// <summary>
		/// RGBを 0xffRRGGBB に詰める
		/// </summary>
		constexpr unsigned long long MakeColor(unsigned long long r, unsigned long long g, unsigned long long b) {
			return 0xff000000ull | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
		}

		inline constexpr unsigned long long Frame = MakeColor(200, 200, 200);   // フレーム全体
		inline constexpr unsigned long long Scene = MakeColor(100, 180, 255);   // シーン描画
		inline constexpr unsigned long long Pass = MakeColor(120, 220, 160);   // レンダーパス
		inline constexpr unsigned long long PostEffect = MakeColor(255, 190, 100);   // ポストエフェクト
		inline constexpr unsigned long long Raytracing = MakeColor(220, 130, 255);   // レイトレーシング
		inline constexpr unsigned long long Compute = MakeColor(255, 130, 130);   // コンピュート
		inline constexpr unsigned long long UI = MakeColor(180, 180, 255);   // ImGui/UI
		inline constexpr unsigned long long Present = MakeColor(255, 255, 255);   // Present
	}

	//==========================================================================
	// スコープを抜けると自動で PIXEndEvent を呼ぶ RAII クラス
	//==========================================================================
	class ScopedPixEvent final {
	public:

#if defined(USE_PIX)
		ScopedPixEvent(ID3D12GraphicsCommandList* commandList, UINT64 color, const char* name)
			: commandList_(commandList) {
			// name に % が含まれていても壊れないよう "%s" 経由で渡す
			PIXBeginEvent(commandList_, color, "%s", name);
		}

		ScopedPixEvent(ID3D12GraphicsCommandList* commandList, UINT64 color, const std::string& name)
			: ScopedPixEvent(commandList, color, name.c_str()) {
		}

		~ScopedPixEvent() {
			PIXEndEvent(commandList_);
		}
#else
		ScopedPixEvent(ID3D12GraphicsCommandList*, unsigned long long, const char*) {}
		ScopedPixEvent(ID3D12GraphicsCommandList*, unsigned long long, const std::string&) {}
		~ScopedPixEvent() = default;
#endif

		ScopedPixEvent(const ScopedPixEvent&) = delete;
		ScopedPixEvent& operator=(const ScopedPixEvent&) = delete;

	private:
#if defined(USE_PIX)
		ID3D12GraphicsCommandList* commandList_ = nullptr;
#endif
	};

	//==========================================================================
	// 手動でBegin/Endしたい場合のラッパー（RAIIが使えない箇所用）
	//==========================================================================
	inline void PixBeginEvent(ID3D12GraphicsCommandList* commandList, unsigned long long color, const char* name) {
#if defined(USE_PIX)
		PIXBeginEvent(commandList, color, "%s", name);
#else
		(void)commandList; (void)color; (void)name;
#endif
	}

	inline void PixEndEvent(ID3D12GraphicsCommandList* commandList) {
#if defined(USE_PIX)
		PIXEndEvent(commandList);
#else
		(void)commandList;
#endif
	}

	// タイムライン上に一瞬のマーカーを打つ（Begin/End不要）
	inline void PixSetMarker(ID3D12GraphicsCommandList* commandList, unsigned long long color, const char* name) {
#if defined(USE_PIX)
		PIXSetMarker(commandList, color, "%s", name);
#else
		(void)commandList; (void)color; (void)name;
#endif
	}

	// CommandQueue 単位のイベント（ExecuteCommandLists の括り）
	inline void PixBeginEvent(ID3D12CommandQueue* commandQueue, unsigned long long color, const char* name) {
#if defined(USE_PIX)
		PIXBeginEvent(commandQueue, color, "%s", name);
#else
		(void)commandQueue; (void)color; (void)name;
#endif
	}

	inline void PixEndEvent(ID3D12CommandQueue* commandQueue) {
#if defined(USE_PIX)
		PIXEndEvent(commandQueue);
#else
		(void)commandQueue;
#endif
	}
}

//==============================================================================
// マクロ
//==============================================================================
#define PIX_MARKER_CONCAT_INNER(a, b) a##b
#define PIX_MARKER_CONCAT(a, b) PIX_MARKER_CONCAT_INNER(a, b)

#define PIX_SCOPED_EVENT(commandList, color, name) \
	::GameEngine::ScopedPixEvent PIX_MARKER_CONCAT(pixScopedEvent_, __LINE__)((commandList), (color), (name))

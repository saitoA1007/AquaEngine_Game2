#pragma once
#include <xaudio2.h>
#include <fstream>
#include <wrl.h>
#include <vector>
#include <unordered_map>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

namespace GameEngine {

	class AudioManager {
	public:

		// シングルトンインスタンスの取得
		static AudioManager& GetInstance();

		// コピーコンストラクタと代入演算子を無効化
		AudioManager(const AudioManager&) = delete;
		AudioManager& operator=(const AudioManager&) = delete;

		// 拡張子の種類
		enum Type {
			MP3,
			WAV,
		};

		// チャンクヘッダ
		struct ChunkHeader {
			char id[4];   // チャンク毎のID
			int32_t size; // チャンクサイズ
		};

		// RIFFヘッダチャンク
		struct RiffHeader {
			ChunkHeader chunk; // RIFF
			char type[4];  // WAVE
		};

		// FMTチャンク
		struct FormatChunk {
			ChunkHeader chunk; // fmt
			WAVEFORMATEX fmt;  // 波形フォーマット
		};

		// 音声データ
		struct SoundData {
			// 波形フォーマット
			WAVEFORMATEX wfex;
			// バッファの先頭アドレス
			std::vector<BYTE> pBuffer;
			// バッファのサイズ
			unsigned int bufferSize;
			// 音声データの名前
			std::string name;
			// 拡張子の種類
			Type type;
		};

	public:

		/// <summary>
		/// 初期化
		/// </summary>
		void Initialize();

		/// <summary>
		/// 終了処理
		/// </summary>
		void Finalize();

	public:

		/// <summary>
		/// 全ての音声データを読み込む
		/// </summary>
		void LoadAllAudio();

		/// <summary>
		/// 音声データを登録する
		/// </summary>
		/// <param name="fileName"></param>
		void RegisterAudio(const std::string& fileName);

		/// <summary>
		/// 音声をロード
		/// </summary>
		/// <param name="fileName"></param>
		/// <returns></returns>
		uint32_t Load(const std::string& fileName);

		/// <summary>
		/// 名前からハンドルを取得
		/// </summary>
		/// <param name="name"></param>
		/// <returns></returns>
		uint32_t GetHandleByName(const std::string& name) const;

		/// <summary>
		/// 音声を再生
		/// </summary>
		/// <param name="soundHandle"></param>
		void Play(uint32_t soundHandle, float volume, bool isloop);

		/// <summary>
		/// 音声を止める
		/// </summary>
		/// <param name="soundHandle"></param>
		void Stop(const uint32_t& soundHandle);

		/// <summary>
		/// 再生中か
		/// </summary>
		/// <param name="soundHandle"></param>
		/// <returns></returns>
		bool IsPlay(const uint32_t& soundHandle);

		/// <summary>
		/// 全ての音声を止める
		/// </summary>
		void StopAll();

	private:
		AudioManager() = default;
		~AudioManager() = default;

		Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
		IXAudio2MasteringVoice* masterVoice_;

		// s
		std::vector<SoundData> soundData_;

		// 音声データのハンドルを保存する
		std::unordered_map<std::string, uint32_t> nameToHandles_;

		// 再生中の音声を保存
		std::unordered_map<uint32_t, IXAudio2SourceVoice*> activeVoices_;

	private:
		/// <summary>
		/// .wav音声を再生
		/// </summary>
		/// <param name="xAudio2"></param>
		/// <param name="soundData"></param>
		void SoundPlayWave(const uint32_t& soundHandle, bool isloop);

		/// <summary>
		/// .mp3音声を再生
		/// </summary>
		/// <param name="soundData"></param>
		void SoundPlayMp3(const uint32_t& soundHandle, bool isloop);

		/// <summary>
		/// .wavファイルの読み込み
		/// </summary>
		/// <param name="filename"></param>
		/// <returns></returns>
		SoundData SoundLoadWave(const std::string& filename);

		/// <summary>
		/// .mp3ファイルの読み込み
		/// </summary>
		/// <param name="path"></param>
		/// <returns></returns>
		SoundData SoundLoadMp3(const std::wstring path);

		/// <summary>
		/// 音声データの解放
		/// </summary>
		void SoundUnload();

		/// <summary>
		/// ファイルの名前を取得する
		/// </summary>
		/// <param name="fullPath"></param>
		/// <returns></returns>
		std::string GetFileName(const std::string& fullPath);
	};
}
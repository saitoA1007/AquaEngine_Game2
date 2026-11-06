#pragma once
#include <xaudio2.h>
#include <fstream>
#include <wrl.h>
#include <vector>
#include <string>
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

		// 音量グループ
		struct VoiceGroup {
			IXAudio2SubmixVoice* submixVoice = nullptr; // グループの出力先
			float volume = 1.0f; // グループの音量
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
		/// <param name="soundHandle">音声ハンドル</param>
		/// <param name="volume">この音声の音量(0.0~1.0)</param>
		/// <param name="isloop">ループ再生するか</param>
		/// <param name="groupName">出力する音量グループ。空ならマスターに直接出力</param>
		void Play(uint32_t soundHandle, float volume, bool isloop, const std::string& groupName = "");

		/// <summary>
		/// 音声を止める（同じハンドルで再生中の音声を全て止める）
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

	public:

		/// <summary>
		/// 音量グループを登録する
		/// </summary>
		/// <param name="groupName">グループ名（"BGM"、"SE" など）</param>
		/// <param name="volume">初期音量(0.0~1.0)</param>
		void RegisterGroup(const std::string& groupName, float volume = 1.0f);

		/// <summary>
		/// グループの音量を設定する(0.0~1.0)
		/// </summary>
		/// <param name="groupName">グループ名</param>
		/// <param name="volume">音量</param>
		void SetGroupVolume(const std::string& groupName, float volume);

		/// <summary>
		/// グループの音量を取得する
		/// </summary>
		/// <param name="groupName">グループ名</param>
		/// <returns>音量</returns>
		float GetGroupVolume(const std::string& groupName) const;

		/// <summary>
		/// グループが登録されているか
		/// </summary>
		/// <param name="groupName">グループ名</param>
		/// <returns></returns>
		bool HasGroup(const std::string& groupName) const;

		/// <summary>
		/// 全体の音量を設定する(0.0~1.0)
		/// </summary>
		/// <param name="volume">音量</param>
		void SetMasterVolume(float volume);

		/// <summary>
		/// 全体の音量を取得する
		/// </summary>
		/// <returns>音量</returns>
		float GetMasterVolume() const { return masterVolume_; }

	private:
		AudioManager() = default;
		~AudioManager() = default;

		Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
		IXAudio2MasteringVoice* masterVoice_ = nullptr;

		// 読み込んだ音声データ
		std::vector<SoundData> soundData_;

		// 音声データのハンドルを保存する
		std::unordered_map<std::string, uint32_t> nameToHandles_;

		// 再生中の音声を保存
		std::unordered_map<uint32_t, std::vector<IXAudio2SourceVoice*>> activeVoices_;

		// 登録された音量グループ
		std::unordered_map<std::string, VoiceGroup> groups_;

		// 全体の音量
		float masterVolume_ = 1.0f;

	private:

		/// <summary>
		/// 指定したグループに出力するSourceVoiceを作成
		/// </summary>
		/// <param name="soundData">音声データ</param>
		/// <param name="groupName">グループ名</param>
		/// <returns>作成したSourceVoice。失敗時はnullptr</returns>
		IXAudio2SourceVoice* CreateSourceVoice(const SoundData& soundData, const std::string& groupName);

		/// <summary>
		/// SourceVoiceを停止して破棄する
		/// </summary>
		/// <param name="voice"></param>
		void DestroySourceVoice(IXAudio2SourceVoice* voice);

		/// <summary>
		/// 再生が終わったSourceVoiceを破棄する
		/// </summary>
		void CleanupFinishedVoices();

		/// <summary>
		/// SourceVoiceが再生中か
		/// </summary>
		/// <param name="voice"></param>
		/// <returns></returns>
		static bool IsVoicePlaying(IXAudio2SourceVoice* voice);

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
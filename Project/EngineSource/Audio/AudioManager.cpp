#include "pch.h"
#include "AudioManager.h"
#include <cassert>
#include <filesystem>
#include "ConvertString.h"
#include "LogManager.h"

#pragma comment(lib,"xaudio2.lib")

#pragma comment(lib, "Mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "Mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

using namespace GameEngine;

AudioManager& AudioManager::GetInstance() {
	static AudioManager instance;
	return instance;
}

void AudioManager::Finalize() {
	xAudio2_.Reset();
	SoundUnload();

	MFShutdown();

	CoUninitialize();
}

void AudioManager::Initialize() {

	// MediaFoundationの初期化
	MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);

	// XAudioエンジンのインスタンスを生成
	HRESULT result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));

	// マスターボイスを生成
	result = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(result));
}

void AudioManager::SoundPlayWave(const uint32_t& soundHandle, bool isloop) {
	HRESULT result;

	const SoundData& soundData = soundData_[soundHandle];

	// 波形フォーマットを元にSourceVoiceの生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer.data();
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.LoopCount = isloop ? XAUDIO2_LOOP_INFINITE : 0;

	// 波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
}

AudioManager::SoundData AudioManager::SoundLoadWave(const std::string& filename) {

	// ファイル入力ストリームのインスタンス
	std::ifstream file;
	// .wavファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);
	// ファイルオープン失敗を検出する
	assert(file.is_open());

	// RIFFヘッダーの読み込み
	RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	// ファイルがRIFFかチェック
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}
	// タイプがWAVRかチェック
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	FormatChunk format = {};
	ChunkHeader chunk = {};

	// チャンクを順に読みながらfmtチャンクを探す
	while (file.read((char*)&chunk, sizeof(chunk))) {
		if (strncmp(chunk.id, "fmt ", 4) == 0) {
			// サイズが適正か確認
			assert(chunk.size <= sizeof(format.fmt));
			format.chunk = chunk;
			file.read((char*)&format.fmt, chunk.size);
			break;
		} else {
			// 必要のないチャンクは読み飛ばす
			file.seekg(chunk.size, std::ios_base::cur);
		}
	}

	// fmtチャンクが見つからなければエラー
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}

	// Dataチャンクの読み込み
	ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	// JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0) {
		// 読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);
		// 再読み込み
		file.read((char*)&data, sizeof(data));

	}

	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}
	// Dataチャンクのデータ部（波形データ）の読み込み
	std::vector<BYTE> pBuffer(data.size);
	file.read(reinterpret_cast<char*>(pBuffer.data()), data.size);
	// Waveファイルを閉じる
	file.close();

	// returnする為の音声データ
	SoundData soundData = {};
	soundData.wfex = format.fmt;
	soundData.pBuffer = std::move(pBuffer);
	soundData.bufferSize = data.size;
	soundData.name = filename;
	soundData.type = WAV;

	return soundData;
}

void AudioManager::SoundUnload() {
	soundData_.clear();
}

uint32_t AudioManager::Load(const std::string& fileName) {

	// データ数が0でなく、同じ音声データがあればその配列番号を返す
	if (soundData_.size() != 0) {
		for (uint32_t i = 0; i < soundData_.size(); ++i) {
			if (soundData_.at(i).name == fileName) {
				return i;
			}
		}
	}

	// 拡張子からファイルタイプを判別
	std::filesystem::path path(fileName);
	std::string extension = path.extension().string();

	// 各拡張子をロード
	if (extension == ".wav") {
		soundData_.push_back(SoundLoadWave(fileName));
	} else if (extension == ".mp3") {
		soundData_.push_back(SoundLoadMp3(ConvertString(fileName)));
	} else {
		// サポートしていない拡張子への対応
		throw std::runtime_error("Unsupported audio format: " + extension);
	}

	// 読み込んだデータが格納されている配列番号を返す
	return uint32_t(soundData_.size() - 1);
}

AudioManager::SoundData AudioManager::SoundLoadMp3(const std::wstring path) {

	SoundData soundData = {};

	// 音声データを登録
	soundData.type = MP3;
	soundData.name = ConvertString(path);

	HRESULT hr;

	// ソースリーダーの作成
	IMFSourceReader* pMFSourceReader{ nullptr };
	hr = MFCreateSourceReaderFromURL(path.c_str(), NULL, &pMFSourceReader);
	if (FAILED(hr)) {
		LogManager::GetInstance().Log("MFCreateSourceReaderFromURL failed: " + soundData.name + " hr=" + std::to_string(hr));
		assert(SUCCEEDED(hr));
		return soundData;
	}

	// メディアタイプを取得
	IMFMediaType* pMFMediaType{ nullptr };
	MFCreateMediaType(&pMFMediaType);
	pMFMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pMFMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	hr = pMFSourceReader->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), nullptr, pMFMediaType);
	assert(SUCCEEDED(hr));

	pMFMediaType->Release();
	pMFMediaType = nullptr;
	hr = pMFSourceReader->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), &pMFMediaType);
	assert(SUCCEEDED(hr));

	WAVEFORMATEX* waveFormat{ nullptr };
	hr = MFCreateWaveFormatExFromMFMediaType(pMFMediaType, &waveFormat, nullptr);
	assert(SUCCEEDED(hr));

	// データ読み込み
	std::vector<BYTE> bufferData;
	while (true) {
		IMFSample* pMFSample{ nullptr };
		DWORD dwStreamFlags = 0;
		hr = pMFSourceReader->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM), 0, nullptr, &dwStreamFlags, nullptr, &pMFSample);
		assert(SUCCEEDED(hr));

		if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
			break;
		}

		IMFMediaBuffer* pMFMediaBuffer{ nullptr };
		hr = pMFSample->ConvertToContiguousBuffer(&pMFMediaBuffer);
		assert(SUCCEEDED(hr));

		BYTE* pBuffer = nullptr;
		DWORD cbCurrentLength = 0;
		hr = pMFMediaBuffer->Lock(&pBuffer, nullptr, &cbCurrentLength);
		assert(SUCCEEDED(hr));
		// データを一時バッファに追加
		bufferData.insert(bufferData.end(), pBuffer, pBuffer + cbCurrentLength);

		// 解放処理
		pMFMediaBuffer->Unlock();
		pMFMediaBuffer->Release();
		pMFSample->Release();

	}

	LogManager::GetInstance().Log("Loaded MP3: " + soundData.name + " bytes=" + std::to_string(bufferData.size())
		+ " formatTag=" + std::to_string(waveFormat->wFormatTag) + " cbSize=" + std::to_string(waveFormat->cbSize));

	soundData.wfex = *waveFormat;
	soundData.pBuffer = std::move(bufferData);
	soundData.bufferSize = static_cast<UINT32>(soundData.pBuffer.size());

	// 解放処理
	CoTaskMemFree(waveFormat);
	pMFMediaType->Release();
	pMFSourceReader->Release();

	return soundData;
}

void AudioManager::Play(uint32_t soundHandle, float volume, bool isloop) {

	// 音声を再生
	if (soundData_[soundHandle].type == MP3) {
		SoundPlayMp3(soundHandle, isloop);

		// 音量を設定
		auto it = activeVoices_.find(soundHandle);
		if (it != activeVoices_.end()) {
			it->second->SetVolume(volume);
		}

	} else {
		SoundPlayWave(soundHandle, isloop);
	}
}

void AudioManager::Stop(const uint32_t& soundHandle) {
	auto it = activeVoices_.find(soundHandle);
	if (it != activeVoices_.end()) {
		IXAudio2SourceVoice* pSourceVoice = it->second;
		if (pSourceVoice) {
			pSourceVoice->Stop(0);
			pSourceVoice->FlushSourceBuffers();
			pSourceVoice->DestroyVoice();
		}
		activeVoices_.erase(it); // 管理から削除
	}
}

void AudioManager::StopAll() {
	// 管理している全てのボイスを停止・破棄する
	for (auto& voicePair : activeVoices_) {
		IXAudio2SourceVoice* pSourceVoice = voicePair.second;
		if (pSourceVoice) {
			// 再生停止
			pSourceVoice->Stop(0);
			// バッファをフラッシュ（待機中のデータを破棄）
			pSourceVoice->FlushSourceBuffers();
			// ボイスを削除
			pSourceVoice->DestroyVoice();
		}
	}

	// マップをクリアして、再生中の情報をリセット
	activeVoices_.clear();
}

void AudioManager::SoundPlayMp3(const uint32_t& soundHandle, bool isloop) {
	HRESULT result;

	const SoundData& soundData = soundData_[soundHandle];

	IXAudio2SourceVoice* pSourceVoice{ nullptr };
	result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	if (FAILED(result)) {
		LogManager::GetInstance().Log("CreateSourceVoice failed for MP3: " + soundData.name + " hr=" + std::to_string(result));
		assert(SUCCEEDED(result));
		return;
	}

	XAUDIO2_BUFFER buffer{ 0 };
	buffer.pAudioData = soundData.pBuffer.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = sizeof(BYTE) * static_cast<UINT32>(soundData.bufferSize);
	buffer.LoopCount = isloop ? XAUDIO2_LOOP_INFINITE : 0;

	// 再生する
	result = pSourceVoice->SubmitSourceBuffer(&buffer);
	assert(SUCCEEDED(result));
	result = pSourceVoice->Start();
	assert(SUCCEEDED(result));

	// 管理テーブルに登録
	activeVoices_[soundHandle] = pSourceVoice;
}

bool AudioManager::IsPlay(const uint32_t& soundHandle) {
	// 再生中のボイスを探す
	auto it = activeVoices_.find(soundHandle);
	if (it == activeVoices_.end()) {
		return false; // 登録なし → 再生していない
	}

	IXAudio2SourceVoice* pSourceVoice = it->second;
	XAUDIO2_VOICE_STATE state{};
	pSourceVoice->GetState(&state);

	// state.BuffersQueued が 0 なら再生終了
	return (state.BuffersQueued > 0);
}

void AudioManager::RegisterAudio(const std::string& fileName) {

	std::string audioName = GetFileName(fileName);

	// 同名のモデルが登録されている場合は早期リターン
	auto getName = nameToHandles_.find(audioName);
	if (getName != nameToHandles_.end()) {
		return;
	}

	// ロードする
	uint32_t handle = Load(fileName);

	// 登録する
	nameToHandles_[audioName] = handle;
}

void AudioManager::LoadAllAudio() {
	namespace fs = std::filesystem;
	const std::string kDirectoryPath = "Resources/Sounds/";

	// Soundsのフォルダが存在するか確認する
	if (!fs::exists(kDirectoryPath)) {
		LogManager::GetInstance().Log("Audio directory not found, skipping LoadAllAudio: " + kDirectoryPath);
		return;
	}

	LogManager::GetInstance().Log("Start Loading All Audios from: " + kDirectoryPath);

	// Soundsのフォルダにある音声ファイルとフォルダを検索
	for (const auto& entry : fs::recursive_directory_iterator(kDirectoryPath)) {

		// 音声を登録、ロードする
		if (entry.is_regular_file()) {
			// ファイルパスを取得する
			std::string filePath = entry.path().string();
			// 登録
			RegisterAudio(filePath);
		}
	}

	LogManager::GetInstance().Log("End Loading All Audios");
}

std::string AudioManager::GetFileName(const std::string& fullPath) {
	return std::filesystem::path(fullPath).filename().string();
}

uint32_t AudioManager::GetHandleByName(const std::string& name) const {
	auto getHandle = nameToHandles_.find(name);
	if (getHandle == nameToHandles_.end()) {
		return 0;
	}
	return getHandle->second;
}
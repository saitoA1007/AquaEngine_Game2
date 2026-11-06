#include "pch.h"
#include "AudioManager.h"
#include <cassert>
#include <filesystem>
#include <algorithm>
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
	// 再生中の音声 → グループ → マスターの順に破棄する
	StopAll();

	for (auto& [name, group] : groups_) {
		if (group.submixVoice) {
			group.submixVoice->DestroyVoice();
			group.submixVoice = nullptr;
		}
	}
	groups_.clear();

	if (masterVoice_) {
		masterVoice_->DestroyVoice();
		masterVoice_ = nullptr;
	}

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
	masterVoice_->SetVolume(masterVolume_);
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

void AudioManager::Play(uint32_t soundHandle, float volume, bool isloop, const std::string& groupName) {

	// 範囲外のハンドルは再生しない
	if (soundHandle >= soundData_.size()) {
		LogManager::GetInstance().Log("Invalid sound handle: " + std::to_string(soundHandle));
		return;
	}

	// 再生が終わったボイスを片付ける
	CleanupFinishedVoices();

	const SoundData& soundData = soundData_[soundHandle];

	// 指定したグループに出力するSourceVoiceを生成
	// WAV・MP3ともに読み込み時にPCMへ変換済みなので、同じ処理で再生できる
	IXAudio2SourceVoice* pSourceVoice = CreateSourceVoice(soundData, groupName);
	if (!pSourceVoice) {
		return;
	}

	// 再生する波形データの設定
	XAUDIO2_BUFFER buffer{};
	buffer.pAudioData = soundData.pBuffer.data();
	buffer.AudioBytes = soundData.bufferSize;
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.LoopCount = isloop ? XAUDIO2_LOOP_INFINITE : 0;

	HRESULT result = pSourceVoice->SubmitSourceBuffer(&buffer);
	if (FAILED(result)) {
		LogManager::GetInstance().Log("SubmitSourceBuffer failed: " + soundData.name + " hr=" + std::to_string(result));
		pSourceVoice->DestroyVoice();
		return;
	}

	// この音声個別の音量を設定
	pSourceVoice->SetVolume(std::clamp(volume, 0.0f, 1.0f));

	// 再生する
	result = pSourceVoice->Start();
	assert(SUCCEEDED(result));

	// 管理テーブルに登録
	activeVoices_[soundHandle].push_back(pSourceVoice);
}

void AudioManager::Stop(const uint32_t& soundHandle) {
	auto it = activeVoices_.find(soundHandle);
	if (it != activeVoices_.end()) {
		// 同じハンドルで再生中のボイスを全て破棄する
		for (IXAudio2SourceVoice* pSourceVoice : it->second) {
			DestroySourceVoice(pSourceVoice);
		}
		activeVoices_.erase(it); // 管理から削除
	}
}

void AudioManager::StopAll() {
	// 管理している全てのボイスを停止・破棄する
	for (auto& [handle, voices] : activeVoices_) {
		for (IXAudio2SourceVoice* pSourceVoice : voices) {
			DestroySourceVoice(pSourceVoice);
		}
	}

	// マップをクリアして、再生中の情報をリセット
	activeVoices_.clear();
}

bool AudioManager::IsPlay(const uint32_t& soundHandle) {
	// 再生中のボイスを探す
	auto it = activeVoices_.find(soundHandle);
	if (it == activeVoices_.end()) {
		return false;
	}

	// 1つでも再生中のボイスがあれば再生中
	for (IXAudio2SourceVoice* pSourceVoice : it->second) {
		if (IsVoicePlaying(pSourceVoice)) {
			return true;
		}
	}
	return false;
}

void AudioManager::RegisterGroup(const std::string& groupName, float volume) {

	if (groupName.empty()) {
		LogManager::GetInstance().Log("Audio group name is empty");
		return;
	}

	// 同名のグループが登録されている場合は音量だけ更新する
	if (HasGroup(groupName)) {
		SetGroupVolume(groupName, volume);
		return;
	}

	// マスターボイスと同じチャンネル数・サンプルレートでサブミックスを作成
	XAUDIO2_VOICE_DETAILS details{};
	masterVoice_->GetVoiceDetails(&details);

	VoiceGroup group;
	HRESULT result = xAudio2_->CreateSubmixVoice(&group.submixVoice, details.InputChannels, details.InputSampleRate);
	if (FAILED(result)) {
		LogManager::GetInstance().Log("CreateSubmixVoice failed: " + groupName + " hr=" + std::to_string(result));
		assert(SUCCEEDED(result));
		return;
	}

	// 音量を設定
	group.volume = std::clamp(volume, 0.0f, 1.0f);
	group.submixVoice->SetVolume(group.volume);

	groups_[groupName] = group;
}

void AudioManager::SetGroupVolume(const std::string& groupName, float volume) {
	auto it = groups_.find(groupName);
	if (it == groups_.end()) {
		LogManager::GetInstance().Log("Audio group not found: " + groupName);
		return;
	}
	it->second.volume = std::clamp(volume, 0.0f, 1.0f);
	it->second.submixVoice->SetVolume(it->second.volume);
}

float AudioManager::GetGroupVolume(const std::string& groupName) const {
	auto it = groups_.find(groupName);
	return (it != groups_.end()) ? it->second.volume : 1.0f;
}

bool AudioManager::HasGroup(const std::string& groupName) const {
	return groups_.contains(groupName);
}

void AudioManager::SetMasterVolume(float volume) {
	masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
	if (masterVoice_) {
		masterVoice_->SetVolume(masterVolume_);
	}
}

IXAudio2SourceVoice* AudioManager::CreateSourceVoice(const SoundData& soundData, const std::string& groupName) {
	IXAudio2SourceVoice* pSourceVoice = nullptr;
	HRESULT result = S_OK;

	// 出力先のグループを探す
	auto it = groups_.find(groupName);
	if (it != groups_.end()) {
		// グループのサブミックスに出力する
		XAUDIO2_SEND_DESCRIPTOR send{ 0, it->second.submixVoice };
		XAUDIO2_VOICE_SENDS sends{ 1, &send };
		result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, &sends);
	} else {
		// 未登録の名前が指定された場合はログを出し、マスターに直接出力する
		if (!groupName.empty()) {
			LogManager::GetInstance().Log("Audio group not found, output to master: " + groupName);
		}
		result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	}

	if (FAILED(result)) {
		LogManager::GetInstance().Log("CreateSourceVoice failed: " + soundData.name + " hr=" + std::to_string(result));
		assert(SUCCEEDED(result));
		return nullptr;
	}
	return pSourceVoice;
}

void AudioManager::DestroySourceVoice(IXAudio2SourceVoice* voice) {
	if (!voice) {
		return;
	}
	// 再生停止
	voice->Stop(0);
	// バッファをフラッシュ
	voice->FlushSourceBuffers();
	// ボイスを削除
	voice->DestroyVoice();
}

void AudioManager::CleanupFinishedVoices() {
	for (auto it = activeVoices_.begin(); it != activeVoices_.end();) {
		std::vector<IXAudio2SourceVoice*>& voices = it->second;

		// 再生が終わったボイスを破棄して取り除く
		std::erase_if(voices, [this](IXAudio2SourceVoice* voice) {
			if (IsVoicePlaying(voice)) {
				return false;
			}
			DestroySourceVoice(voice);
			return true;
			});

		// ボイスが無くなったハンドルは管理から外す
		if (voices.empty()) {
			it = activeVoices_.erase(it);
		} else {
			++it;
		}
	}
}

bool AudioManager::IsVoicePlaying(IXAudio2SourceVoice* voice) {
	if (!voice) {
		return false;
	}
	XAUDIO2_VOICE_STATE state{};
	voice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
	// BuffersQueued が 0 なら再生終了
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
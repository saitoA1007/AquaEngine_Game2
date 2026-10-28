#pragma once

// 時間を管理する
class TimeController {
public:
	TimeController();

	// 更新処理
	void Update();

public:

	// 時間を止める
	void StartStopTime(float stopMaxTime) {
		if (isStopTimeActive_) { return; }
		stopMaxTime_ = stopMaxTime;
		isStopTimeActive_ = true;
	}

	// リセット
	void Reset() {
		isStopTimeActive_ = false;
		timer_ = 0.0f;
	}

private:
	bool isStopTimeActive_ = false;
	float stopMaxTime_ = 0.0f;
	float timer_ = 0.0f;
};
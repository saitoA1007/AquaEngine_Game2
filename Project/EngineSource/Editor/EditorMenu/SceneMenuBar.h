#pragma once
#include"SceneChangeRequest.h"
#include <string>

namespace GameEngine {

	class SceneMenuBar {
	public:

		SceneMenuBar(SceneChangeRequest* request);

		void Run();

	private:
		SceneChangeRequest* sceneChangeRequest_ = nullptr;
	};
}


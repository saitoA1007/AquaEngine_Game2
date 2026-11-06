#pragma once
#include "BufferRefManager.h"

namespace GameEngine {

	class BufferRefResource {
	public:
		virtual ~BufferRefResource() = default;

		static void StaticInitialize(BufferRefManager* bufferRefManager) {
			bufferRefManager_ = bufferRefManager;
		}

	protected:
		static BufferRefManager* bufferRefManager_;
	};
}
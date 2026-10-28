#pragma once
#include "BufferRefManager.h"

namespace GameEngine {

	class BufferRefResource {
	public:
		virtual ~BufferRefResource() = default;

		static void StaticInitialize(BufferRefManager* bufferRefManager,const uint32_t& bufferStartIndex) {
			bufferRefManager_ = bufferRefManager;
			bufferStartIndex_ = bufferStartIndex;
		}

		const uint32_t& GetBufferStartIndex() const { return bufferStartIndex_; }

	protected:
		static BufferRefManager* bufferRefManager_;
		static uint32_t bufferStartIndex_;
	};
}
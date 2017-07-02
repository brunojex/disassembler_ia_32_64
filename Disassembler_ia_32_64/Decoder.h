#pragma once

#include "Instruction.h"

#include <cstdint>

namespace diasm
{
	class Decoder
	{
	public:
		enum class Operation_mode
		{
			_16bit,
			_32bit,
			_64bit
		};

		//returns empty instruction if decode fails
		Instruction decode(const uint8_t* data) const noexcept;

		Decoder(Operation_mode mode)
			:mode_(mode)
		{}
	private:
		const Operation_mode mode_;
	};
}
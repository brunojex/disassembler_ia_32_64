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
			_32bit,
			_64bit
		};

		//returns empty instruction if decode fails
		Instruction decode(const uint8_t* data) const noexcept;

		Decoder(Operation_mode mode, bool default_addressing_mode_16bit, bool default_operand_size_16bit)
			:mode_(mode)
			,default_addressing_mode_16bit_(default_addressing_mode_16bit)
			,default_operand_size_16bit_(default_operand_size_16bit)
		{}
	private:
		const Operation_mode mode_;
		const bool default_addressing_mode_16bit_;
		const bool default_operand_size_16bit_;
	};
}
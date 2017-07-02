#pragma once

#include "common_def.h"

#include <cstring>

namespace diasm
{
	struct Instruction
	{
		Instruction() 
		{
			static_assert(16 == sizeof(Instruction), "Instruction size isn't 16 bytes");
			std::memset(this, 0, sizeof(Instruction));
		}

		bool empty() { return !original_size; }

		int32_t immediate;
		int32_t displacement;
		Opcode opcode;
		Register register_[3];
		struct 
		{
		Bit_size immediate_size : 2;
		Bit_size displacement_size : 2;
		Custom_prefix prefix : 4;
		Operand_format operand1 : 4;
		Operand_format operand2 : 4;
		Operand_format operand3 : 4;
		Operand_format operand4 : 4;
		uint8_t original_size : 4;
		Addressing_size addressing_size : 4; //Placeholder for now
		};
	};
}
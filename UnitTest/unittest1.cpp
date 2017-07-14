#include "stdafx.h"
#include "CppUnitTest.h"
#include "test_common.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(AND_AL_imm8)
		{
			test_AL_imm8(Opcode::and, { 0x24, 0xFF }); //and AL, 0xFF
		}

		TEST_METHOD(AND_AX_imm16)
		{
			test_AX_imm16(Opcode::and, { 0x25, 0xFF, 0xF0 }); //and AX, 0xFFF0
		}

		TEST_METHOD(AND_EAX_imm32)
		{
			test_EAX_imm32(Opcode::and, { 0x25, 0xFF, 0xF0, 0xFF, 0xF0 }); //and EAX, 0xFFF0FFF0
		}

		TEST_METHOD(AND_RAX_imm32)
		{
			test_RAX_imm32(Opcode::and, { rex_w, 0x25, 0xFF, 0xF0, 0xFF, 0xF0 }); //and RAX, 0xFFF0FFF0
		}

		TEST_METHOD(AND_rm8_imm8)
		{
			test_extended_rm8_imm8(Opcode::and, 0x80, 0x04);
		}

		//add rex rm8_imm8

		TEST_METHOD(AND_rm16_imm16)
		{
			test_extended_rm16_imm16(Opcode::and, 0x81, 0x04);
		}

		TEST_METHOD(AND_rm32_imm32)
		{
			test_extended_rm32_imm32(Opcode::and, 0x81, 0x04);
		}

		//add rex rm64_imm32

		TEST_METHOD(AND_rm16_imm8)
		{
			test_extended_rm16_imm8(Opcode::and, 0x83, 0x04);
		}

		TEST_METHOD(AND_rm32_imm8)
		{
			test_extended_rm32_imm8(Opcode::and, 0x83, 0x04);
		}

		//add rex rm64_imm8
	};
}
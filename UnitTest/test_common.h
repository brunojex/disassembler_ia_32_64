#pragma once
#include "stdafx.h"
#include "CppUnitTest.h"
#include "common_def.h"

#include <vector>
#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace diasm;

namespace UnitTest
{
	static const constexpr uint8_t rex = 0x40;
	static const constexpr uint8_t rex_b = 0x41;
	static const constexpr uint8_t rex_x = 0x42;
	static const constexpr uint8_t rex_xb = 0x43;
	static const constexpr uint8_t rex_r = 0x44;
	static const constexpr uint8_t rex_rb = 0x45;
	static const constexpr uint8_t rex_rx = 0x46;
	static const constexpr uint8_t rex_rxb = 0x47;
	static const constexpr uint8_t rex_w = 0x48;
	static const constexpr uint8_t rex_wb = 0x49;
	static const constexpr uint8_t rex_wx = 0x50;
	static const constexpr uint8_t rex_wxb = 0x51;
	static const constexpr uint8_t rex_wr = 0x52;
	static const constexpr uint8_t rex_wrb = 0x53;
	static const constexpr uint8_t rex_wrx = 0x54;
	static const constexpr uint8_t rex_wrxb = 0x55;

	void test_AL_imm8(Opcode opcode, const std::vector<uint8_t>& data);

	void test_AX_imm16(Opcode opcode, const std::vector<uint8_t>& data);

	void test_EAX_imm32(Opcode opcode, const std::vector<uint8_t>& data);

	void test_RAX_imm32(Opcode opcode, const std::vector<uint8_t>& data);

	void test_extended_rm8_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field);

	void test_extended_rm16_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field);

	void test_extended_rm32_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field);

	void test_extended_rm64_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field);

	void test_extended_rm16_imm16(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field);

	void test_extended_rm32_imm32(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field);

	void test_extended_rm64_imm32(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field);

	void test_rm8_r8(Opcode opcode, uint8_t opcode_byte);

	void test_rm16_r16(Opcode opcode, uint8_t opcode_byte);

	void test_rm32_r32(Opcode opcode, uint8_t opcode_byte);

	void test_rm64_r64(Opcode opcode, uint8_t opcode_byte);

	void test_r8_rm8(Opcode opcode, uint8_t opcode_byte);

	void test_r16_rm16(Opcode opcode, uint8_t opcode_byte);

	void test_r32_rm32(Opcode opcode, uint8_t opcode_byte);

	void test_r64_rm64(Opcode opcode, uint8_t opcode_byte);
}
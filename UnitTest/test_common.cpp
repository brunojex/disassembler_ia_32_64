#include "stdafx.h"
#include "test_common.h"
#include "Decoder.h"
#include "decoder_def.h"

#include <set>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace diasm;

namespace UnitTest
{
	using Mode = Decoder::Operation_mode;

	namespace 
	{
		struct Addressing_line_16bit
		{
			uint8_t mod;
			uint8_t rm;
			Register r1, r2;
			Operand_format format;
			Bit_size displacement_size = Bit_size::_0;
			int32_t displacement = 0;
		};

		static const constexpr Addressing_line_16bit addressing_line_16bit[] =
		{
			{ 0, 0, Register::bx, Register::si , Operand_format::addr_of_reg1_reg2 },
			{ 0, 1, Register::bx, Register::di , Operand_format::addr_of_reg1_reg2 },
			{ 0, 2, Register::bp, Register::si , Operand_format::addr_of_reg1_reg2 },
			{ 0, 3, Register::bp, Register::di , Operand_format::addr_of_reg1_reg2 },
			{ 0, 4, Register::si, Register::invalid , Operand_format::addr_of_reg1 },
			{ 0, 5, Register::di, Register::invalid , Operand_format::addr_of_reg1 },
			{ 0, 6, Register::invalid, Register::invalid , Operand_format::displacement, Bit_size::_16, 0xFC33 },
			{ 0, 7, Register::bx, Register::invalid , Operand_format::addr_of_reg1 },

			{ 1, 0, Register::bx, Register::si, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_8 , 0xF5 },
			{ 1, 1, Register::bx, Register::di, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_8 , 0xF5 },
			{ 1, 2, Register::bp, Register::si, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_8 , 0xF5 },
			{ 1, 3, Register::bp, Register::di, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_8 , 0xF5 },
			{ 1, 4, Register::si, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 5, Register::di, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 6, Register::bp, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 7, Register::bx, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },

			{ 2, 0, Register::bx, Register::si, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_16 , 0xFC33 },
			{ 2, 1, Register::bx, Register::di, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_16 , 0xFC33 },
			{ 2, 2, Register::bp, Register::si, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_16 , 0xFC33 },
			{ 2, 3, Register::bp, Register::di, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_16 , 0xFC33 },
			{ 2, 4, Register::si, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_16 , 0xFC33 },
			{ 2, 5, Register::di, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_16 , 0xFC33 },
			{ 2, 6, Register::bp, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_16 , 0xFC33 },
			{ 2, 7, Register::bx, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_16 , 0xFC33 },
		};

		struct Addressing_line_32bit
		{
			uint8_t mod;
			uint8_t rm;
			Register r1;
			Operand_format format;
			Bit_size displacement_size = Bit_size::_0;
			int32_t displacement = 0;
		};

		static const constexpr Addressing_line_32bit addressing_line_32bit[] =
		{
			{ 0, 0, Register::eax, Operand_format::addr_of_reg1 },
			{ 0, 1, Register::ecx, Operand_format::addr_of_reg1 },
			{ 0, 2, Register::edx, Operand_format::addr_of_reg1 },
			{ 0, 3, Register::ebx, Operand_format::addr_of_reg1 },
			{ 0, 5, Register::invalid, Operand_format::displacement, Bit_size::_32, 0x0C33FC33 },
			{ 0, 6, Register::esi, Operand_format::addr_of_reg1 },
			{ 0, 7, Register::edi, Operand_format::addr_of_reg1 },

			{ 1, 0, Register::eax, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 1, Register::ecx, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 2, Register::edx, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 3, Register::ebx, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 5, Register::ebp, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 6, Register::esi, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },
			{ 1, 7, Register::edi, Operand_format::addr_of_reg1_disp, Bit_size::_8 , 0xF5 },

			{ 2, 0, Register::eax, Operand_format::addr_of_reg1_disp, Bit_size::_32 , 0x0C33FC33 },
			{ 2, 1, Register::ecx, Operand_format::addr_of_reg1_disp, Bit_size::_32 , 0x0C33FC33 },
			{ 2, 2, Register::edx, Operand_format::addr_of_reg1_disp, Bit_size::_32 , 0x0C33FC33 },
			{ 2, 3, Register::ebx, Operand_format::addr_of_reg1_disp, Bit_size::_32 , 0x0C33FC33 },
			{ 2, 5, Register::ebp, Operand_format::addr_of_reg1_disp, Bit_size::_32 , 0x0C33FC33 },
			{ 2, 6, Register::esi, Operand_format::addr_of_reg1_disp, Bit_size::_32 , 0x0C33FC33 },
			{ 2, 7, Register::edi, Operand_format::addr_of_reg1_disp, Bit_size::_32 , 0x0C33FC33 },
		};

		struct Addressing_line_SIB_32bit
		{
			uint8_t ss;
			uint8_t index;
			Register r1;
			Operand_format format;
		};

		static const constexpr Addressing_line_SIB_32bit addressing_line_SIB_32bit[] =
		{
			{ 0, 0, Register::eax, Operand_format::sib_scaled },
			{ 0, 1, Register::ecx, Operand_format::sib_scaled },
			{ 0, 2, Register::edx, Operand_format::sib_scaled },
			{ 0, 3, Register::ebx, Operand_format::sib_scaled },
			{ 0, 5, Register::ebp, Operand_format::sib_scaled },
			{ 0, 6, Register::esi, Operand_format::sib_scaled },
			{ 0, 7, Register::edi, Operand_format::sib_scaled },

			{ 1, 0, Register::eax, Operand_format::sib_scaled2 },
			{ 1, 1, Register::ecx, Operand_format::sib_scaled2 },
			{ 1, 2, Register::edx, Operand_format::sib_scaled2 },
			{ 1, 3, Register::ebx, Operand_format::sib_scaled2 },
			{ 1, 5, Register::ebp, Operand_format::sib_scaled2 },
			{ 1, 6, Register::esi, Operand_format::sib_scaled2 },
			{ 1, 7, Register::edi, Operand_format::sib_scaled2 },

			{ 2, 0, Register::eax, Operand_format::sib_scaled4 },
			{ 2, 1, Register::ecx, Operand_format::sib_scaled4 },
			{ 2, 2, Register::edx, Operand_format::sib_scaled4 },
			{ 2, 3, Register::ebx, Operand_format::sib_scaled4 },
			{ 2, 5, Register::ebp, Operand_format::sib_scaled4 },
			{ 2, 6, Register::esi, Operand_format::sib_scaled4 },
			{ 2, 7, Register::edi, Operand_format::sib_scaled4 },

			{ 3, 0, Register::eax, Operand_format::sib_scaled8 },
			{ 3, 1, Register::ecx, Operand_format::sib_scaled8 },
			{ 3, 2, Register::edx, Operand_format::sib_scaled8 },
			{ 3, 3, Register::ebx, Operand_format::sib_scaled8 },
			{ 3, 5, Register::ebp, Operand_format::sib_scaled8 },
			{ 3, 6, Register::esi, Operand_format::sib_scaled8 },
			{ 3, 7, Register::edi, Operand_format::sib_scaled8 },
		};

		struct Addressing_column_SIB_32bit
		{
			uint8_t base;
			Register r1;
			bool has_displacement = false;
		};

		static const constexpr Addressing_column_SIB_32bit addressing_column_SIB_32bit[] =
		{
			{ 0, Register::eax },{ 1, Register::ecx },{ 2, Register::edx },{ 3, Register::ebx },
			{ 4, Register::esp },{ 5, Register::ebp, true },{ 6, Register::esi },{ 7, Register::edi }
		};

		struct Validator
		{
			Validator(const std::set<Decoder::Operation_mode>& modes, const std::vector<uint8_t>& data, int count = 0)
				:len(data.size()), count(count)
			{
				Assert::IsTrue(!modes.empty(), std::wstring(L"No operation mode specified").append(std::to_wstring(count)).c_str());
				Assert::IsTrue(modes.size() < 4, std::wstring(L"Too many operation modes specified").append(std::to_wstring(count)).c_str());

				for (const auto& mode : modes)
					instructions.emplace_back(Decoder(mode).decode(data.data()), mode);
			}

			~Validator()
			{
				for (const auto& val : instructions)
				{
					auto instruction = val.first;
					auto local_count = count;

					auto extra_info = val.second == Decoder::Operation_mode::_16bit ?
						std::wstring(L" (16 bit mode) validator count =").append(std::to_wstring(count)) :
						val.second == Decoder::Operation_mode::_32bit ?
						std::wstring(L" (32 bit mode) validator count =").append(std::to_wstring(count)) :
						std::wstring(L" (64 bit mode) validator count =").append(std::to_wstring(count));

					Assert::IsTrue(!instruction.empty(), std::wstring(L"Instruction is empty").append(extra_info).c_str());
					Assert::AreEqual(instruction.immediate, immediate, std::wstring(L"Immediate value mismatch").append(extra_info).c_str());
					Assert::AreEqual(instruction.displacement, displacement, std::wstring(L"Displacement value mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.immediate_size == immediate_size, std::wstring(L"Immediate size mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.displacement_size == displacement_size, std::wstring(L"Displacement size mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.prefix == prefix, std::wstring(L"Prefix mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.opcode == opcode, std::wstring(L"Opcode mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.register_[0] == register1, std::wstring(L"register1 mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.register_[1] == register2, std::wstring(L"register2 mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.register_[2] == register3, std::wstring(L"register3 mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.operand1 == operand1, std::wstring(L"operand1 mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.operand2 == operand2, std::wstring(L"operand2 mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.operand3 == operand3, std::wstring(L"operand3 mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.operand4 == operand4, std::wstring(L"operand4 mismatch").append(extra_info).c_str());
					Assert::IsTrue(instruction.original_size == len, std::wstring(L"instruction length mismatch").append(extra_info).c_str());
				}
			}

			Validator(Validator&&) = delete;
			Validator(const Validator&) = delete;
			Validator& operator=(const Validator&) = delete;
			Validator& operator=(Validator&&) = delete;

			int32_t immediate = 0;
			int32_t displacement = 0;
			Bit_size immediate_size = Bit_size::_0;
			Bit_size displacement_size = Bit_size::_0;
			Custom_prefix prefix = Custom_prefix::invalid;
			Opcode opcode = Opcode::invalid;
			Register register1 = Register::invalid;
			Register register2 = Register::invalid;
			Register register3 = Register::invalid;
			Operand_format operand1 = Operand_format::invalid;
			Operand_format operand2 = Operand_format::invalid;
			Operand_format operand3 = Operand_format::invalid;
			Operand_format operand4 = Operand_format::invalid;
		private:
			int len;
			int count;
			std::vector<std::pair<Instruction, Decoder::Operation_mode>> instructions;
		};

		enum class Bit_mode
		{
			_8 = 0, _16, _32, _64
		};

		void test_extended_rXX_imm8(Bit_mode reg_type, Opcode opcode, uint8_t opcode_byte, uint8_t reg_field, int& validator_count)
		{
			static constexpr const Register registers[4][8] =
			{
				{ Register::al, Register::cl, Register::dl, Register::bl, Register::ah, Register::ch, Register::dh, Register::bh },
				{ Register::ax, Register::cx, Register::dx, Register::bx, Register::sp, Register::bp, Register::si, Register::di },
				{ Register::eax, Register::ecx, Register::edx, Register::ebx, Register::esp, Register::ebp, Register::esi, Register::edi },
				{ Register::rax, Register::rcx, Register::rdx, Register::rbx, Register::rsp, Register::rbp, Register::rsi, Register::rdi }
			};

			int reg_index = static_cast<int>(reg_type);

			const std::set<Decoder::Operation_mode> modes = reg_index == 0 ? decltype(modes){ Mode::_16bit, Mode::_32bit, Mode::_64bit } :
				                                            reg_index == 1 ? decltype(modes){ Mode::_16bit } :
				                                            reg_index == 2 ? decltype(modes){ Mode::_32bit } : 
				                                            decltype(modes){Mode::_64bit};

			static const constexpr uint8_t immediate_8 = 0xF0;
			uint8_t mod_rm = 0xC0 | (reg_field << 3);

			for (auto reg : registers[reg_index])
			{
				Validator val(modes, { opcode_byte, mod_rm++, immediate_8 }, validator_count++);
				val.opcode = opcode;
				val.operand1 = Operand_format::register_;
				val.operand2 = Operand_format::immediate;
				val.register1 = reg;
				val.immediate_size = Bit_size::_8;
				val.immediate = static_cast<int8_t>(immediate_8);
			}	
		}
	}

	void test_AL_imm8(Opcode opcode, const std::vector<uint8_t>& data)
	{
		Validator val({ Mode::_16bit, Mode::_32bit, Mode::_64bit }, data);
		val.opcode = opcode;
		val.operand1 = Operand_format::register_;
		val.operand2 = Operand_format::immediate;
		val.register1 = Register::al;
		val.immediate_size = Bit_size::_8;
		val.immediate = reinterpret_cast<const int8_t&>(data[data.size() - sizeof(int8_t)]);
	}

	void test_AX_imm16(Opcode opcode, const std::vector<uint8_t>& data)
	{
		Validator val({ Mode::_16bit }, data);
		val.opcode = opcode;
		val.operand1 = Operand_format::register_;
		val.operand2 = Operand_format::immediate;
		val.register1 = Register::ax;
		val.immediate_size = Bit_size::_16;
		val.immediate = reinterpret_cast<const int16_t&>(data[data.size() - sizeof(int16_t)]);
	}

	void test_EAX_imm32(Opcode opcode, const std::vector<uint8_t>& data)
	{
		Validator val({ Mode::_32bit, Mode::_64bit }, data);
		val.opcode = opcode;
		val.operand1 = Operand_format::register_;
		val.operand2 = Operand_format::immediate;
		val.register1 = Register::eax;
		val.immediate_size = Bit_size::_32;
		val.immediate = reinterpret_cast<const int32_t&>(data[data.size() - sizeof(int32_t)]);
	}

	void test_RAX_imm32(Opcode opcode, const std::vector<uint8_t>& data)
	{
		Validator val({ Mode::_64bit }, data);
		val.opcode = opcode;
		val.operand1 = Operand_format::register_;
		val.operand2 = Operand_format::immediate;
		val.register1 = Register::rax;
		val.immediate_size = Bit_size::_32;
		val.immediate = reinterpret_cast<const int32_t&>(data[data.size() - sizeof(int32_t)]);
	}

	void test_extended_rm8_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
		int validator_count = 0;
		const uint8_t immediate_8 = 0xF0;

		test_extended_rXX_imm8(Bit_mode::_8, opcode, opcode_byte, reg_field, validator_count);
	
		//test 16bit addressing
		{
			for (const auto& entry : addressing_line_16bit)
			{
				const uint8_t mod_rm = (entry.mod << 6) | (reg_field << 3) | entry.rm;

				std::vector<uint8_t> data = entry.displacement_size == Bit_size::_0 ?
					std::vector<uint8_t>{ opcode_byte, mod_rm, immediate_8 } : entry.displacement_size == Bit_size::_8 ?
					std::vector<uint8_t>{ opcode_byte, mod_rm, (uint8_t)(entry.displacement), immediate_8 } :
					std::vector<uint8_t>{ opcode_byte, mod_rm, ((uint8_t*)(&entry.displacement))[0], ((uint8_t*)(&entry.displacement))[1], immediate_8 };

				Validator val({ Mode::_16bit }, data, validator_count++);

				val.opcode = opcode;
				val.operand1 = entry.format;
				val.operand2 = Operand_format::immediate;
				val.register1 = entry.r1;
				val.register2 = entry.r2;
				val.displacement_size = entry.displacement_size;
				val.displacement = entry.displacement_size == Bit_size::_0 ? val.displacement
					: entry.displacement_size == Bit_size::_8 ? static_cast<int8_t>((uint8_t)entry.displacement)
					: *reinterpret_cast<int16_t*>((uint16_t*)(&entry.displacement));
				val.immediate_size = Bit_size::_8;
				val.immediate = static_cast<int8_t>(immediate_8);
			}
		}

		//test 32bit addressing
		{
			for (const auto& entry : addressing_line_32bit)
			{
				const uint8_t mod_rm = (entry.mod << 6) | (reg_field << 3) | entry.rm;

				std::vector<uint8_t> data = entry.displacement_size == Bit_size::_0 ?
					std::vector<uint8_t>{ opcode_byte, mod_rm, immediate_8 } : entry.displacement_size == Bit_size::_8 ?
					std::vector<uint8_t>{ opcode_byte, mod_rm, (uint8_t)(entry.displacement), immediate_8 } :
					std::vector<uint8_t>{ opcode_byte, mod_rm, ((uint8_t*)(&entry.displacement))[0], ((uint8_t*)(&entry.displacement))[1],
					((uint8_t*)(&entry.displacement))[2], ((uint8_t*)(&entry.displacement))[3], immediate_8 };

				Validator val({ Mode::_32bit ,  Mode::_64bit }, data, validator_count++);

				val.opcode = opcode;
				val.operand1 = entry.format;
				val.operand2 = Operand_format::immediate;
				val.register1 = entry.r1;
				val.displacement_size = entry.displacement_size;
				val.displacement = entry.displacement_size == Bit_size::_0 ? val.displacement
					: entry.displacement_size == Bit_size::_8 ? static_cast<int8_t>((uint8_t)entry.displacement)
					: *reinterpret_cast<int32_t*>((uint32_t*)(&entry.displacement));
				val.immediate_size = Bit_size::_8;
				val.immediate = static_cast<int8_t>(immediate_8);
			}
		}

		//test 32bit addressing (sib table)
		{
			for (uint8_t mod = 0; mod < 3; mod++)
			{
				const uint8_t mod_rm = (mod << 6) | (reg_field << 3) | 0x04;

				for (const auto& line_entry : addressing_line_SIB_32bit)
				{
					for (const auto& column_entry : addressing_column_SIB_32bit)
					{
						const uint8_t sib_byte = (line_entry.ss << 6) | (line_entry.index << 3) | column_entry.base;

						std::vector<uint8_t> data = !column_entry.has_displacement ?
							std::vector<uint8_t>{ opcode_byte, mod_rm, sib_byte, immediate_8 } : mod == 1 ?
							std::vector<uint8_t>{ opcode_byte, mod_rm, sib_byte, 0xF5, immediate_8 } :
							std::vector<uint8_t>{ opcode_byte, mod_rm, sib_byte, 0xFD, 0x57, 0xE0, 0x59, immediate_8 };

						Validator val({ Mode::_32bit ,  Mode::_64bit }, data, validator_count++);

						val.opcode = opcode;
						val.operand1 = line_entry.format;
						val.operand2 = Operand_format::immediate;
						val.register1 = line_entry.r1;
						val.register2 = (mod || column_entry.base != 5) ? column_entry.r1 : Register::invalid;
						if (column_entry.has_displacement)
						{
							if (mod == 1)
							{
								val.displacement_size = Bit_size::_8;
								val.displacement = static_cast<int8_t>((uint8_t)0xF5);
							}
							else
							{
								val.displacement_size = Bit_size::_32;
								val.displacement = static_cast<int32_t>((uint32_t)0x59E057FD);
							}
						}
						val.immediate_size = Bit_size::_8;
						val.immediate = static_cast<int8_t>(immediate_8);
					}
				}
			}
		}
	}

	void test_extended_rm16_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{

	}

	void test_extended_rm32_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
	}

	void test_extended_rm64_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
	}

	void test_extended_rm16_imm16(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
	}

	void test_extended_rm32_imm32(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
	}

	void test_extended_rm64_imm32(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
	}

	void test_rm8_r8(Opcode opcode, uint8_t opcode_byte)
	{
	}

	void test_rm16_r16(Opcode opcode, uint8_t opcode_byte)
	{
	}

	void test_rm32_r32(Opcode opcode, uint8_t opcode_byte)
	{
	}

	void test_rm64_r64(Opcode opcode, uint8_t opcode_byte)
	{
	}

	void test_r8_rm8(Opcode opcode, uint8_t opcode_byte)
	{
	}

	void test_r16_rm16(Opcode opcode, uint8_t opcode_byte)
	{
	}

	void test_r32_rm32(Opcode opcode, uint8_t opcode_byte)
	{
	}

	void test_r64_rm64(Opcode opcode, uint8_t opcode_byte)
	{
	}

}
#include "stdafx.h"
#include "test_common.h"
#include "Decoder.h"
#include "decoder_def.h"

#include <set>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace diasm;

namespace UnitTest
{
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
			{ 0, 6, Register::invalid, Register::invalid , Operand_format::displacement, Bit_size::_16},
			{ 0, 7, Register::bx, Register::invalid , Operand_format::addr_of_reg1 },

			{ 1, 0, Register::bx, Register::si, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_8},
			{ 1, 1, Register::bx, Register::di, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_8},
			{ 1, 2, Register::bp, Register::si, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_8},
			{ 1, 3, Register::bp, Register::di, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_8},
			{ 1, 4, Register::si, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_8},
			{ 1, 5, Register::di, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_8},
			{ 1, 6, Register::bp, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_8},
			{ 1, 7, Register::bx, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_8},

			{ 2, 0, Register::bx, Register::si, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_16},
			{ 2, 1, Register::bx, Register::di, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_16},
			{ 2, 2, Register::bp, Register::si, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_16},
			{ 2, 3, Register::bp, Register::di, Operand_format::addr_of_reg1_reg2_disp, Bit_size::_16},
			{ 2, 4, Register::si, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_16},
			{ 2, 5, Register::di, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_16},
			{ 2, 6, Register::bp, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_16},
			{ 2, 7, Register::bx, Register::invalid, Operand_format::addr_of_reg1_disp, Bit_size::_16},
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

		enum class Addressing_mode : uint8_t
		{
			_16,
			not16,
			any,
		};

		enum class Operand_size : uint8_t
		{
			_16,
			not16,
			any,
		};

		enum class Operation_mode : uint8_t
		{
			_32,
			_64,
			any
		};

		struct Validator
		{
			Validator(Operation_mode operation_mode, Operand_size operand_size, Addressing_mode addr_mode, const std::vector<uint8_t>& data, int count = 0)
				:len(data.size()), count(count)
			{
				instructions.reserve(8);

				using Mode = Decoder::Operation_mode;
				
				if (operation_mode != Operation_mode::_32)
				{
					if (operand_size != Operand_size::_16)
					{
						if (addr_mode != Addressing_mode::_16)
							add_instruction({ Mode::_64bit, false, false, 0}, data);
						if (addr_mode != Addressing_mode::not16)
							add_instruction({ Mode::_64bit, true, false, 1}, data);
					}

					if (operand_size != Operand_size::not16)
					{
						if (addr_mode != Addressing_mode::_16)
							add_instruction({ Mode::_64bit, false, true, 2}, data);
						if (addr_mode != Addressing_mode::not16)
							add_instruction({ Mode::_64bit, true, true, 3}, data);
					}
				}

				if (operation_mode != Operation_mode::_64)
				{
					if (operand_size != Operand_size::_16)
					{
						if (addr_mode != Addressing_mode::_16)
							add_instruction({ Mode::_32bit, false, false, 4}, data);
						if (addr_mode != Addressing_mode::not16)
							add_instruction({ Mode::_32bit, true, false, 5}, data);
					}

					if (operand_size != Operand_size::not16)
					{
						if (addr_mode != Addressing_mode::_16)
							add_instruction({ Mode::_32bit, false, true, 6}, data);
						if (addr_mode != Addressing_mode::not16)
							add_instruction({ Mode::_32bit, true, true, 7}, data);
					}
				}
			}

			~Validator()
			{
				for (const auto& val : instructions)
				{
					auto instruction = val.first;
					auto local_count = count;

					const char* const base_msg_ptr = base_messages[val.second.base_message_index];
					auto extra_info = std::wstring(base_msg_ptr, base_msg_ptr + 56).append(std::to_wstring(count));

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
			static const constexpr char* const base_messages[8] =
			{
				" mode=64bit, op_size_16=0, addr_16=0, validator count = ",
				" mode=64bit, op_size_16=1, addr_16=0, validator count = ",
				" mode=64bit, op_size_16=0, addr_16=1, validator count = ",
				" mode=64bit, op_size_16=1, addr_16=1, validator count = ",
				" mode=32bit, op_size_16=0, addr_16=0, validator count = ",
				" mode=32bit, op_size_16=1, addr_16=0, validator count = ",
				" mode=32bit, op_size_16=0, addr_16=1, validator count = ",
				" mode=32bit, op_size_16=1, addr_16=1, validator count = "
			};

			struct Decoder_config
			{
				Decoder::Operation_mode operation_mode;
				bool is_default_operand_size_16;
				bool is_addressing_mode_16;
				uint8_t base_message_index;
			};

			void add_instruction(Decoder_config config, const std::vector<uint8_t>& data)
			{
				instructions.emplace_back(
					Decoder(config.operation_mode, config.is_default_operand_size_16, config.is_addressing_mode_16).decode(data.data()), config);
			}

			int len;
			int count;
			std::vector<std::pair<Instruction, Decoder_config>> instructions;
		};

		enum class Bit_mode
		{
			_8 = 0, _16, _32, _64
		};

		void test_extended_mXX_immX(const Opcode opcode, const Bit_size imm_size, 
			const Operand_size operand_size, const uint8_t opcode_byte, const uint8_t reg_field, int& validator_count)
		{
			//test 16bit addressing
			for (const auto& entry : addressing_line_16bit)
			{
				const uint8_t mod_rm = (entry.mod << 6) | (reg_field << 3) | entry.rm;

				int32_t immediate;
				int32_t displacement;
				std::vector<uint8_t> data;

				switch (entry.displacement_size)
				{
				case Bit_size::_0:
					switch (imm_size)
					{
					case Bit_size::_8:
						data = { opcode_byte, mod_rm, 0xF0 };
						immediate = static_cast<int8_t>((uint8_t)0xF0);

						break;
					case Bit_size::_16:
						data = { opcode_byte, mod_rm, 0xF5, 0x66 };
						immediate = static_cast<int32_t>((uint16_t)0x66F5);
						break;
					default:
						data = { opcode_byte, mod_rm, 0xF5, 0x66 , 0x55, 0x69};
						immediate = static_cast<int32_t>((uint32_t)0x695566F5);
					}
					displacement = 0;
					break;
				case Bit_size::_8:
					switch (imm_size)
					{
					case Bit_size::_8:
						data = { opcode_byte, mod_rm, 0xF8, 0xF0 };
						immediate = static_cast<int8_t>((uint8_t)0xF0);
						break;
					case Bit_size::_16:
						data = { opcode_byte, mod_rm, 0xF8, 0xF5, 0x66 };
						immediate = static_cast<int16_t>((uint16_t)0x66F5);
						break;
					default:
						data = { opcode_byte, mod_rm, 0xF8, 0xF5, 0x66 , 0x55, 0x69 };
						immediate = static_cast<int32_t>((uint32_t)0x695566F5);
					}
					displacement = static_cast<int8_t>((uint8_t)0xF8);
					break;
				default:
					switch (imm_size)
					{
					case Bit_size::_8:
						data = { opcode_byte, mod_rm, 0xF8, 0x66, 0xF0 };
						immediate = static_cast<int8_t>((uint8_t)0xF0);
						break;
					case Bit_size::_16:
						data = { opcode_byte, mod_rm, 0xF8, 0x66, 0xF5, 0x66 };
						immediate = static_cast<int16_t>((uint16_t)0x66F5);
						break;
					default:
						data = { opcode_byte, mod_rm, 0xF8, 0x66, 0xF5, 0x66 , 0x55, 0x69 };
						immediate = static_cast<int32_t>((uint32_t)0x695566F5);
					}
					displacement = static_cast<int16_t>((uint16_t)0x66F8);
				}
				
				Validator val(Operation_mode::any, operand_size, Addressing_mode::_16, data, validator_count++);
				val.opcode = opcode;
				val.operand1 = entry.format;
				val.operand2 = Operand_format::immediate;
				val.register1 = entry.r1;
				val.register2 = entry.r2;
				val.displacement_size = entry.displacement_size;
				val.displacement = displacement;
				val.immediate_size = imm_size;
				val.immediate = immediate;
			}

			//test 32bit addressing (sib table)
			for (uint8_t mod = 0; mod < 3; mod++)
			{
				const uint8_t mod_rm = (mod << 6) | (reg_field << 3) | 0x04;

				for (const auto& line_entry : addressing_line_SIB_32bit)
				{
					for (const auto& column_entry : addressing_column_SIB_32bit)
					{
						const uint8_t sib_byte = (line_entry.ss << 6) | (line_entry.index << 3) | column_entry.base;

						std::vector<uint8_t> data;
						int32_t displacement;
						int32_t immediate;
						Bit_size displacement_size;

						if (!column_entry.has_displacement)
						{
							switch (imm_size)
							{
							case Bit_size::_8:
								data = { opcode_byte, mod_rm, sib_byte, 0xF0 };
								immediate = static_cast<int8_t>((uint8_t)0xF0);
								break;
							case Bit_size::_16:
								data = { opcode_byte, mod_rm, sib_byte, 0xF5, 0x66 };
								immediate = static_cast<int16_t>((uint16_t)0x66F5);
								break;
							default:
								data = { opcode_byte, mod_rm, sib_byte, 0xF5, 0x66 , 0x55, 0x69 };
								immediate = static_cast<int32_t>((uint32_t)0x695566F5);
							}
							displacement = 0;
							displacement_size = Bit_size::_0;
						}
						else if(mod == 1)
						{
							switch (imm_size)
							{
							case Bit_size::_8:
								data = { opcode_byte, mod_rm, sib_byte, 0xF3, 0xF0 };
								immediate = static_cast<int8_t>((uint8_t)0xF0);
								break;
							case Bit_size::_16:
								data = { opcode_byte, mod_rm, sib_byte, 0xF3, 0xF5, 0x66 };
								immediate = static_cast<int16_t>((uint16_t)0x66F5);
								break;
							default:
								data = { opcode_byte, mod_rm, sib_byte, 0xF3, 0xF5, 0x66 , 0x55, 0x69 };
								immediate = static_cast<int32_t>((uint32_t)0x695566F5);
							}
							displacement = static_cast<int8_t>((uint8_t)0xF3);
							displacement_size = Bit_size::_8;
						}
						else
						{
							switch(imm_size)
							{
							case Bit_size::_8:
								data = { opcode_byte, mod_rm, sib_byte, 0xFD, 0x57, 0xE0, 0x59, 0xF0 };
								immediate = static_cast<int8_t>((uint8_t)0xF0);
								break;
							case Bit_size::_16:
								data = { opcode_byte, mod_rm, sib_byte, 0xFD, 0x57, 0xE0, 0x59, 0xF5, 0x66 };
								immediate = static_cast<int16_t>((uint16_t)0x66F5);
								break;
							default:
								data = { opcode_byte, mod_rm, sib_byte, 0xFD, 0x57, 0xE0, 0x59, 0xF5, 0x66 , 0x55, 0x69 };
								immediate = static_cast<int32_t>((uint32_t)0x695566F5);
							}
							displacement = static_cast<int32_t>((uint32_t)0x59E057FD);
							displacement_size = Bit_size::_32;
						}

						Validator val(Operation_mode::any, operand_size, Addressing_mode::not16, data, validator_count++);
						val.opcode = opcode;
						val.operand1 = line_entry.format;
						val.operand2 = Operand_format::immediate;
						val.register1 = line_entry.r1;
						val.register2 = (mod || column_entry.base != 5) ? column_entry.r1 : Register::invalid;	
						val.displacement_size = displacement_size;
						val.displacement = displacement;
						val.immediate_size = imm_size;
						val.immediate = immediate;
					}
				}
			}

			//test 32bit addressing
			for (const auto& entry : addressing_line_32bit)
			{
				const uint8_t mod_rm = (entry.mod << 6) | (reg_field << 3) | entry.rm;

				std::vector<uint8_t> data;
				int32_t displacement;
				int32_t immediate;

				switch (entry.displacement_size)
				{
				case Bit_size::_0:
					switch (imm_size)
					{
					case Bit_size::_8:
						data = { opcode_byte, mod_rm, 0x59};
						immediate = static_cast<int8_t>((uint8_t)0x59);
						break;
					case Bit_size::_16:
						data = { opcode_byte, mod_rm, 0xF5, 0x66 };
						immediate = static_cast<int16_t>((uint16_t)0x66F5);
						break;
					default:
						data = { opcode_byte, mod_rm, 0xF5, 0x66 , 0x55, 0x69 };
						immediate = static_cast<int32_t>((uint32_t)0x695566F5);
					}
					displacement = 0;
					break;
				case Bit_size::_8:
					switch (imm_size)
					{
					case Bit_size::_8:
						data = { opcode_byte, mod_rm, 0xF7, 0x59 };
						immediate = static_cast<int8_t>((uint8_t)0x59);
						break;
					case Bit_size::_16:
						data = { opcode_byte, mod_rm, 0xF7, 0xF5, 0x66 };
						immediate = static_cast<int16_t>((uint16_t)0x66F5);
						break;
					default:
						data = { opcode_byte, mod_rm, 0xF7, 0xF5, 0x66 , 0x55, 0x69 };
						immediate = static_cast<int32_t>((uint32_t)0x695566F5);
					}
					displacement = static_cast<int8_t>((uint8_t)0xF7);
					break;
				default:
					switch (imm_size)
					{
					case Bit_size::_8:
						data = { opcode_byte, mod_rm, 0xF9, 0x56, 0xE0, 0x98, 0x59 };
						immediate = static_cast<int8_t>((uint8_t)0x59);
						break;
					case Bit_size::_16:
						data = { opcode_byte, mod_rm, 0xF9, 0x56, 0xE0, 0x98, 0xF5, 0x66 };
						immediate = static_cast<int16_t>((uint16_t)0x66F5);
						break;
					default:
						data = { opcode_byte, mod_rm, 0xF9, 0x56, 0xE0, 0x98, 0xF5, 0x66 , 0x55, 0x69 };
						immediate = static_cast<int32_t>((uint32_t)0x695566F5);
					}
					displacement = static_cast<int32_t>((uint32_t)0x98E056F9);
				}

				Validator val(Operation_mode::any, operand_size, Addressing_mode::not16, data, validator_count++);
				val.opcode = opcode;
				val.operand1 = entry.format;
				val.operand2 = Operand_format::immediate;
				val.register1 = entry.r1;
				val.displacement_size = entry.displacement_size;
				val.displacement = displacement;
				val.immediate_size = imm_size;
				val.immediate = immediate;
			}
		}

		void test_extended_rXX_immX(Bit_mode reg_type, Opcode opcode, Bit_size imm_size, uint8_t opcode_byte, uint8_t reg_field, int& validator_count)
		{
			Assert::IsTrue(imm_size != Bit_size::_0, L"Immediate size can't be 0");

			static constexpr const Register registers[4][8] =
			{
				{ Register::al, Register::cl, Register::dl, Register::bl, Register::ah, Register::ch, Register::dh, Register::bh },
				{ Register::ax, Register::cx, Register::dx, Register::bx, Register::sp, Register::bp, Register::si, Register::di },
				{ Register::eax, Register::ecx, Register::edx, Register::ebx, Register::esp, Register::ebp, Register::esi, Register::edi },
				{ Register::rax, Register::rcx, Register::rdx, Register::rbx, Register::rsp, Register::rbp, Register::rsi, Register::rdi }
			};

			int reg_index = static_cast<int>(reg_type);
			Operation_mode operation_mode;
			Operand_size operand_size;

			switch (reg_index)
			{
			case 0:
				operation_mode = Operation_mode::any;
				operand_size = Operand_size::any;
				break;
			case 1:
				operation_mode = Operation_mode::any;
				operand_size = Operand_size::_16;
				break;
			case 2:
				operation_mode = Operation_mode::_32;
				operand_size = Operand_size::not16;
				break;
			default:
				operation_mode = Operation_mode::_64;
				operand_size = Operand_size::not16;
				break;
			}

			uint8_t mod_rm = 0xC0 | (reg_field << 3);

			std::vector<uint8_t> data;
			int32_t immediate;

			switch (imm_size)
			{
			case diasm::Bit_size::_8:
				data = { opcode_byte, 0, 0xF0 };
				immediate = static_cast<int8_t>((uint8_t)0xF0);
				break;
			case diasm::Bit_size::_16:
				data = { opcode_byte, 0, 0xF0, 0x85 };
				immediate = static_cast<int16_t>((uint16_t)0x85F0);
				break;
			case diasm::Bit_size::_32:
				data = { opcode_byte, 0, 0xF0, 0x85, 0x54, 0x34 };
				immediate = static_cast<int32_t>((uint32_t)0x345485F0);
				break;
			}

			for (auto reg : registers[reg_index])
			{
				data[1] = mod_rm++;

				Validator val(operation_mode, operand_size, Addressing_mode::any, data, validator_count++);
				val.opcode = opcode;
				val.operand1 = Operand_format::register_;
				val.operand2 = Operand_format::immediate;
				val.register1 = reg;
				val.immediate_size = imm_size;
				val.immediate = immediate;
			}
		}
	}

	void test_AL_imm8(Opcode opcode, const std::vector<uint8_t>& data)
	{
		Validator val(Operation_mode::any, Operand_size::any, Addressing_mode::any, data);
		val.opcode = opcode;
		val.operand1 = Operand_format::register_;
		val.operand2 = Operand_format::immediate;
		val.register1 = Register::al;
		val.immediate_size = Bit_size::_8;
		val.immediate = reinterpret_cast<const int8_t&>(data[data.size() - sizeof(int8_t)]);
	}

	void test_AX_imm16(Opcode opcode, const std::vector<uint8_t>& data)
	{
		Validator val(Operation_mode::any, Operand_size::_16, Addressing_mode::any, data);
		val.opcode = opcode;
		val.operand1 = Operand_format::register_;
		val.operand2 = Operand_format::immediate;
		val.register1 = Register::ax;
		val.immediate_size = Bit_size::_16;
		val.immediate = reinterpret_cast<const int16_t&>(data[data.size() - sizeof(int16_t)]);
	}

	void test_EAX_imm32(Opcode opcode, const std::vector<uint8_t>& data)
	{
		Validator val(Operation_mode::any, Operand_size::not16, Addressing_mode::any, data);
		val.opcode = opcode;
		val.operand1 = Operand_format::register_;
		val.operand2 = Operand_format::immediate;
		val.register1 = Register::eax;
		val.immediate_size = Bit_size::_32;
		val.immediate = reinterpret_cast<const int32_t&>(data[data.size() - sizeof(int32_t)]);
	}

	void test_RAX_imm32(Opcode opcode, const std::vector<uint8_t>& data)
	{
		Validator val(Operation_mode::_64, Operand_size::not16, Addressing_mode::any, data);
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

		test_extended_rXX_immX(Bit_mode::_8, opcode, Bit_size::_8, opcode_byte, reg_field, validator_count);
		test_extended_mXX_immX(opcode, Bit_size::_8, Operand_size::any, opcode_byte, reg_field, validator_count);
	}

	void test_extended_rm16_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
		int validator_count = 0;

		test_extended_rXX_immX(Bit_mode::_16, opcode, Bit_size::_8, opcode_byte, reg_field, validator_count);
		test_extended_mXX_immX(opcode, Bit_size::_8, Operand_size::any, opcode_byte, reg_field, validator_count);
	}

	void test_extended_rm32_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
		int validator_count = 0;

		test_extended_rXX_immX(Bit_mode::_32, opcode, Bit_size::_8, opcode_byte, reg_field, validator_count);
		test_extended_mXX_immX(opcode, Bit_size::_8, Operand_size::any, opcode_byte, reg_field, validator_count);
	}

	void test_extended_rm64_imm8(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
		//TODO::need rex support
		Assert::Fail(L"Not yet implemented");
	}

	void test_extended_rm16_imm16(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
		int validator_count = 0;

		test_extended_rXX_immX(Bit_mode::_16, opcode, Bit_size::_16, opcode_byte, reg_field, validator_count);
		test_extended_mXX_immX(opcode, Bit_size::_16, Operand_size::_16, opcode_byte, reg_field, validator_count);
	}

	void test_extended_rm32_imm32(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
		int validator_count = 0;

		test_extended_rXX_immX(Bit_mode::_32, opcode, Bit_size::_32, opcode_byte, reg_field, validator_count);
		test_extended_mXX_immX(opcode, Bit_size::_32, Operand_size::not16, opcode_byte, reg_field, validator_count);
	}

	void test_extended_rm64_imm32(Opcode opcode, uint8_t opcode_byte, uint8_t reg_field)
	{
		//TODO::need rex support
		Assert::Fail(L"Not yet implemented");
	}

	void test_rm8_r8(Opcode opcode, uint8_t opcode_byte)
	{
		Assert::Fail(L"Not yet implemented");
	}

	void test_rm16_r16(Opcode opcode, uint8_t opcode_byte)
	{
		Assert::Fail(L"Not yet implemented");
	}

	void test_rm32_r32(Opcode opcode, uint8_t opcode_byte)
	{
		Assert::Fail(L"Not yet implemented");
	}

	void test_rm64_r64(Opcode opcode, uint8_t opcode_byte)
	{
		Assert::Fail(L"Not yet implemented");
	}

	void test_r8_rm8(Opcode opcode, uint8_t opcode_byte)
	{
		Assert::Fail(L"Not yet implemented");
	}

	void test_r16_rm16(Opcode opcode, uint8_t opcode_byte)
	{
		Assert::Fail(L"Not yet implemented");
	}

	void test_r32_rm32(Opcode opcode, uint8_t opcode_byte)
	{
		Assert::Fail(L"Not yet implemented");
	}

	void test_r64_rm64(Opcode opcode, uint8_t opcode_byte)
	{
		Assert::Fail(L"Not yet implemented");
	}
}
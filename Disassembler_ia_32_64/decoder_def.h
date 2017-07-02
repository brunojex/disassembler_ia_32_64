#pragma once

#include "common_def.h"

#include <cstdint>

namespace diasm 
{
	enum class Segment_prefix : uint8_t
	{
		invalid = 0,
		seg_equal_es,
		seg_equal_cs,
		seg_equal_ss,
		seg_equal_ds,
		seg_equal_fs,
		seg_equal_gs
	};

	enum class Prefix : uint8_t
	{
		invalid,
		xacquire,
		xrelease,
		lock,
		rep,
		repe,
		repene
	};

	enum class Rex_prefix : uint8_t
	{
		invalid = 0,
		rex = 0x40,
		rex_b,
		rex_x,
		rex_xb,
		rex_r,
		rex_rb,
		rex_rx,
		rex_rxb,
		rex_w,
		rex_wb,
		rex_wx,
		rex_wxb,
		rex_wr,
		rex_wrb,
		rex_wrx,
		rex_wrxb,
	};

	enum class Rex_field : uint8_t
	{
		b = static_cast<uint8_t>(Rex_prefix::rex_b),
		x = static_cast<uint8_t>(Rex_prefix::rex_x),
		r = static_cast<uint8_t>(Rex_prefix::rex_r),
		w = static_cast<uint8_t>(Rex_prefix::rex_w),
	};

	enum class Addressing_method : uint8_t
	{
		invalid = 0,
		direct_address,								   //Direct access: The instruction has no ModR/M byte; the address of the operand is encoded in the instruction. No base register, index register, or scaling factor can be applied(for example, far JMP(EA)).
		vex_selects_general_register,                  //The VEX.vvvv field of the VEX prefix selects a general purpose register.
		modRM_reg_field_selects_control_register,      //The reg field of the ModR/M byte selects a control register (for example, MOV(0F20, 0F22)).
		modRM_reg_field_selects_debug_register,        //The reg field of the ModR/M byte selects a debug register (for example, MOV(0F21, 0F23)).
		modRM_selects_general_register_or_memory,	   //The ModR/M byte follows the opcode and specifies the operand. The operand is either a general - purpose register or a memory address. If it is a memory address, the address is computed from a segment register and any of the following values : a base register, an index register, a scaling factor, a displacement.
		Eflags_or_Rflags_register,					   //EFLAGS/RFLAGS Register.
		modRM_reg_field_selects_general_register,      //The reg field of the ModR/M byte selects a general register (for example, AX(000)).
		vex_selects_128_XMM_or_256_YMM_register,       //The VEX.vvvv field of the VEX prefix selects a 128 - bit XMM register or a 256 - bit YMM register, determined by operand type. For legacy SSE encodings this operand does not exist, changing the instruction to destructive form.
		immediate_data,								   //Immediate data : the operand value is encoded in subsequent bytes of the instruction.
		adds_offset_to_intruction_pointer,			   //The instruction contains a relative offset to be added to the instruction pointer register (for example, JMP (0E9), LOOP).
		immediate_selects_128_XMM_or_256_YMM_register, //The upper 4 bits of the 8 - bit immediate selects a 128 - bit XMM register or a 256 - bit YMM register, determined by operand type. (the MSB is ignored in 32 - bit mode)
		modRM_may_only_refer_to_memory,				   //The ModR / M byte may refer only to memory(for example, BOUND, LES, LDS, LSS, LFS, LGS, CMPXCHG8B).
		modRM_R_M_selects_quadword_register,		   //The R / M field of the ModR / M byte selects a packed - quadword, MMX technology register.
		no_modRM,									   //The instruction has no ModR / M byte.The offset of the operand is coded as a word or double word (depending on address size attribute) in the instruction.No base register, index register, or scaling factor can be applied(for example, MOV(A0–A3)).
		modRM_reg_selects_quadword_register,		   //The reg field of the ModR / M byte selects a packed quadword MMX technology register.
		modRM_selects_MMX_register_or_memory,		   //A ModR / M byte follows the opcode and specifies the operand.The operand is either an MMX technology register or a memory address.If it is a memory address, the address is computed from a segment register and any of the following values : a base register, an index register, a scaling factor, and a displacement.
		modRM_R_M_may_only_refer_to_general_register,  //The R / M field of the ModR / M byte may refer only to a general register (for example, MOV(0F20 - 0F23)).
		modRM_reg_selects_segment_register,			   //The reg field of the ModR / M byte selects a segment register (for example, MOV(8C, 8E)).
		modRM_R_M_selects_128_XMM_or_256_YMM_register, //The R / M field of the ModR / M byte selects a 128 - bit XMM register or a 256 - bit YMM register, determined by operand type.
		modRM_reg_selects_128_XMM_or_256_YMM_register, //The reg field of the ModR / M byte selects a 128 - bit XMM register or a 256 - bit YMM register, determined by operand type.
		modRM_selects_128_XMM_or_256_YMM_register,     //A ModR/M byte follows the opcode and specifies the operand. The operand is either a 128-bit XMM register, a 256 - bit YMM register (determined by operand type), or a memory address.If it is a memory address, the address is computed from a segment register and any of the following values : a base register, an index register, a scaling factor, and a displacement.
		memory_addressed_by_DS,						   //Memory addressed by the DS : rSI register pair(for example, MOVS, CMPS, OUTS, or LODS).
		memory_addressed_by_ES,                        //Memory addressed by the ES : rDI register pair(for example, MOVS, CMPS, INS, STOS, or SCAS).
	};

	//Helper so we can go read the intel tables these match the Addressing_method enum
	enum class Addressing_method_intel_doc : uint8_t
	{
		invalid = static_cast<uint8_t>(Addressing_method::invalid),
		A = static_cast<uint8_t>(Addressing_method::direct_address),
		B = static_cast<uint8_t>(Addressing_method::vex_selects_general_register),
		C = static_cast<uint8_t>(Addressing_method::modRM_reg_field_selects_control_register),
		D = static_cast<uint8_t>(Addressing_method::modRM_reg_field_selects_debug_register),
		E = static_cast<uint8_t>(Addressing_method::modRM_selects_general_register_or_memory),
		F = static_cast<uint8_t>(Addressing_method::Eflags_or_Rflags_register),
		G = static_cast<uint8_t>(Addressing_method::modRM_reg_field_selects_general_register),
		H = static_cast<uint8_t>(Addressing_method::vex_selects_128_XMM_or_256_YMM_register),
		I = static_cast<uint8_t>(Addressing_method::immediate_data),
		J = static_cast<uint8_t>(Addressing_method::adds_offset_to_intruction_pointer),
		L = static_cast<uint8_t>(Addressing_method::immediate_selects_128_XMM_or_256_YMM_register),
		M = static_cast<uint8_t>(Addressing_method::modRM_may_only_refer_to_memory),
		N = static_cast<uint8_t>(Addressing_method::modRM_R_M_selects_quadword_register),
		O = static_cast<uint8_t>(Addressing_method::no_modRM),
		P = static_cast<uint8_t>(Addressing_method::modRM_reg_selects_quadword_register),
		Q = static_cast<uint8_t>(Addressing_method::modRM_selects_MMX_register_or_memory),
		R = static_cast<uint8_t>(Addressing_method::modRM_R_M_may_only_refer_to_general_register),
		S = static_cast<uint8_t>(Addressing_method::modRM_reg_selects_segment_register),
		U = static_cast<uint8_t>(Addressing_method::modRM_R_M_selects_128_XMM_or_256_YMM_register),
		V = static_cast<uint8_t>(Addressing_method::modRM_reg_selects_128_XMM_or_256_YMM_register),
		W = static_cast<uint8_t>(Addressing_method::modRM_selects_128_XMM_or_256_YMM_register),
		X = static_cast<uint8_t>(Addressing_method::memory_addressed_by_DS),
		Y = static_cast<uint8_t>(Addressing_method::memory_addressed_by_ES)
	};

	enum class Operand_type : uint8_t
	{
		invalid = 0,
		two_words_or_two_doublewords,      //Two one - word operands in memory or two double - word operands in memory, depending on operand - size attribute(used only by the BOUND instruction).
		byte,                              //Byte, regardless of operand - size attribute.
		byte_or_word,                      //Byte or word, depending on operand - size attribute.
		doubleword,                        //Doubleword, regardless of operand - size attribute.
		double_quadword,                   //Double - quadword, regardless of operand - size attribute.
		bit_pointer_32_or_48_or_80,        //32 - bit, 48 - bit, or 80 - bit pointer, depending on operand - size attribute.
		double_precision_128_or_256,       //128 - bit or 256 - bit packed double - precision floating - point data.
		quadword_MMX_register,             //Quadword MMX technology register (for example: mm0).
		single_precision_128_or_256,       //128 - bit or 256 - bit packed single - precision floating - point data.
		quadword,                          //Quadword, regardless of operand - size attribute.
		quad_quadword_256,                 //Quad - Quadword(256 - bits), regardless of operand - size attribute.
		pseudo_descriptor_6byte_or_10byte, //6 - byte or 10 - byte pseudo - descriptor.
		scalar_of_128_double_precision,    //Scalar element of a 128 - bit double - precision floating data.
		scalar_of_128_single_precision,    //Scalar element of a 128 - bit single - precision floating data.
		doubleword_integer_register,       //Doubleword integer register (for example: eax).
		word_or_doubleword_or_quadword,    //Word, doubleword or quadword(in 64 - bit mode), depending on operand - size attribute.
		word,                              //Word, regardless of operand - size attribute.
		double_quadword_or_quad_quadword,  //Double quadword or quad quadword based on the operand - size attribute.
		doubleword_or_quadword,            //Doubleword or quadword(in 64 - bit mode), depending on operand - size attribute.
		word_or_doubleword                 //Word for 16 - bit operand - size or doubleword for 32 or 64 - bit operand - size.
	};

	//Helper so we can go read the intel tables these match the Operand_type enum
	enum class Operand_type_intel_doc : uint8_t
	{
		invalid = static_cast<uint8_t>(Operand_type::invalid),
		a = static_cast<uint8_t>(Operand_type::two_words_or_two_doublewords),
		b = static_cast<uint8_t>(Operand_type::byte),
		c = static_cast<uint8_t>(Operand_type::byte_or_word),
		d = static_cast<uint8_t>(Operand_type::doubleword),
		dq = static_cast<uint8_t>(Operand_type::double_quadword),
		p = static_cast<uint8_t>(Operand_type::bit_pointer_32_or_48_or_80),
		pd = static_cast<uint8_t>(Operand_type::double_precision_128_or_256),
		pi = static_cast<uint8_t>(Operand_type::quadword_MMX_register),
		ps = static_cast<uint8_t>(Operand_type::single_precision_128_or_256),
		q = static_cast<uint8_t>(Operand_type::quadword),
		qq = static_cast<uint8_t>(Operand_type::quad_quadword_256),
		s = static_cast<uint8_t>(Operand_type::pseudo_descriptor_6byte_or_10byte),
		sd = static_cast<uint8_t>(Operand_type::scalar_of_128_double_precision),
		ss = static_cast<uint8_t>(Operand_type::scalar_of_128_single_precision),
		si = static_cast<uint8_t>(Operand_type::doubleword_integer_register),
		v = static_cast<uint8_t>(Operand_type::word_or_doubleword_or_quadword),
		w = static_cast<uint8_t>(Operand_type::word),
		x = static_cast<uint8_t>(Operand_type::double_quadword_or_quad_quadword),
		y = static_cast<uint8_t>(Operand_type::doubleword_or_quadword),
		z = static_cast<uint8_t>(Operand_type::word_or_doubleword)
	};

	enum class Register_code : uint8_t
	{
		invalid = 0,
		AL,  //The AL register specifically
		rAX, //The AX, EAX, or RAX register
		ES,  //The ES register specifically
		SS,  //The SS register specifically
		CS,  //The CS register specifically
		DS   //The DS register specifically
	};

	enum class Register_type
	{
		_64 = 0,
		_32,
		_16,
		_8,
		mm,
		xmm,
		ymm,
		r,
		rD,
		rW,
		rL
	};

	enum class Register_index
	{
		rAX_r8 = 0, rCX_r9, rDX_r10, rBX_r11, rSP_r12, rBP_r13, rSI_r14, rDI_r15,
		   eAX = 0,    eCX,     eDX,     eBX,     eSP,     eBP,     eSI,     eDI,
		AL_R8L = 0, CL_R9L, DL_R10L, BL_R11L, AH_R12L, CH_R13L, DH_R14L, BH_R15L
	};

	struct Addressing_form
	{
		constexpr Addressing_form(Operand_format type, Bit_size size = Bit_size::_0)
			: format(type), size(size) {}

		constexpr Addressing_form(Register register1)
			: format(Operand_format::addr_of_reg1), register1(register1) {}

		constexpr Addressing_form(Register register1, Bit_size size)
			: format(Operand_format::addr_of_reg1_disp), register1(register1), size(size) {}
		
		constexpr Addressing_form(Register register1, Register register2)
			: format(Operand_format::addr_of_reg1_reg2), register1(register1), register2(register2) {}

		constexpr Addressing_form(Register register1, Register register2, Bit_size size)
			: format(Operand_format::addr_of_reg1_reg2_disp), register1(register1), register2(register2), size(size) {}

		Operand_format format;
		Bit_size size = Bit_size::_0;
		Register register1 = Register::invalid;
		Register register2 = Register::invalid;
	};

	struct ModRM
	{
		uint8_t r_m : 3;
		uint8_t reg_opcode : 3;
		uint8_t mod : 2;
	};

	struct SIB
	{
		uint8_t base : 3;
		uint8_t index : 3;
		uint8_t scale : 2;
	};

	struct Operand //easier to manipulate than working with bitfields
	{
		Operand_format format = Operand_format::invalid;
		Register register1 = Register::invalid;
		Register register2 = Register::invalid;
	};

	struct Operand_info
	{
		Operand_info()
			: Operand_info(Register_code::invalid)
		{}

		Operand_info(Register_code register_code)
			: register_code(register_code)
			, addressing_method(Addressing_method::invalid)
			, type(Operand_type::invalid)
		{}

		Operand_info(Addressing_method addressing_method, Operand_type type)
			: register_code(Register_code::invalid)
			, addressing_method(addressing_method)
			, type(type)
		{}

		Operand_type type;
		Register_code register_code;
		Addressing_method addressing_method;
	};

	struct Opcode_map_entry
	{
		Opcode_map_entry() = default;

		Opcode_map_entry(Prefix prefix)
			:prefix(prefix) {}

		Opcode_map_entry(Opcode opcode, Operand_info op1 = {}, Operand_info op2 = {}, Operand_info op3 = {}, Operand_info op4 = {})
			:opcode(opcode), first_operand(op1), second_operand(op2), third_operand(op3), fourth_operand(op4) {}

		Rex_prefix rex_prefix = Rex_prefix::invalid;
		Prefix  prefix = Prefix::invalid;
		Opcode  opcode = Opcode::invalid;
		Operand_info first_operand = {};
		Operand_info second_operand = {};
		Operand_info third_operand = {};
		Operand_info fourth_operand = {};
	};
}
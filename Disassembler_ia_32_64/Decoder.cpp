#include "Decoder.h"
#include "decoder_def.h"

namespace
{
	using namespace diasm;
	using Operation_mode = Decoder::Operation_mode;

	//line can be indexed by R/M or Reg field
	static const constexpr Register register_table[8][11] = {
		{ { Register::rax },{ Register::eax },{ Register::ax },{ Register::al },{ Register::mm0 },{ Register::xmm0 },{ Register::ymm0 } ,{ Register::r8  } ,{ Register::r8D  } ,{ Register::r8W } , { Register::r8L  } },
		{ { Register::rcx },{ Register::ecx },{ Register::cx },{ Register::cl },{ Register::mm1 },{ Register::xmm1 },{ Register::ymm1 } ,{ Register::r9  } ,{ Register::r9D  } ,{ Register::r9W } , { Register::r9L  } },
		{ { Register::rdx },{ Register::edx },{ Register::dx },{ Register::dl },{ Register::mm2 },{ Register::xmm2 },{ Register::ymm2 } ,{ Register::r10 } ,{ Register::r10D } ,{ Register::r10W } ,{ Register::r10L } },
		{ { Register::rbx },{ Register::ebx },{ Register::bx },{ Register::bl },{ Register::mm3 },{ Register::xmm3 },{ Register::ymm3 } ,{ Register::r11 } ,{ Register::r11D } ,{ Register::r11W } ,{ Register::r11L } },
		{ { Register::rsp },{ Register::esp },{ Register::sp },{ Register::ah },{ Register::mm4 },{ Register::xmm4 },{ Register::ymm4 } ,{ Register::r12 } ,{ Register::r12D } ,{ Register::r12W } ,{ Register::r12L } },
		{ { Register::rbp },{ Register::ebp },{ Register::bp },{ Register::ch },{ Register::mm5 },{ Register::xmm5 },{ Register::ymm5 } ,{ Register::r13 } ,{ Register::r13D } ,{ Register::r13W } ,{ Register::r13L } },
		{ { Register::rsi },{ Register::esi },{ Register::si },{ Register::dh },{ Register::mm6 },{ Register::xmm6 },{ Register::ymm6 } ,{ Register::r14 } ,{ Register::r14D } ,{ Register::r14W } ,{ Register::r14L } },
		{ { Register::rdi },{ Register::edi },{ Register::di },{ Register::bh },{ Register::mm7 },{ Register::xmm7 },{ Register::ymm7 } ,{ Register::r15 } ,{ Register::r15D } ,{ Register::r15W } ,{ Register::r15L } },
	};

	static const constexpr Addressing_form mod_addressing_16[3][8] = {
	{
		{ Register::bx, Register::si },
		{ Register::bx, Register::di },
		{ Register::bp, Register::si },
		{ Register::bp, Register::di },
		{ Register::si },
		{ Register::di },
		{ Operand_format::displacement, Bit_size::_16 },
		{ Register::bx }
	},{
		{ Register::bx, Register::si, Bit_size::_8 },
		{ Register::bx, Register::di, Bit_size::_8 },
		{ Register::bp, Register::si, Bit_size::_8 },
		{ Register::bp, Register::di, Bit_size::_8 },
		{ Register::si, Bit_size::_8 },
		{ Register::di, Bit_size::_8 },
		{ Register::bp, Bit_size::_8 },
		{ Register::bx, Bit_size::_8 }
	},{ 
		{ Register::bx, Register::si, Bit_size::_16 },
		{ Register::bx, Register::di, Bit_size::_16 },
		{ Register::bp, Register::si, Bit_size::_16 },
		{ Register::bp, Register::di, Bit_size::_16 },
		{ Register::si, Bit_size::_16 },
		{ Register::di, Bit_size::_16 },
		{ Register::bp, Bit_size::_16 },
		{ Register::bx, Bit_size::_16 },
	}};

	static const constexpr Addressing_form mod_addressing_32[3][8] = {
	{
		{ Register::eax },
		{ Register::ecx },
		{ Register::edx },
		{ Register::ebx },
		{ Operand_format::sib_table_escape, Bit_size::_32 },
		{ Operand_format::displacement, Bit_size::_32 },
		{ Register::esi },
		{ Register::edi }
	},{ 
		{ Register::eax, Bit_size::_8 },
		{ Register::ecx, Bit_size::_8 },
		{ Register::edx, Bit_size::_8 },
		{ Register::ebx, Bit_size::_8 },
		{ Operand_format::sib_table_escape, Bit_size::_8 },
		{ Register::ebp, Bit_size::_8 },
		{ Register::esi, Bit_size::_8 },
		{ Register::edi, Bit_size::_8 }
	},{ 
		{ Register::eax, Bit_size::_32 },
		{ Register::ecx, Bit_size::_32 },
		{ Register::edx, Bit_size::_32 },
		{ Register::ebx, Bit_size::_32 },
		{ Operand_format::sib_table_escape, Bit_size::_32 },
		{ Register::ebp, Bit_size::_32 },
		{ Register::esi, Bit_size::_32 },
		{ Register::edi, Bit_size::_32 }
	}};

	struct Parse_context
	{
		Parse_context(const uint8_t* data, bool addressing_mode_16_bit, Operation_mode operation_mode, bool operand_size_16_bit)
			: data(data), addressing_mode_16_bit(addressing_mode_16_bit), 
			operation_mode(operation_mode), operand_size_16_bit(operand_size_16_bit){}

		Instruction instruction;
		const uint8_t* data;
		const Operation_mode operation_mode;
		const Segment_prefix segment_prefix = Segment_prefix::invalid;
		const Rex_prefix rex_prefix = Rex_prefix::invalid;
		const bool operand_size_override_prefix = false;
		const bool address_size_override_prefix = false;
		bool operand_size_16_bit;
		bool addressing_mode_16_bit;
	};

	using Opcode_table = void(*[16][16]) (Parse_context& context);

	constexpr inline void opcode_table_call(const Opcode_table& table, Parse_context& context, uint8_t value)
	{
		table[value >> 4][value & 0x0F](context);
	}

	Opcode grp1_lookup(uint8_t value)
	{
		static const constexpr Opcode op_table[8] = { Opcode::add, Opcode::or,  Opcode::adc, Opcode::sbb, 
			                                          Opcode::and, Opcode::sub, Opcode::xor, Opcode::cmp };
		return op_table[(value & 0x3F) >> 3];
	}

	Opcode grp2_lookup(uint8_t value)
	{
		static const constexpr Opcode op_table[8] = { Opcode::rol, Opcode::ror, Opcode::rcl,     Opcode::rcr,
													  Opcode::shl, Opcode::shr, Opcode::invalid, Opcode::sar };
		return op_table[(value & 0x1F) >> 2];
	}

#pragma region forward_dec
	void parse_Eb_Gb(Parse_context& context, Opcode opcode);
	void parse_Gb_Eb(Parse_context& context, Opcode opcode);
	void parse_Ev_Gv(Parse_context& context, Opcode opcode);
	void parse_Gv_Ev(Parse_context& context, Opcode opcode);
	void parse_AL_Ib(Parse_context& context, Opcode opcode);
	void parse_rAX_Iz(Parse_context& context, Opcode opcode);
	void parse_rex_prefix(Parse_context& context, Opcode opcode, Rex_prefix prefix, Register_index reg_index);
	void parse_register_intruction(Parse_context& context, Opcode opcode, Register reg);
	void parse_segment_prefix(Parse_context& context, Segment_prefix prefix);
	void parse_push_pop_d64_register(Parse_context& context, Opcode opcode, Register_index reg_index);
	void parse_opcode_only_intruction(Parse_context& context, Opcode opcode);
	void parse_immediate8_instruction(Parse_context& context, Opcode opcode);
	void parse_register_immediate8_intruction(Parse_context& context, Opcode opcode, Register_index reg_index);
	void parse_register_immediateZ_intruction(Parse_context& context, Opcode opcode, Register_index reg_index);
	void parse_prefix_address_size(Parse_context& context);
	void parse_prefix_operand_size(Parse_context& context);
	void parse_Eb_Ib(Parse_context& context, Opcode opcode);
	void parse_Ev_Iz(Parse_context& context, Opcode opcode);
	void parse_Ev_Ib(Parse_context& context, Opcode opcode);
#pragma endregion

	namespace opcode_table2 {
		//jumptable to the processing routine
		static constexpr Opcode_table table = {};
	}

	namespace opcode_table1 {

#pragma region opcode_table1_line0 //finished
		void add_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::add);
		};

		void add_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::add);
		};

		void add_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::add);
		};

		void add_Gv_Ev(Parse_context& context) 
		{
			parse_Gv_Ev(context, Opcode::add);
		};

		void add_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::add);
		};

		void add_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::add);
		};

		void push_es(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ? parse_register_intruction(context, Opcode::push, Register::es) : 0;
		};

		void pop_es(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ? parse_register_intruction(context, Opcode::pop, Register::es) : 0;
		};

		//////////////////////////////////////////////////////

		void or_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::or);
		};

		void or_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::or);
		};

		void or_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::or);
		};

		void or_Gv_Ev(Parse_context& context) 
		{
			parse_Gv_Ev(context, Opcode::or);
		};

		void or_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::or);
		};

		void or_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::or);
		};

		void push_cs(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ? parse_register_intruction(context, Opcode::push, Register::cs) : 0;
		};

		void escape_opcode_table2(Parse_context& context) 
		{
			opcode_table_call(opcode_table2::table, context, *++context.data);
		};
#pragma endregion 

#pragma region opcode_table1_line1 //finished
		void adc_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::adc);
		};

		void adc_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::adc);
		};

		void adc_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::adc);
		};

		void adc_Gv_Ev(Parse_context& context) 
		{
			parse_Gv_Ev(context, Opcode::adc);
		};

		void adc_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::adc);
		};

		void adc_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::adc);
		};

		void push_ss(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ? parse_register_intruction(context, Opcode::push, Register::ss) : 0;
		};

		void pop_ss(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ? parse_register_intruction(context, Opcode::pop, Register::ss) : 0;
		};

		//////////////////////////////////////////////////////

		void sbb_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::sbb);
		};

		void sbb_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::sbb);
		};

		void sbb_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::sbb);
		};

		void sbb_Gv_Ev(Parse_context& context) 
		{
			parse_Gv_Ev(context, Opcode::sbb);
		};

		void sbb_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::sbb);
		};

		void sbb_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::sbb);
		};

		void push_ds(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ? parse_register_intruction(context, Opcode::push, Register::ds) : 0;
		};

		void pop_ds(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ? parse_register_intruction(context, Opcode::pop, Register::ds) : 0;
		};
#pragma endregion

#pragma region opcode_table1_line2 //finished
		void and_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::and);
		};

		void and_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::and);
		};

		void and_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::and);
		};

		void and_Gv_Ev(Parse_context& context) 
		{
			parse_Gv_Ev(context, Opcode::and);
		};

		void and_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::and);
		};

		void and_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::and);
		};

		void prefix_seg_es(Parse_context& context) 
		{
			parse_segment_prefix(context, Segment_prefix::seg_equal_es);
		};

		void daa(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::daa);
		};

		//////////////////////////////////////////////////////

		void sub_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::sub);
		};

		void sub_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::sub);
		};

		void sub_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::sub);
		};

		void sub_Gv_Ev(Parse_context& context) 
		{
			parse_Gv_Ev(context, Opcode::sub);
		};

		void sub_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::sub);
		};

		void sub_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::sub);
		};

		void prefix_seg_cs(Parse_context& context) 
		{
			parse_segment_prefix(context, Segment_prefix::seg_equal_cs);
		};

		void das(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::das);
		};

#pragma endregion

#pragma region opcode_table1_line3 //finished
		void xor_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::xor);
		};

		void xor_Ev_Gv(Parse_context& context)
		{
			parse_Ev_Gv(context, Opcode::xor);
		};

		void xor_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::xor);
		};

		void xor_Gv_Ev(Parse_context& context) 
		{
			parse_Gv_Ev(context, Opcode::xor);
		};

		void xor_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::xor);
		};

		void xor_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::xor);
		};

		void prefix_seg_ss(Parse_context& context) 
		{
			parse_segment_prefix(context, Segment_prefix::seg_equal_ss);
		};

		void aaa(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::aaa);
		};

		//////////////////////////////////////////////////////

		void cmp_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::cmp);
		};

		void cmp_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::cmp);
		};

		void cmp_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::cmp);
		};

		void cmp_Gv_Ev(Parse_context& context) 
		{
			parse_Gv_Ev(context, Opcode::cmp);
		};

		void cmp_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::cmp);
		};

		void cmp_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::cmp);
		};

		void prefix_seg_ds(Parse_context& context) 
		{
			parse_segment_prefix(context, Segment_prefix::seg_equal_ds);
		};

		void aas(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::aas);
		};
#pragma endregion

#pragma region opcode_table1_line4 //finished
		void inc_eAX__REX(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::inc, Rex_prefix::rex, Register_index::eAX);
		};

		void inc_eCX__REX_b(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::inc, Rex_prefix::rex_b, Register_index::eCX);
		};

		void inc_eDX__REX_x(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::inc, Rex_prefix::rex_x, Register_index::eDX);
		};

		void inc_eBX__REX_xb(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::inc, Rex_prefix::rex_xb, Register_index::eBX);
		};

		void inc_eSP__REX_r(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::inc, Rex_prefix::rex_r, Register_index::eSP);
		};

		void inc_eBP__REX_rb(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::inc, Rex_prefix::rex_rb, Register_index::eBP);
		};

		void inc_eSI__REX_rx(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::inc, Rex_prefix::rex_rx, Register_index::eSI);
		};

		void inc_eDI__REX_rxb(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::inc, Rex_prefix::rex_rxb, Register_index::eDI);
		};

		//////////////////////////////////////////////////////

		void dec_eAX__REX_w(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::dec, Rex_prefix::rex_w, Register_index::eAX);
		};

		void dec_eCX__REX_wb(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::dec, Rex_prefix::rex_wb, Register_index::eCX);
		};

		void dec_eDX__REX_wx(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::dec, Rex_prefix::rex_wx, Register_index::eDX);
		};

		void dec_eBX__REX_wxb(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::dec, Rex_prefix::rex_wxb, Register_index::eBX);
		};

		void dec_eSP__REX_wr(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::dec, Rex_prefix::rex_wr, Register_index::eSP);
		};

		void dec_eBP__REX_wrb(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::dec, Rex_prefix::rex_wrb, Register_index::eBP);
		};

		void dec_eSI__REX_wrx(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::dec, Rex_prefix::rex_wrx, Register_index::eSI);
		};

		void dec_eDI__REX_wrxb(Parse_context& context) 
		{
			parse_rex_prefix(context, Opcode::dec, Rex_prefix::rex_wrxb, Register_index::eDI);
		};
#pragma endregion

#pragma region opcode_table1_line5 //finished
		void push_rAX_r8(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::push, Register_index::rAX_r8);
		};

		void push_rCX_r9(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::push, Register_index::rCX_r9);
		};

		void push_rDX_r10(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::push, Register_index::rDX_r10);
		};

		void push_rBX_r11(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::push, Register_index::rBX_r11);
		};

		void push_rSP_r12(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::push, Register_index::rSP_r12);
		};

		void push_rBP_r13(Parse_context& context)
		{
			parse_push_pop_d64_register(context, Opcode::push, Register_index::rBP_r13);
		};

		void push_rSI_r14(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::push, Register_index::rSI_r14);
		};

		void push_rDI_r15(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::push, Register_index::rDI_r15);
		};

		//////////////////////////////////////////////////////

		void pop_rAX_r8(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::pop, Register_index::rAX_r8);
		};

		void pop_rCX_r9(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::pop, Register_index::rCX_r9);
		};

		void pop_rDX_r10(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::pop, Register_index::rDX_r10);
		};

		void pop_rBX_r11(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::pop, Register_index::rBX_r11);
		};

		void pop_rSP_r12(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::pop, Register_index::rSP_r12);
		};

		void pop_rBP_r13(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::pop, Register_index::rBP_r13);
		};

		void pop_rSI_r14(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::pop, Register_index::rSI_r14);
		};

		void pop_rDI_r15(Parse_context& context) 
		{
			parse_push_pop_d64_register(context, Opcode::pop, Register_index::rDI_r15);
		};
#pragma endregion

#pragma region opcode_table1_line6
		void pusha_pushad(Parse_context& context)
		{
			context.operation_mode != Operation_mode::_64bit ? 
				parse_opcode_only_intruction(context, context.operand_size_16_bit ? Opcode::pusha : Opcode::pushad) : 0;
		};

		void popa_popad(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ?
				parse_opcode_only_intruction(context, context.operand_size_16_bit ? Opcode::popa : Opcode::popad) : 0;
		};

		void bound_Gv_Ma(Parse_context& context) {};
		void arpl_Ew_Gw__movsxd_Gv_Ev(Parse_context& context) {};

		void prefix_seg_fs(Parse_context& context) 
		{
			parse_segment_prefix(context, Segment_prefix::seg_equal_fs);
		};

		void prefix_seg_gs(Parse_context& context) 
		{
			parse_segment_prefix(context, Segment_prefix::seg_equal_gs);
		};

		void prefix_operand_size(Parse_context& context) 
		{
			parse_prefix_operand_size(context);
		};

		void prefix_address_size(Parse_context& context) 
		{
			parse_prefix_address_size(context);
		};

		//////////////////////////////////////////////////////

		void push_Iz(Parse_context& context) {};
		void imul_Gv_Ev_Iz(Parse_context& context) {};
		void push_Ib(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::push);
		};

		void imul_Gv_Ev_Ib(Parse_context& context) {};
		void ins_insb_Yb_DX(Parse_context& context) {};
		void ins_insw_insd_Yz_DX(Parse_context& context) {};
		void outs_outsb_DX_Xb(Parse_context& context) {};
		void outs_outsw_outsd_DX_Xz(Parse_context& context) {};
#pragma endregion

#pragma region opcode_table1_line7 //finished
		void jo(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jo);
		};

		void jno(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jno);
		};

		void jb(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jb);
		};

		void jnb(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jnb);
		};

		void je(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::je);
		};

		void jne(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jne);
		};

		void jbe(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jbe);
		};

		void jnbe(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jnbe);
		};

		//////////////////////////////////////////////////////

		void js(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::js);
		};

		void jns(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jns);
		};

		void jpe(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jpe);
		};

		void jpo(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jpo);
		};

		void jl(Parse_context& context)
		{
			parse_immediate8_instruction(context, Opcode::jl);
		};

		void jnl(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jnl);
		};

		void jle(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jle);
		};

		void jnle(Parse_context& context) 
		{
			parse_immediate8_instruction(context, Opcode::jnle);
		};
#pragma endregion

#pragma region opcode_table1_line8
		void grp1_immediate_Eb_Ib(Parse_context& context) 
		{
			parse_Eb_Ib(context, grp1_lookup(*(context.data + 1)));
		};

		void grp1_immediate_Ev_Iz(Parse_context& context) 
		{
			parse_Ev_Iz(context, grp1_lookup(*(context.data + 1)));
		};

		void grp1_immediate_Eb_Ib_legacy(Parse_context& context) 
		{
			context.operation_mode != Operation_mode::_64bit ? parse_Eb_Ib(context, grp1_lookup(*(context.data + 1))) : 0;
		};

		void grp1_immediate_Ev_Ib(Parse_context& context) 
		{
			parse_Ev_Ib(context, grp1_lookup(*(context.data + 1)));
		};

		void test_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::test);
		};

		void test_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::test);
		};

		void xchg_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::xchg);
		};

		void xchg_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::xchg);
		};

		//////////////////////////////////////////////////////

		void mov_Eb_Gb(Parse_context& context) 
		{
			parse_Eb_Gb(context, Opcode::mov);
		};

		void mov_Ev_Gv(Parse_context& context) 
		{
			parse_Ev_Gv(context, Opcode::mov);
		};

		void mov_Gb_Eb(Parse_context& context) 
		{
			parse_Gb_Eb(context, Opcode::mov);
		};

		void mov_Gv_Ev(Parse_context& context)
		{
			parse_Gv_Ev(context, Opcode::mov);
		};

		void mov_Ev_Sw(Parse_context& context) {};
		void lea_Gv_M(Parse_context& context) {};
		void mov_Sw_Ew(Parse_context& context) {};
		void grp1a_pop_Ev(Parse_context& context) {};
#pragma endregion
	
#pragma region opcode_table1_line9
		void nop_xchg_r8_rAX(Parse_context& context) {};
		void xchg_rCX_r9(Parse_context& context) {};
		void xchg_rDX_r10(Parse_context& context) {};
		void xchg_rBX_r11(Parse_context& context) {};
		void xchg_rSP_r12(Parse_context& context) {};
		void xchg_rBP_r13(Parse_context& context) {};
		void xchg_rSI_r14(Parse_context& context) {};
		void xchg_rDI_r15(Parse_context& context) {};

		//////////////////////////////////////////////////////

		void cbw_cwde_cdqe(Parse_context& context) {};
		void cwd_cdq_cqo(Parse_context& context) {};
		void call_Ap_far(Parse_context& context) {};
		void fwait_wait(Parse_context& context) {};
		void pushf_pushfd_pushfq_Fv(Parse_context& context) {};
		void popf_popfd_popfq_Fv(Parse_context& context) {};
		void sahf(Parse_context& context) {};
		void lahf(Parse_context& context) {};
#pragma endregion

#pragma region opcode_table1_lineA
		void mov_AL_Ob(Parse_context& context) {};
		void mov_rAX_Ov(Parse_context& context) {};
		void mov_Ob_AL(Parse_context& context) {};
		void mov_Ov_rAX(Parse_context& context) {};
		void movs_movb_Yb_Xb(Parse_context& context) {};
		void movs_movsw_movsd_movsq_Yv_Xv(Parse_context& context) {};
		void cmps_cmpsb_Xb_Yb(Parse_context& context) {};
		void cmps_cmpsw_cmpsd_Xv_Yv(Parse_context& context) {};

		//////////////////////////////////////////////////////

		void test_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::test);
		};

		void test_rAX_Iz(Parse_context& context) 
		{
			parse_rAX_Iz(context, Opcode::test);
		};

		void stos_stosb_Yb_AL(Parse_context& context) {};
		void stos_stosw_stosd_stosq_Yv_rAX(Parse_context& context) {};
		void lods_lodsb_AL_Xb(Parse_context& context) {};
		void lods_lodsw_lodsd_lodsq_rAX_Xv(Parse_context& context) {};
		void scas_scasb_AL_Yb(Parse_context& context) {};
		void scas_scasw_scasd_scasq_rAX_Yv(Parse_context& context) {};
#pragma endregion

#pragma region opcode_table1_lineB //finished
		void mov_AL_R8L_Ib(Parse_context& context) 
		{
			parse_register_immediate8_intruction(context, Opcode::mov, Register_index::AL_R8L);
		};

		void mov_CL_R9L_Ib(Parse_context& context) 
		{
			parse_register_immediate8_intruction(context, Opcode::mov, Register_index::CL_R9L);
		};

		void mov_DL_R10L_Ib(Parse_context& context) 
		{
			parse_register_immediate8_intruction(context, Opcode::mov, Register_index::DL_R10L);
		};

		void mov_BL_R11L_Ib(Parse_context& context) 
		{
			parse_register_immediate8_intruction(context, Opcode::mov, Register_index::BL_R11L);
		};

		void mov_AH_R12L_Ib(Parse_context& context)
		{
			parse_register_immediate8_intruction(context, Opcode::mov, Register_index::AH_R12L);
		};

		void mov_CH_R13L_Ib(Parse_context& context) 
		{
			parse_register_immediate8_intruction(context, Opcode::mov, Register_index::CH_R13L);
		};

		void mov_DH_R14L_Ib(Parse_context& context) 
		{
			parse_register_immediate8_intruction(context, Opcode::mov, Register_index::DH_R14L);
		};

		void mov_BH_R15L_Ib(Parse_context& context) 
		{
			parse_register_immediate8_intruction(context, Opcode::mov, Register_index::BH_R15L);
		};

		//////////////////////////////////////////////////////

		void mov_rAX_r8_Iv(Parse_context& context) 
		{
			parse_register_immediateZ_intruction(context, Opcode::mov, Register_index::rAX_r8);
		};

		void mov_rCX_r9_Iv(Parse_context& context) 
		{
			parse_register_immediateZ_intruction(context, Opcode::mov, Register_index::rCX_r9);
		};

		void mov_rDX_r10_Iv(Parse_context& context) 
		{
			parse_register_immediateZ_intruction(context, Opcode::mov, Register_index::rDX_r10);
		};

		void mov_rBX_r11_Iv(Parse_context& context) 
		{
			parse_register_immediateZ_intruction(context, Opcode::mov, Register_index::rBX_r11);
		};

		void mov_rSP_r12_Iv(Parse_context& context) 
		{
			parse_register_immediateZ_intruction(context, Opcode::mov, Register_index::rSP_r12);
		};

		void mov_rBP_r13_Iv(Parse_context& context) 
		{
			parse_register_immediateZ_intruction(context, Opcode::mov, Register_index::rBP_r13);
		};

		void mov_rSI_r14_Iv(Parse_context& context) 
		{
			parse_register_immediateZ_intruction(context, Opcode::mov, Register_index::rSI_r14);
		};

		void mov_rDI_r15_Iv(Parse_context& context) 
		{
			parse_register_immediateZ_intruction(context, Opcode::mov, Register_index::rDI_r15);
		};
#pragma endregion

#pragma region opcode_table1_lineC
		void grp2_shift_Eb_Ib(Parse_context& context) 
		{
			parse_Eb_Ib(context, grp2_lookup(*(context.data + 1)));
		};

		void grp2_shift_Ev_Ib(Parse_context& context) 
		{
			parse_Ev_Ib(context, grp2_lookup(*(context.data + 1)));
		};

		void ret_Iw_near(Parse_context& context) {};
		void ret_near(Parse_context& context) {};
		void les_Gz_Mp__VEX2(Parse_context& context) {};
		void lds_Gz_Mp__VEX1(Parse_context& context) {};
		void grp11_mov_Eb_Ib(Parse_context& context) {};
		void grp11_mov_Ev_Iz(Parse_context& context) {};

		//////////////////////////////////////////////////////

		void enter_Iw_Ib(Parse_context& context) {};
		void leave(Parse_context& context) {};
		void ret_Iw_far(Parse_context& context) {};
		void ret_far(Parse_context& context) {};
		void int3(Parse_context& context) {};
		void int_Ib(Parse_context& context) {};
		void into(Parse_context& context) {};
		void iret_iretd_iretq(Parse_context& context) {};
#pragma endregion

#pragma region opcode_table1_lineD
		void grp2_shift_Eb_1(Parse_context& context) {};
		void grp2_shift_Ev_1(Parse_context& context) {};
		void grp2_shift_Eb_CL(Parse_context& context) {};
		void grp2_shift_Ev_CL(Parse_context& context) {};
		void aam_Ib(Parse_context& context) {};
		void aad_Ib(Parse_context& context) {};
		void empty_entry(Parse_context& context) {};
		void xlat_xlatb(Parse_context& context) {};

		//////////////////////////////////////////////////////

		void escape_co(Parse_context& context) {};
		void escape_co(Parse_context& context);
		void escape_co(Parse_context& context);
		void escape_co(Parse_context& context);
		void escape_co(Parse_context& context);
		void escape_co(Parse_context& context);
		void escape_co(Parse_context& context);
		void escape_co(Parse_context& context);
#pragma endregion

#pragma region opcode_table1_lineE
		void loopne_loopnz_Jb(Parse_context& context) {};
		void loope_loopz_Jb(Parse_context& context) {};
		void loop_Jb(Parse_context& context) {};
		void JrCXZ_Jb(Parse_context& context) {}; //TODO:: confirm this

		void in_AL_Ib(Parse_context& context) 
		{
			parse_AL_Ib(context, Opcode::in);
		};

		void in_eAX_Ib(Parse_context& context) {};
		void out_Ib_AL(Parse_context& context) {};
		void out_Ib_eAX(Parse_context& context) {};

		//////////////////////////////////////////////////////

		void call_Jz_near(Parse_context& context) {};
		void jmp_Jz_near(Parse_context& context) {};
		void jmp_Ap_far(Parse_context& context) {};
		void jmp_Jb_short(Parse_context& context) {};
		void in_AL_DX(Parse_context& context) {};
		void in_eAX_DX(Parse_context& context) {};
		void out_DX_AL(Parse_context& context) {};
		void out_DX_eAX(Parse_context& context) {};
#pragma endregion

#pragma region opcode_table1_lineF //missing extended tab
		void prefix_lock(Parse_context& context) {};
		void empty_entry(Parse_context& context);
		void prefix_repne_xaquire(Parse_context& context) {};
		void prefix_rep_repe_xrelease(Parse_context& context) {};

		void hlt(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::hlt);
		};

		void cmc(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::cmc);
		};

		void grp3_unary_Eb(Parse_context& context) {};
		void grp3_unary_Ev(Parse_context& context) {};

		//////////////////////////////////////////////////////

		void clc(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::clc);
		};

		void stc(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::stc);
		};

		void cli(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::cli);
		};

		void sti(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::sti);
		};

		void cld(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::cld);
		};

		void std(Parse_context& context) 
		{
			parse_opcode_only_intruction(context, Opcode::std);
		};

		void grp4_inc_dec(Parse_context& context) {};
		void grp5_inc_dec(Parse_context& context) {};
#pragma endregion

		//jumptable to the processing routine
		static const constexpr Opcode_table table =
		{
			{ //line 0
				{ &add_Eb_Gb },{ &add_Ev_Gv },{ &add_Gb_Eb },{ &add_Gv_Ev },{ &add_AL_Ib },{ &add_rAX_Iz },{ &push_es },{ &pop_es },
				{ &or_Eb_Gb },{ &or_Ev_Gv },{ &or_Gb_Eb },{ &or_Gv_Ev },{ &or_AL_Ib },{ &or_rAX_Iz },{ &push_cs },{ &escape_opcode_table2 }
			},
			{ //line 1
				{ &adc_Eb_Gb },{ &adc_Ev_Gv },{ &adc_Gb_Eb },{ &adc_Gv_Ev },{ &adc_AL_Ib },{ &adc_rAX_Iz },{ &push_ss },{ &pop_ss },
				{ &sbb_Eb_Gb },{ &sbb_Ev_Gv },{ &sbb_Gb_Eb },{ &sbb_Gv_Ev },{ &sbb_AL_Ib },{ &sbb_rAX_Iz },{ &push_ds },{ &pop_ds }
			},
			{ //line 2
				{ &and_Eb_Gb },{ &and_Ev_Gv },{ &and_Gb_Eb },{ &and_Gv_Ev },{ &and_AL_Ib },{ &and_rAX_Iz },{ &prefix_seg_es },{ &daa },
				{ &sub_Eb_Gb },{ &sub_Ev_Gv },{ &sub_Gb_Eb },{ &sub_Gv_Ev },{ &sub_AL_Ib },{ &sub_rAX_Iz },{ &prefix_seg_cs },{ &das }
			},
			{ //line 3
				{ &xor_Eb_Gb },{ &xor_Ev_Gv },{ &xor_Gb_Eb },{ &xor_Gv_Ev },{ &xor_AL_Ib },{ &xor_rAX_Iz },{ &prefix_seg_ss },{ &aaa },
				{ &cmp_Eb_Gb },{ &cmp_Ev_Gv },{ &cmp_Gb_Eb },{ &cmp_Gv_Ev },{ &cmp_AL_Ib },{ &cmp_rAX_Iz },{ &prefix_seg_ds },{ &aas }
			},
			{ //line 4
				{ &inc_eAX__REX },{ &inc_eCX__REX_b },{ &inc_eDX__REX_x },{ &inc_eBX__REX_xb },{ &inc_eSP__REX_r },{ &inc_eBP__REX_rb },{ &inc_eSI__REX_rx },{ &inc_eDI__REX_rxb },
				{ &dec_eAX__REX_w },{ &dec_eCX__REX_wb },{ &dec_eDX__REX_wx },{ &dec_eBX__REX_wxb },{ &dec_eSP__REX_wr },{ &dec_eBP__REX_wrb },{ &dec_eSI__REX_wrx },{ &dec_eDI__REX_wrxb }
			},
			{ //line 5
				{ &push_rAX_r8 },{ &push_rCX_r9 },{ &push_rDX_r10 },{ &push_rBX_r11 },{ &push_rSP_r12 },{ &push_rBP_r13 },{ &push_rSI_r14 },{ &push_rDI_r15 },
				{ &pop_rAX_r8 },{ &pop_rCX_r9 },{ &pop_rDX_r10 },{ &pop_rBX_r11 },{ &pop_rSP_r12 },{ &pop_rBP_r13 },{ &pop_rSI_r14 },{ &pop_rDI_r15 }
			},
			{ //line 6
				{ &pusha_pushad },{ &popa_popad },{ &bound_Gv_Ma },{ &arpl_Ew_Gw__movsxd_Gv_Ev },{ &prefix_seg_fs },{ &prefix_seg_gs },{ &prefix_operand_size },{ &prefix_address_size },
				{ &push_Iz },{ &imul_Gv_Ev_Iz },{ &push_Ib },{ &imul_Gv_Ev_Ib },{ &ins_insb_Yb_DX },{ &ins_insw_insd_Yz_DX },{ &outs_outsb_DX_Xb },{ &outs_outsw_outsd_DX_Xz }
			},
			{ //line 7
				{ &jo },{ &jno },{ &jb },{ &jnb },{ &je },{ &jne },{ &jbe},{ &jnbe},
				{ &js },{ &jns },{ &jpe },{ &jpo },{ &jl },{ &jnl },{ &jle },{ &jnle }
			},
			{ //line 8
				{ &grp1_immediate_Eb_Ib },{ &grp1_immediate_Ev_Iz },{ &grp1_immediate_Eb_Ib_legacy },{ &grp1_immediate_Ev_Ib },{ &test_Eb_Gb },{ &test_Ev_Gv },{ &xchg_Eb_Gb },{ &xchg_Ev_Gv },
				{ &mov_Eb_Gb },{ &mov_Ev_Gv },{ &mov_Gb_Eb },{ &mov_Gv_Ev },{ &mov_Ev_Sw },{ &lea_Gv_M },{ &mov_Sw_Ew },{ &grp1a_pop_Ev }
			},
			{ //line 9
				{ &nop_xchg_r8_rAX },{ &xchg_rCX_r9 },{ &xchg_rDX_r10 },{ &xchg_rBX_r11 },{ &xchg_rSP_r12 },{ &xchg_rBP_r13 },{ &xchg_rSI_r14 },{ &xchg_rDI_r15 },
				{ &cbw_cwde_cdqe },{ &cwd_cdq_cqo },{ &call_Ap_far },{ &fwait_wait },{ &pushf_pushfd_pushfq_Fv },{ &popf_popfd_popfq_Fv },{ &sahf },{ &lahf }
			},
			{ //line A
				{ &mov_AL_Ob },{ &mov_rAX_Ov },{ &mov_Ob_AL },{ &mov_Ov_rAX },{ &movs_movb_Yb_Xb },{ &movs_movsw_movsd_movsq_Yv_Xv },{ &cmps_cmpsb_Xb_Yb },{ &cmps_cmpsw_cmpsd_Xv_Yv },
				{ &test_AL_Ib },{ &test_rAX_Iz },{ &stos_stosb_Yb_AL },{ &stos_stosw_stosd_stosq_Yv_rAX },{ &lods_lodsb_AL_Xb },{ &lods_lodsw_lodsd_lodsq_rAX_Xv },{ &scas_scasb_AL_Yb },{ &scas_scasw_scasd_scasq_rAX_Yv }
			},
			{ //line B
				{ &mov_AL_R8L_Ib },{ &mov_CL_R9L_Ib },{ &mov_DL_R10L_Ib },{ &mov_BL_R11L_Ib },{ &mov_AH_R12L_Ib },{ &mov_CH_R13L_Ib },{ &mov_DH_R14L_Ib },{ &mov_BH_R15L_Ib },
				{ &mov_rAX_r8_Iv },{ &mov_rCX_r9_Iv },{ &mov_rDX_r10_Iv },{ &mov_rBX_r11_Iv },{ &mov_rSP_r12_Iv },{ &mov_rBP_r13_Iv },{ &mov_rSI_r14_Iv },{ &mov_rDI_r15_Iv }
			},
			{ //line C
				{ &grp2_shift_Eb_Ib },{ &grp2_shift_Ev_Ib },{ &ret_Iw_near },{ &ret_near },{ &les_Gz_Mp__VEX2 },{ &lds_Gz_Mp__VEX1 },{ &grp11_mov_Eb_Ib },{ &grp11_mov_Ev_Iz },
				{ &enter_Iw_Ib },{ &leave },{ &ret_Iw_far },{ &ret_far },{ &int3 },{ &int_Ib },{ &into },{ &iret_iretd_iretq }
			},
			{ //line D
				{ &grp2_shift_Eb_1 },{ &grp2_shift_Ev_1 },{ &grp2_shift_Eb_CL },{ &grp2_shift_Ev_CL },{ &aam_Ib },{ &aad_Ib },{ &empty_entry },{ &xlat_xlatb },
				{ &escape_co },{ &escape_co },{ &escape_co },{ &escape_co },{ &escape_co },{ &escape_co },{ &escape_co },{ &escape_co }
			},
			{ //line E
				{ &loopne_loopnz_Jb },{ &loope_loopz_Jb },{ &loop_Jb },{ &JrCXZ_Jb },{ &in_AL_Ib },{ &in_eAX_Ib },{ &out_Ib_AL },{ &out_Ib_eAX },
				{ &call_Jz_near },{ &jmp_Jz_near },{ &jmp_Ap_far },{ &jmp_Jb_short },{ &in_AL_DX },{ &in_eAX_DX },{ &out_DX_AL },{ &out_DX_eAX }
			},
			{ //line F
				{ &prefix_lock },{ &empty_entry },{ &prefix_repne_xaquire },{ &prefix_rep_repe_xrelease },{ &hlt },{ &cmc },{ &grp3_unary_Eb },{ &grp3_unary_Ev },
				{ &clc },{ &stc },{ &cli },{ &sti },{ &cld },{ &std },{ &grp4_inc_dec },{ &grp5_inc_dec }
			}
		};
	}

#pragma region helpers
	void store_operand_register(Register*& reg, const Operand& op)
	{
		if (op.register1 != Register::invalid)
			*reg++ = op.register1;
		if (op.register2 != Register::invalid)
			*reg++ = op.register2;
	}

	void store_intruction_operand(Instruction& instruction, const Operand& op1)
	{
		instruction.operand1 = op1.format;

		auto reg_ptr = instruction.register_;
		store_operand_register(reg_ptr, op1);
	}

	void store_intruction_operands(Instruction& instruction, const Operand& op1, const Operand& op2)
	{
		instruction.operand1 = op1.format;
		instruction.operand2 = op2.format;

		auto reg_ptr = instruction.register_;
		store_operand_register(reg_ptr, op1);
		store_operand_register(reg_ptr, op2);
	}

	void store_intruction_operands(Instruction& instruction, const Operand& op1, const Operand& op2, const Operand& op3)
	{
		instruction.operand1 = op1.format;
		instruction.operand2 = op2.format;
		instruction.operand3 = op3.format;

		auto reg_ptr = instruction.register_;
		store_operand_register(reg_ptr, op1);
		store_operand_register(reg_ptr, op2);
		store_operand_register(reg_ptr, op3);
	}

	void store_intruction_operands(Instruction& instruction, const Operand& op1, const Operand& op2, const Operand& op3, const Operand& op4)
	{
		instruction.operand1 = op1.format;
		instruction.operand2 = op2.format;
		instruction.operand3 = op3.format;
		instruction.operand4 = op4.format;

		auto reg_ptr = instruction.register_;
		store_operand_register(reg_ptr, op1);
		store_operand_register(reg_ptr, op2);
		store_operand_register(reg_ptr, op3);
		store_operand_register(reg_ptr, op4);
	}

	void store_immediate_8(Parse_context& context)
	{
		context.instruction.immediate_size = Bit_size::_8;
		context.instruction.immediate = *reinterpret_cast<const int8_t*>(++context.data);
	}

	void store_immediate_16(Parse_context& context)
	{
		context.instruction.immediate_size = Bit_size::_16;
		context.instruction.immediate = *reinterpret_cast<const int16_t*>(++context.data);
		context.data++;
	}

	void store_immediate_32(Parse_context& context)
	{
		context.instruction.immediate_size = Bit_size::_32;
		context.instruction.immediate = *reinterpret_cast<const int32_t*>(++context.data);
		context.data+= 3;
	}

	void store_immediate(Parse_context& context, Bit_size size)
	{
		switch (size)
		{
		case Bit_size::_8:
			return store_immediate_8(context);
		case Bit_size::_16:
			return store_immediate_16(context);
		case Bit_size::_32:
			return store_immediate_32(context);
		}
	}

	void store_displacement_8(Parse_context& context)
	{
		context.instruction.displacement_size = Bit_size::_8;
		context.instruction.displacement = *reinterpret_cast<const int8_t*>(++context.data);
	}

	void store_displacement_16(Parse_context& context)
	{
		context.instruction.displacement_size = Bit_size::_16;
		context.instruction.displacement = *reinterpret_cast<const int16_t*>(++context.data);
		context.data++;
	}

	void store_displacement_32(Parse_context& context)
	{
		context.instruction.displacement_size = Bit_size::_32;
		context.instruction.displacement = *reinterpret_cast<const int32_t*>(++context.data);
		context.data += 3;
	}

	void store_displacement(Parse_context& context, Bit_size size)
	{
		switch (size) 
		{
		case Bit_size::_8:
			return store_displacement_8(context);
		case Bit_size::_16:
			return store_displacement_16(context);
		case Bit_size::_32:
			return store_displacement_32(context);
		}
	}

	Bit_size process_mod_rm_and_sib(const uint8_t*& data, Operand& operand, ModRM mod_rm, Register_type reg_type, bool addressing_mode_16_bit)
	{
		if (mod_rm.mod == 3)
		{
			operand.format = Operand_format::register_;
			operand.register1 = register_table[mod_rm.r_m][static_cast<int>(reg_type)];
			return Bit_size::_0;
		}

		auto addressing_form = addressing_mode_16_bit ?
			mod_addressing_16[mod_rm.mod][mod_rm.r_m] : 
			mod_addressing_32[mod_rm.mod][mod_rm.r_m];

		if (addressing_form.format == Operand_format::sib_table_escape)
		{
			SIB sib = *reinterpret_cast<const SIB*>(++data);

			if (sib.index != 4)
				operand.register1 = register_table[sib.index][1];
			if (mod_rm.mod || sib.base != 5)
				operand.register2 = register_table[sib.base][1];
			if (sib.base != 5)
				addressing_form.size = Bit_size::_0;

			operand.format = sib.scale == 3 ? Operand_format::sib_scaled8
				: sib.scale == 2 ? Operand_format::sib_scaled4
				: sib.scale == 1 ? Operand_format::sib_scaled2
				: Operand_format::sib_scaled;
		}
		else
		{
			operand.format    = addressing_form.format;
			operand.register1 = addressing_form.register1;
			operand.register2 = addressing_form.register2;
		}

		return addressing_form.size;
	}

	constexpr inline bool rex_field_on(Rex_prefix rex_prefix, Rex_field rex_field)
	{
		return static_cast<uint8_t>(rex_prefix) & static_cast<uint8_t>(rex_field);
	}

	constexpr inline Register fetch_register(Register_type type, Register_index index)
	{
		return register_table[static_cast<uint8_t>(type)][static_cast<uint8_t>(index)];
	}

#pragma endregion

	namespace impl
	{
		void process_Eb_Gb__Ev_Gv(Parse_context& context, Register_type reg_type, Operand& first_operand, Operand& second_operand)
		{
			auto mod_rm = *reinterpret_cast<const ModRM*>(++context.data);

			second_operand.format = Operand_format::register_;
			second_operand.register1 = register_table[mod_rm.reg_opcode][static_cast<int>(reg_type)];

			auto disp_size = process_mod_rm_and_sib(context.data, first_operand, mod_rm, reg_type, context.addressing_mode_16_bit);
			store_displacement(context, disp_size);
		}
	}

	void parse_Eb_Ib(Parse_context& context, Opcode opcode)
	{
		Operand op1;
		auto mod_rm = *reinterpret_cast<const ModRM*>(++context.data);
		auto disp_size = process_mod_rm_and_sib(context.data, op1, mod_rm, Register_type::_8, context.addressing_mode_16_bit);

		store_displacement(context, disp_size);
		store_immediate_8(context);
		store_intruction_operands(context.instruction, op1, { Operand_format::immediate });
		context.instruction.opcode = opcode;
	}

	void parse_Ev_Iz(Parse_context& context, Opcode opcode)
	{
		auto reg_type = context.operand_size_16_bit ? Register_type::_16 :
						context.operation_mode == Operation_mode::_32bit ?
						Register_type::_32 : Register_type::_64;
		Operand op1;
		auto mod_rm = *reinterpret_cast<const ModRM*>(++context.data);
		auto disp_size = process_mod_rm_and_sib(context.data, op1, mod_rm, reg_type, context.addressing_mode_16_bit);

		store_displacement(context, disp_size);
		context.operand_size_16_bit ? store_immediate_16(context) : store_immediate_32(context);
		store_intruction_operands(context.instruction, op1, { Operand_format::immediate });
		context.instruction.opcode = opcode;
	}

	void parse_Ev_Ib(Parse_context& context, Opcode opcode)
	{
		auto reg_type = context.operand_size_16_bit ? Register_type::_16 :
						context.operation_mode == Operation_mode::_32bit ?
						Register_type::_32 : Register_type::_64;
		Operand op1;
		auto mod_rm = *reinterpret_cast<const ModRM*>(++context.data);
		auto disp_size = process_mod_rm_and_sib(context.data, op1, mod_rm, reg_type, context.addressing_mode_16_bit);

		store_displacement(context, disp_size);
		store_immediate_8(context);
		store_intruction_operands(context.instruction, op1, { Operand_format::immediate });
		context.instruction.opcode = opcode;
	}

	void parse_Eb_Gb(Parse_context& context, Opcode opcode)
	{
		Operand op1, op2;
		impl::process_Eb_Gb__Ev_Gv(context, Register_type::_8, op1, op2);
		store_intruction_operands(context.instruction, op1, op2);
		context.instruction.opcode = opcode;
	}

	void parse_Gb_Eb(Parse_context& context, Opcode opcode)
	{
		Operand op1, op2;
		impl::process_Eb_Gb__Ev_Gv(context, Register_type::_8, op1, op2);
		store_intruction_operands(context.instruction, op2, op1);
		context.instruction.opcode = opcode;
	}

	void parse_Ev_Gv(Parse_context& context, Opcode opcode)
	{
		auto reg_type = context.operand_size_16_bit ? Register_type::_16 :
		                (context.operation_mode == Operation_mode::_32bit) ?
						Register_type::_32 : Register_type::_64;

		Operand op1, op2;
		impl::process_Eb_Gb__Ev_Gv(context, reg_type, op1, op2);
		store_intruction_operands(context.instruction, op1, op2);
		context.instruction.opcode = opcode;
	}

	void parse_Gv_Ev(Parse_context& context, Opcode opcode)
	{
		auto reg_index = context.operand_size_16_bit ? Register_type::_16 :
			(context.operation_mode == Operation_mode::_32bit) ?
			Register_type::_32 : Register_type::_64;

		Operand op1, op2;
		impl::process_Eb_Gb__Ev_Gv(context, reg_index, op1, op2);
		store_intruction_operands(context.instruction, op2, op1);
		context.instruction.opcode = opcode;
	}

	void parse_AL_Ib(Parse_context& context, Opcode opcode)
	{
		store_immediate_8(context);
		store_intruction_operands(context.instruction, { Operand_format::register_, Register::al }, { Operand_format::immediate });
		context.instruction.opcode = opcode;
	}

	void parse_rAX_Iz(Parse_context& context, Opcode opcode)
	{
		Operand op1{ Operand_format::register_ };

		if (context.operand_size_16_bit)
		{
			op1.register1 = Register::ax;
			store_immediate_16(context);
		}
		else
		{
			op1.register1 = context.rex_prefix == Rex_prefix::rex_w ? Register::rax : Register::eax;
			store_immediate_32(context);
		}
		store_intruction_operands(context.instruction, op1, { Operand_format::immediate });
		context.instruction.opcode = opcode;
	}

	void parse_rex_prefix(Parse_context& context, Opcode opcode, Rex_prefix prefix, Register_index reg_index)
	{
		++context.data;
		if (context.operation_mode == Operation_mode::_64bit)
		{
			if (context.operand_size_override_prefix)
				context.operand_size_16_bit = rex_field_on(prefix, Rex_field::w) ? !context.operand_size_16_bit : true;

			const_cast<Rex_prefix>(context.rex_prefix) = prefix;
			opcode_table1::table[*context.data >> 4][*context.data & 0x0F](context);
		}
		else
		{
			auto& instruction = context.instruction;
			instruction.opcode = opcode;
			instruction.operand1 = Operand_format::register_;
			instruction.register_[0] = fetch_register(context.operand_size_16_bit ? Register_type::_16 : Register_type::_32, reg_index);
		}
	}

	void parse_register_intruction(Parse_context& context, Opcode opcode, Register reg)
	{
		++context.data;
		context.instruction.opcode = opcode;
		context.instruction.operand1 = Operand_format::register_;
		context.instruction.register_[0] = reg;
	}

	void parse_segment_prefix(Parse_context& context, Segment_prefix prefix)
	{
		auto data = ++context.data;
		const_cast<Segment_prefix>(context.segment_prefix) = prefix;
		opcode_table1::table[*data >> 4][*data & 0x0F](context);
	}

	void parse_prefix_address_size(Parse_context& context)
	{
		const_cast<bool&>(context.address_size_override_prefix) = true;
		context.addressing_mode_16_bit = !context.addressing_mode_16_bit;
		opcode_table_call(opcode_table1::table, context, *++context.data);
	}

	void parse_prefix_operand_size(Parse_context& context)
	{
		const_cast<bool&>(context.operand_size_override_prefix) = true;
		context.operand_size_16_bit = !context.operand_size_16_bit;
		opcode_table_call(opcode_table1::table, context, *++context.data);
	}

	void parse_push_pop_d64_register(Parse_context& context, Opcode opcode, Register_index reg_index)
	{
		Register_type reg_type;

		if(context.operation_mode == Operation_mode::_64bit)
		{
			reg_type = rex_field_on(context.rex_prefix, Rex_field::b) ?
					   context.operand_size_16_bit ? Register_type::rW  : Register_type::r :
					   context.operand_size_16_bit ? Register_type::_64 : Register_type::_16;
		}
		else
			reg_type = context.operand_size_16_bit ? Register_type::_32 : Register_type::_16;

		context.data++;
		context.instruction.opcode = opcode;
		context.instruction.operand1 = Operand_format::register_;
		context.instruction.register_[0] = fetch_register(reg_type, reg_index);
	}

	void parse_opcode_only_intruction(Parse_context& context, Opcode opcode)
	{
		context.instruction.opcode = opcode;
		context.data++;
	}

	void parse_immediate8_instruction(Parse_context& context, Opcode opcode)
	{
		context.instruction.opcode = opcode;
		context.instruction.operand1 = Operand_format::immediate;
		store_immediate_8(context);
	}

	void parse_register_immediate8_intruction(Parse_context& context, Opcode opcode, Register_index reg_index)
	{
		auto reg_type = rex_field_on(context.rex_prefix, Rex_field::r) ? Register_type::rL : Register_type::_8;

		context.instruction.opcode = opcode;
		context.instruction.operand1 = Operand_format::register_;
		context.instruction.register_[0] = fetch_register(reg_type, reg_index);
		context.instruction.operand2 = Operand_format::immediate;
		store_immediate_8(context);
	}

	void parse_register_immediateZ_intruction(Parse_context& context, Opcode opcode, Register_index reg_index)
	{
		Register_type reg_type;

		if (context.operation_mode == Operation_mode::_64bit)
		{
			reg_type = rex_field_on(context.rex_prefix, Rex_field::r) ?
				context.operand_size_16_bit ? Register_type::rW : Register_type::r :
				context.operand_size_16_bit ? Register_type::_64 : Register_type::_16;
		}
		else
			reg_type = context.operand_size_16_bit ? Register_type::_32 : Register_type::_16;

		context.instruction.opcode = opcode;
		context.instruction.operand1 = Operand_format::register_;
		context.instruction.register_[0] = fetch_register(reg_type, reg_index);
		context.instruction.operand2 = Operand_format::immediate;
		context.operand_size_16_bit ? store_immediate_16(context) : store_immediate_32(context);
	}
}

namespace diasm
{
	Instruction Decoder::decode(const uint8_t* data) const noexcept
	{
		Parse_context context(data, mode_ == Operation_mode::_16bit, mode_, mode_ == Operation_mode::_16bit);

		//TODO::move prefix logic here to avoid jumping around
		opcode_table_call(opcode_table1::table, context, *data);

		context.instruction.original_size = context.data - data + 1;
		return context.instruction;
	}
}
#pragma once

#include <cstdint>

namespace diasm
{
	enum class Register : uint8_t
	{
		invalid = 0,
		cs, ds, ss, es, fs, gs,
		r8,  r8D,   r8W,  r8L, rax, eax, ax, al, mm0, xmm0, ymm0,
		r9,  r9D,   r9W,  r9L, rcx, ecx, cx, cl, mm1, xmm1, ymm1,
		r10, r10D, r10W, r10L, rdx, edx, dx, dl, mm2, xmm2, ymm2,
		r11, r11D, r11W, r11L, rbx, ebx, bx, bl, mm3, xmm3, ymm3,
		r12, r12D, r12W, r12L, rsp, esp, sp, ah, mm4, xmm4, ymm4,
		r13, r13D, r13W, r13L, rbp, ebp, bp, ch, mm5, xmm5, ymm5,
		r14, r14D, r14W, r14L, rsi, esi, si, dh, mm6, xmm6, ymm6,
		r15, r15D, r15W, r15L, rdi, edi, di, bh, mm7, xmm7, ymm7,
	};

	enum class Custom_prefix : uint8_t
	{
		invalid = 0,
		xacquire,
		xrelease,
		lock_xacquire,
		lock_xrelease,
		lock,
		rep,
		repe,
		repene
	};

	enum class Opcode : uint8_t
	{
		invalid = 0,
		escape, //private use
		add,
		push,
		pop,
		adc,
		and,
		or,
		sbb,
		sub,
		daa,
		aaa,
		das,
		aas,
		xor,
		cmp,
		test,
		xchg,
		mov,
		in,
		inc,
		dec,
		pusha,
		pushad,
		popa,
		popad,
		jo,
		jno,
		jb,
		jnb,
		je,
		jne,
		jbe,
		jnbe,
		js,
		jns,
		jpe,
		jpo,
		jl,
		jnl,
		jle,
		jnle,
		std,
		clc,
		stc,
		cli,
		sti,
		cld,
		hlt,
		cmc,
		rol,
		ror,
		rcl,
		rcr,
		shl,
		shr,
		sar
	};

	enum class Operand_format : uint8_t
	{
		invalid = 0,
		immediate,
		register_,
		sib_scaled,
		sib_scaled2,
		sib_scaled4,
		sib_scaled8,
		displacement,
		addr_of_reg1,
		addr_of_reg1_disp,
		addr_of_reg1_reg2,
		addr_of_reg1_reg2_disp,
		sib_table_escape //private use
	};

	enum class Bit_size : uint8_t
	{
		_0 = 0,
		_8,
		_16,
		_32
	};

	enum class Addressing_size : uint8_t
	{
		_0 = 0,
		_8,
		_16,
		_32,
		_64
	};
}
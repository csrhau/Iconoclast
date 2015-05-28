/******************************************************************************
 * FIRESTARTER - A Processor Stress Test Utility 
 * Copyright (C) 2014 TU Dresden, Center for Information Services and High
 * Performance Computing
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * Contact: daniel.hackenberg@tu-dresden.de
 *****************************************************************************/

#include "work.h"


																																																																																																																																													int init_bld_opteron_fma4_1t(unsigned long long addrMem) __attribute__((noinline));
int init_bld_opteron_fma4_1t(unsigned long long addrMem)
{
	int i;
	for (i=0;i<13338624;i++) *((double*)(addrMem+8*i)) = 0.25 + (double)(i%9267) * 0.24738995982e-4;

	return EXIT_SUCCESS;
}

/**
 * assembler implementation of processor and memory stress test
 * uses FMA instruction set
 * AMD Bulldozer (experimental)
 * @input - addrMem:   pointer to buffer
 * @return EXIT_SUCCESS
 */
int asm_work_bld_opteron_fma4_1t(unsigned long long addrMem, unsigned long long addrHigh) __attribute__((noinline));
int asm_work_bld_opteron_fma4_1t(unsigned long long addrMem, unsigned long long addrHigh)
{
	if (*((unsigned long long*)addrHigh) == 0) return EXIT_SUCCESS;
		/* input: 
		 *   - addrMem -> rax
		 * register usage:
		 *   - rax:		stores original pointer to buffer, used to periodically reset other pointers
		 *   - rbx:		pointer to L1 buffer
		 *   - rcx:		pointer to L2 buffer
		 *   - r8:		pointer to L3 buffer
		 *   - r9:		pointer to RAM buffer
		 *   - r10:		counter for L2-pointer reset
		 *   - r11:		counter for L3-pointer reset
		 *   - r12:		counter for RAM-pointer reset
		 *   - r13:		register for temporary results
		 *   - r14:		stores cacheline width as increment for buffer addresses
		 *   - r15:		stores address of shared variable that controls load level
		 *   - rdx, rsi, rdi:	registers for shift operations
		 *   - mm*,xmm*,xmm*:	data registers for SIMD instructions
		 */
	       __asm__ __volatile__(
		"mov %0, %%rax;" 	// store start address of buffer
		"mov %1, %%r15;" 	// store address of shared variable that controls load level
		"mov $64, %%r14;"	// increment after each cache/memory access
		//Initialize registers for shift operations
		"mov $0xAAAAAAAA, %%edi;"
		"mov $0xAAAAAAAA, %%esi;"
		"mov $0xAAAAAAAA, %%edx;"
		//Initialize AVX-Registers for FMA Operations
		"vmovapd (%%rax), %%xmm0;"
		"vmovapd (%%rax), %%xmm1;"		
		"vmovapd 320(%%rax), %%xmm2;"
		"vmovapd 352(%%rax), %%xmm3;"
		"vmovapd 384(%%rax), %%xmm4;"
		"vmovapd 416(%%rax), %%xmm5;"
		"vmovapd 448(%%rax), %%xmm6;"
		"vmovapd 480(%%rax), %%xmm7;"
		"vmovapd 512(%%rax), %%xmm8;"
		"vmovapd 544(%%rax), %%xmm9;"
		"vmovapd 576(%%rax), %%xmm10;"
		"vmovapd 608(%%rax), %%xmm11;"
		"vmovapd 640(%%rax), %%xmm12;"
		"vmovapd 672(%%rax), %%xmm13;"
		"mov %%rax, %%rbx;" 	// address for L1-buffer 
		"mov %%rax, %%rcx;"     
		"add $16384, %%rcx;"	// address for L2-buffer
		"mov %%rax, %%r8;"
		"add $1048576, %%r8;"	// address for L3-buffer
		"mov %%rax, %%r9;"	 		
		"add $786432, %%r9;"	// address for RAM-buffer
		"movabs $262, %%r10;"	// reset-counter for L2-buffer with 50 cache line accesses per loop (818 KB)
		"movabs $983, %%r11;"	// reset-counter for L3-buffer with 10 cache line accesses per loop (614 KB)
		"movabs $163840, %%r12;"	// reset-counter for RAM-buffer with 10 cache line accesses per loop (102400 KB)

                #ifdef __MACH__ /* Mac OS compatibility */
                ".align 6;"      /* alignment as power of 2 */
                #else
                ".align 64;"     /* alignment in bytes */
                #endif
                "_work_loop_bld_opteron_fma4_1t:"
		/*****************************************************************************************************************************************************
		decode 0 					decode 1 				decode 2	decode 3				     */
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm6, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm6, %%xmm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm8, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm8, %%xmm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm2, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm2, %%xmm2;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 64(%%r8), %%xmm1, %%xmm4, %%xmm4;	shr $1, %%esi;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm7, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm7, %%xmm7;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm9, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm9, %%xmm9;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shr $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm4, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm4, %%xmm4;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm6, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm6, %%xmm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	mov %%rax, %%rbx;"	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm9, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm9, %%xmm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 64(%%r8), %%xmm1, %%xmm2, %%xmm2;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm5, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm5, %%xmm5;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm7, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm7, %%xmm7;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shl $1, %%edx;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm2, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm2, %%xmm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm4, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm4, %%xmm4;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm7, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm7, %%xmm7;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 64(%%r8), %%xmm1, %%xmm9, %%xmm9;	shl $1, %%edi;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm3, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm3, %%xmm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	mov %%rax, %%rbx;"	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm5, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm5, %%xmm5;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm9, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm9, %%xmm9;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm2, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm2, %%xmm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm5, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm5, %%xmm5;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 64(%%r8), %%xmm1, %%xmm7, %%xmm7;	shr $1, %%esi;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm10, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm10, %%xmm10;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm3, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm3, %%xmm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shr $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm7, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm7, %%xmm7;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	mov %%rax, %%rbx;"	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm9, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm9, %%xmm9;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm3, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm3, %%xmm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 64(%%r8), %%xmm1, %%xmm5, %%xmm5;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm8, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm8, %%xmm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm10, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm10, %%xmm10;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shl $1, %%edx;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm5, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm5, %%xmm5;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm7, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm7, %%xmm7;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm10, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm10, %%xmm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 64(%%r8), %%xmm1, %%xmm3, %%xmm3;	shl $1, %%edi;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	mov %%rax, %%rbx;"	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm6, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm6, %%xmm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm8, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm8, %%xmm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm3, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm3, %%xmm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm5, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm5, %%xmm5;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm8, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm8, %%xmm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 64(%%r8), %%xmm1, %%xmm10, %%xmm10;	shr $1, %%esi;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm4, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm4, %%xmm4;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm6, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm6, %%xmm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shr $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	mov %%rax, %%rbx;"	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm10, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm10, %%xmm10;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm3, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm3, %%xmm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm6, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm6, %%xmm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 64(%%r8), %%xmm1, %%xmm8, %%xmm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm2, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm2, %%xmm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm4, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm4, %%xmm4;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shl $1, %%edx;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm8, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm8, %%xmm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm10, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm10, %%xmm10;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm4, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm4, %%xmm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	mov %%rax, %%rbx;"	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 64(%%r8), %%xmm1, %%xmm6, %%xmm6;	shl $1, %%edi;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm9, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm9, %%xmm9;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm2, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm2, %%xmm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 64(%%r9), %%xmm1, %%xmm15, %%xmm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm6, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm6, %%xmm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm8, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm8, %%xmm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm2, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm2, %%xmm2;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 64(%%r8), %%xmm1, %%xmm4, %%xmm4;	shr $1, %%esi;	add %%r14, %%r8;  "	// L3 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd %%xmm10, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd %%xmm4, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd %%xmm7, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm7, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm7, %%xmm7;	shl $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd %%xmm2, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm10, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd %%xmm5, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm4, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd %%xmm8, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm7, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vmovapd %%xmm9, 96(%%rcx);			vfmaddpd 64(%%rcx), %%xmm0, %%xmm9, %%xmm9;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm2, %%xmm0, %%xmm10, %%xmm10;		vfmaddpd %%xmm3, %%xmm1, %%xmm11, %%xmm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm3, %%xmm0, %%xmm2, %%xmm2;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm2, %%ymm2;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm4, %%xmm0, %%xmm3, %%xmm3;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm3, %%ymm3;	shr $1, %%edi;	mov %%rax, %%rbx;"	// L1 load
		"vfmaddpd %%xmm5, %%xmm0, %%xmm4, %%xmm4;		vfmaddpd %%xmm6, %%xmm1, %%xmm12, %%xmm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm6, %%xmm0, %%xmm5, %%xmm5;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm5, %%ymm5;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm7, %%xmm0, %%xmm6, %%xmm6;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm6, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm8, %%xmm0, %%xmm7, %%xmm7;		vfmaddpd %%xmm9, %%xmm1, %%xmm13, %%xmm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vfmaddpd %%xmm9, %%xmm0, %%xmm8, %%xmm8;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm8, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load
		"vfmaddpd %%xmm10, %%xmm0, %%xmm9, %%xmm9;		vfmaddpd 32(%%rbx), %%ymm1, %%ymm9, %%ymm9;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load
		//reset RAM counter
		"sub $1, %%r12;"
		"jnz _work_no_ram_reset_bld_opteron_fma4_1t;"
		"movabs $163840, %%r12;" 
		"mov %%rax, %%r9;"
		"add $786432, %%r9;"
		"_work_no_ram_reset_bld_opteron_fma4_1t:"
		//reset L2-Cache counter
		"sub $1, %%r10;"
		"jnz _work_no_L2_reset_bld_opteron_fma4_1t;"
		"movabs $262, %%r10;"
		"mov %%rax, %%rcx;"
		"add $16384, %%rcx;"
		"_work_no_L2_reset_bld_opteron_fma4_1t:"
		//reset L3-Cache counter
		"sub $1, %%r11;"
		"jnz _work_no_L3_reset_bld_opteron_fma4_1t;"
		"movabs $983, %%r11;"	
		"mov %%rax, %%r8;"
		"add $1048576, %%r8;"
		"_work_no_L3_reset_bld_opteron_fma4_1t:"
		"mov %%rax, %%rbx;"
		"mov (%%r15), %%r13;"
		"test $1, %%r13;"
		"jnz _work_loop_bld_opteron_fma4_1t;"
                :
		: "a"(addrMem), "b"(addrHigh) 
                : "%rcx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rdx", "%rsi", "%rdi", "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7", "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15"
		);
	return EXIT_SUCCESS;
}

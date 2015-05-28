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


																																																																																																																																														int init_hsw_corei_fma_1t(unsigned long long addrMem) __attribute__((noinline));
int init_hsw_corei_fma_1t(unsigned long long addrMem)
{
	int i;
	for (i=0;i<13340672;i++) *((double*)(addrMem+8*i)) = 0.25 + (double)(i%9267) * 0.24738995982e-4;

	return EXIT_SUCCESS;
}

/**
 * assembler implementation of processor and memory stress test
 * uses FMA instruction set
 * optimized for Intel Haswell/Broadwell based Core i5/i7 and Xeon E3 with disabled Hyperthreading
 * @input - addrMem:   pointer to buffer
 * @return EXIT_SUCCESS
 */
int asm_work_hsw_corei_fma_1t(unsigned long long addrMem, unsigned long long addrHigh) __attribute__((noinline));
int asm_work_hsw_corei_fma_1t(unsigned long long addrMem, unsigned long long addrHigh)
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
		 *   - mm*,xmm*,ymm*:	data registers for SIMD instructions
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
		"vmovapd (%%rax), %%ymm0;"
		"vmovapd (%%rax), %%ymm1;"		
		"vmovapd 320(%%rax), %%ymm2;"
		"vmovapd 352(%%rax), %%ymm3;"
		"vmovapd 384(%%rax), %%ymm4;"
		"vmovapd 416(%%rax), %%ymm5;"
		"vmovapd 448(%%rax), %%ymm6;"
		"vmovapd 480(%%rax), %%ymm7;"
		"vmovapd 512(%%rax), %%ymm8;"
		"vmovapd 544(%%rax), %%ymm9;"
		"vmovapd 576(%%rax), %%ymm10;"
		"vmovapd 608(%%rax), %%ymm11;"
		"vmovapd 640(%%rax), %%ymm12;"
		"vmovapd 672(%%rax), %%ymm13;"
		"mov %%rax, %%rbx;" 	// address for L1-buffer 
		"mov %%rax, %%rcx;"     
		"add $32768, %%rcx;"	// address for L2-buffer
		"mov %%rax, %%r8;"
		"add $262144, %%r8;"	// address for L3-buffer
		"mov %%rax, %%r9;"	 		
		"add $1572864, %%r9;"	// address for RAM-buffer
		"movabs $36, %%r10;"	// reset-counter for L2-buffer with 90 cache line accesses per loop (202 KB)
		"movabs $655, %%r11;"	// reset-counter for L3-buffer with 30 cache line accesses per loop (1228 KB)
		"movabs $81920, %%r12;"	// reset-counter for RAM-buffer with 20 cache line accesses per loop (102400 KB)

                #ifdef __MACH__ /* Mac OS compatibility */
                ".align 6;"      /* alignment as power of 2 */
                #else
                ".align 64;"     /* alignment in bytes */
                #endif
                "_work_loop_hsw_corei_fma_1t:"
		/*****************************************************************************************************************************************************
		decode 0 					decode 1 				decode 2	decode 3				     */
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		//reset RAM counter
		"sub $1, %%r12;"
		"jnz _work_no_ram_reset_hsw_corei_fma_1t;"
		"movabs $81920, %%r12;" 
		"mov %%rax, %%r9;"
		"add $1572864, %%r9;"
		"_work_no_ram_reset_hsw_corei_fma_1t:"
		//reset L2-Cache counter
		"sub $1, %%r10;"
		"jnz _work_no_L2_reset_hsw_corei_fma_1t;"
		"movabs $36, %%r10;"
		"mov %%rax, %%rcx;"
		"add $32768, %%rcx;"
		"_work_no_L2_reset_hsw_corei_fma_1t:"
		//reset L3-Cache counter
		"sub $1, %%r11;"
		"jnz _work_no_L3_reset_hsw_corei_fma_1t;"
		"movabs $655, %%r11;"	
		"mov %%rax, %%r8;"
		"add $262144, %%r8;"
		"_work_no_L3_reset_hsw_corei_fma_1t:"
		"mov %%rax, %%rbx;"
		"mov (%%r15), %%r13;"
		"test $1, %%r13;"
		"jnz _work_loop_hsw_corei_fma_1t;"
                :
		: "a"(addrMem), "b"(addrHigh) 
                : "%rcx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rdx", "%rsi", "%rdi", "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7", "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15"
		);
	return EXIT_SUCCESS;
}


																																																																																																																																														int init_hsw_corei_fma_2t(unsigned long long addrMem) __attribute__((noinline));
int init_hsw_corei_fma_2t(unsigned long long addrMem)
{
	int i;
	for (i=0;i<6670336;i++) *((double*)(addrMem+8*i)) = 0.25 + (double)(i%9267) * 0.24738995982e-4;

	return EXIT_SUCCESS;
}

/**
 * assembler implementation of processor and memory stress test
 * uses FMA instruction set
 * optimized for Intel Haswell/Broadwell based Core i5/i7 and Xeon E3 with enabled Hyperthreading
 * @input - addrMem:   pointer to buffer
 * @return EXIT_SUCCESS
 */
int asm_work_hsw_corei_fma_2t(unsigned long long addrMem, unsigned long long addrHigh) __attribute__((noinline));
int asm_work_hsw_corei_fma_2t(unsigned long long addrMem, unsigned long long addrHigh)
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
		 *   - mm*,xmm*,ymm*:	data registers for SIMD instructions
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
		"vmovapd (%%rax), %%ymm0;"
		"vmovapd (%%rax), %%ymm1;"		
		"vmovapd 320(%%rax), %%ymm2;"
		"vmovapd 352(%%rax), %%ymm3;"
		"vmovapd 384(%%rax), %%ymm4;"
		"vmovapd 416(%%rax), %%ymm5;"
		"vmovapd 448(%%rax), %%ymm6;"
		"vmovapd 480(%%rax), %%ymm7;"
		"vmovapd 512(%%rax), %%ymm8;"
		"vmovapd 544(%%rax), %%ymm9;"
		"vmovapd 576(%%rax), %%ymm10;"
		"vmovapd 608(%%rax), %%ymm11;"
		"vmovapd 640(%%rax), %%ymm12;"
		"vmovapd 672(%%rax), %%ymm13;"
		"mov %%rax, %%rbx;" 	// address for L1-buffer 
		"mov %%rax, %%rcx;"     
		"add $16384, %%rcx;"	// address for L2-buffer
		"mov %%rax, %%r8;"
		"add $131072, %%r8;"	// address for L3-buffer
		"mov %%rax, %%r9;"	 		
		"add $786432, %%r9;"	// address for RAM-buffer
		"movabs $36, %%r10;"	// reset-counter for L2-buffer with 45 cache line accesses per loop (101 KB)
		"movabs $655, %%r11;"	// reset-counter for L3-buffer with 15 cache line accesses per loop (614 KB)
		"movabs $81920, %%r12;"	// reset-counter for RAM-buffer with 10 cache line accesses per loop (51200 KB)

                #ifdef __MACH__ /* Mac OS compatibility */
                ".align 6;"      /* alignment as power of 2 */
                #else
                ".align 64;"     /* alignment in bytes */
                #endif
                "_work_loop_hsw_corei_fma_2t:"
		/*****************************************************************************************************************************************************
		decode 0 					decode 1 				decode 2	decode 3				     */
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm7, %%ymm0, %%ymm6;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shr $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		//reset RAM counter
		"sub $1, %%r12;"
		"jnz _work_no_ram_reset_hsw_corei_fma_2t;"
		"movabs $81920, %%r12;" 
		"mov %%rax, %%r9;"
		"add $786432, %%r9;"
		"_work_no_ram_reset_hsw_corei_fma_2t:"
		//reset L2-Cache counter
		"sub $1, %%r10;"
		"jnz _work_no_L2_reset_hsw_corei_fma_2t;"
		"movabs $36, %%r10;"
		"mov %%rax, %%rcx;"
		"add $16384, %%rcx;"
		"_work_no_L2_reset_hsw_corei_fma_2t:"
		//reset L3-Cache counter
		"sub $1, %%r11;"
		"jnz _work_no_L3_reset_hsw_corei_fma_2t;"
		"movabs $655, %%r11;"	
		"mov %%rax, %%r8;"
		"add $131072, %%r8;"
		"_work_no_L3_reset_hsw_corei_fma_2t:"
		"mov %%rax, %%rbx;"
		"mov (%%r15), %%r13;"
		"test $1, %%r13;"
		"jnz _work_loop_hsw_corei_fma_2t;"
                :
		: "a"(addrMem), "b"(addrHigh) 
                : "%rcx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rdx", "%rsi", "%rdi", "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7", "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15"
		);
	return EXIT_SUCCESS;
}


																																																																																																																												int init_hsw_xeonep_fma_1t(unsigned long long addrMem) __attribute__((noinline));
int init_hsw_xeonep_fma_1t(unsigned long long addrMem)
{
	int i;
	for (i=0;i<13471744;i++) *((double*)(addrMem+8*i)) = 0.25 + (double)(i%9267) * 0.24738995982e-4;

	return EXIT_SUCCESS;
}

/**
 * assembler implementation of processor and memory stress test
 * uses FMA instruction set
 * optimized for Intel Haswell/Broadwell-E based Core i7 and Haswell/Broadwell-EP based Xeon E5 with disabled Hyperthreading
 * @input - addrMem:   pointer to buffer
 * @return EXIT_SUCCESS
 */
int asm_work_hsw_xeonep_fma_1t(unsigned long long addrMem, unsigned long long addrHigh) __attribute__((noinline));
int asm_work_hsw_xeonep_fma_1t(unsigned long long addrMem, unsigned long long addrHigh)
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
		 *   - mm*,xmm*,ymm*:	data registers for SIMD instructions
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
		"vmovapd (%%rax), %%ymm0;"
		"vmovapd (%%rax), %%ymm1;"		
		"vmovapd 320(%%rax), %%ymm2;"
		"vmovapd 352(%%rax), %%ymm3;"
		"vmovapd 384(%%rax), %%ymm4;"
		"vmovapd 416(%%rax), %%ymm5;"
		"vmovapd 448(%%rax), %%ymm6;"
		"vmovapd 480(%%rax), %%ymm7;"
		"vmovapd 512(%%rax), %%ymm8;"
		"vmovapd 544(%%rax), %%ymm9;"
		"vmovapd 576(%%rax), %%ymm10;"
		"vmovapd 608(%%rax), %%ymm11;"
		"vmovapd 640(%%rax), %%ymm12;"
		"vmovapd 672(%%rax), %%ymm13;"
		"mov %%rax, %%rbx;" 	// address for L1-buffer 
		"mov %%rax, %%rcx;"     
		"add $32768, %%rcx;"	// address for L2-buffer
		"mov %%rax, %%r8;"
		"add $262144, %%r8;"	// address for L3-buffer
		"mov %%rax, %%r9;"	 		
		"add $2621440, %%r9;"	// address for RAM-buffer
		"movabs $30, %%r10;"	// reset-counter for L2-buffer with 108 cache line accesses per loop (202 KB)
		"movabs $2730, %%r11;"	// reset-counter for L3-buffer with 12 cache line accesses per loop (2047 KB)
		"movabs $68266, %%r12;"	// reset-counter for RAM-buffer with 24 cache line accesses per loop (102399 KB)

                #ifdef __MACH__ /* Mac OS compatibility */
                ".align 6;"      /* alignment as power of 2 */
                #else
                ".align 64;"     /* alignment in bytes */
                #endif
                "_work_loop_hsw_xeonep_fma_1t:"
		/*****************************************************************************************************************************************************
		decode 0 					decode 1 				decode 2	decode 3				     */
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		//reset RAM counter
		"sub $1, %%r12;"
		"jnz _work_no_ram_reset_hsw_xeonep_fma_1t;"
		"movabs $68266, %%r12;" 
		"mov %%rax, %%r9;"
		"add $2621440, %%r9;"
		"_work_no_ram_reset_hsw_xeonep_fma_1t:"
		//reset L2-Cache counter
		"sub $1, %%r10;"
		"jnz _work_no_L2_reset_hsw_xeonep_fma_1t;"
		"movabs $30, %%r10;"
		"mov %%rax, %%rcx;"
		"add $32768, %%rcx;"
		"_work_no_L2_reset_hsw_xeonep_fma_1t:"
		//reset L3-Cache counter
		"sub $1, %%r11;"
		"jnz _work_no_L3_reset_hsw_xeonep_fma_1t;"
		"movabs $2730, %%r11;"	
		"mov %%rax, %%r8;"
		"add $262144, %%r8;"
		"_work_no_L3_reset_hsw_xeonep_fma_1t:"
		"mov %%rax, %%rbx;"
		"mov (%%r15), %%r13;"
		"test $1, %%r13;"
		"jnz _work_loop_hsw_xeonep_fma_1t;"
                :
		: "a"(addrMem), "b"(addrHigh) 
                : "%rcx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rdx", "%rsi", "%rdi", "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7", "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15"
		);
	return EXIT_SUCCESS;
}


																																																																																																																												int init_hsw_xeonep_fma_2t(unsigned long long addrMem) __attribute__((noinline));
int init_hsw_xeonep_fma_2t(unsigned long long addrMem)
{
	int i;
	for (i=0;i<6735872;i++) *((double*)(addrMem+8*i)) = 0.25 + (double)(i%9267) * 0.24738995982e-4;

	return EXIT_SUCCESS;
}

/**
 * assembler implementation of processor and memory stress test
 * uses FMA instruction set
 * optimized for Intel Haswell/Broadwell-E based Core i7 and Haswell/Broadwell-EP based Xeon E5 with enabled Hyperthreading
 * @input - addrMem:   pointer to buffer
 * @return EXIT_SUCCESS
 */
int asm_work_hsw_xeonep_fma_2t(unsigned long long addrMem, unsigned long long addrHigh) __attribute__((noinline));
int asm_work_hsw_xeonep_fma_2t(unsigned long long addrMem, unsigned long long addrHigh)
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
		 *   - mm*,xmm*,ymm*:	data registers for SIMD instructions
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
		"vmovapd (%%rax), %%ymm0;"
		"vmovapd (%%rax), %%ymm1;"		
		"vmovapd 320(%%rax), %%ymm2;"
		"vmovapd 352(%%rax), %%ymm3;"
		"vmovapd 384(%%rax), %%ymm4;"
		"vmovapd 416(%%rax), %%ymm5;"
		"vmovapd 448(%%rax), %%ymm6;"
		"vmovapd 480(%%rax), %%ymm7;"
		"vmovapd 512(%%rax), %%ymm8;"
		"vmovapd 544(%%rax), %%ymm9;"
		"vmovapd 576(%%rax), %%ymm10;"
		"vmovapd 608(%%rax), %%ymm11;"
		"vmovapd 640(%%rax), %%ymm12;"
		"vmovapd 672(%%rax), %%ymm13;"
		"mov %%rax, %%rbx;" 	// address for L1-buffer 
		"mov %%rax, %%rcx;"     
		"add $16384, %%rcx;"	// address for L2-buffer
		"mov %%rax, %%r8;"
		"add $131072, %%r8;"	// address for L3-buffer
		"mov %%rax, %%r9;"	 		
		"add $1310720, %%r9;"	// address for RAM-buffer
		"movabs $30, %%r10;"	// reset-counter for L2-buffer with 54 cache line accesses per loop (101 KB)
		"movabs $2730, %%r11;"	// reset-counter for L3-buffer with 6 cache line accesses per loop (1023 KB)
		"movabs $68266, %%r12;"	// reset-counter for RAM-buffer with 12 cache line accesses per loop (51199 KB)

                #ifdef __MACH__ /* Mac OS compatibility */
                ".align 6;"      /* alignment as power of 2 */
                #else
                ".align 64;"     /* alignment in bytes */
                #endif
                "_work_loop_hsw_xeonep_fma_2t:"
		/*****************************************************************************************************************************************************
		decode 0 					decode 1 				decode 2	decode 3				     */
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	mov %%rax, %%rbx;"	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm4, %%ymm0, %%ymm3;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%edi;	add %%r14, %%r9;  "	// RAM load
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 96(%%r8);			vfmadd231pd 64(%%r8), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%r8;  "	// L3 load, L3 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm11;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm12;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm13;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm11;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd 64(%%r9), %%ymm1, %%ymm15;	shl $1, %%esi;	add %%r14, %%r9;  "	// RAM load
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm12;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm13;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm11;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm12;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm13;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm11;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm3, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm3;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm5, %%ymm0, %%ymm4;		vfmadd231pd %%ymm6, %%ymm1, %%ymm12;	shl $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm5, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm5;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shr $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm8, %%ymm0, %%ymm7;		vfmadd231pd %%ymm9, %%ymm1, %%ymm13;	shr $1, %%esi;	xor %%rdi, %%r13;  "	// REG ops only
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm9, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm9;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm3, %%ymm0, %%ymm2;		vfmadd231pd %%ymm4, %%ymm1, %%ymm11;	shl $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm3, 96(%%rcx);			vfmadd231pd 64(%%rcx), %%ymm0, %%ymm3;	shr $1, %%edi;	add %%r14, %%rcx;  "	// L2 load, L2 store
		"vmovapd %%xmm4, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm4;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm6, %%ymm0, %%ymm5;		vfmadd231pd %%ymm7, %%ymm1, %%ymm12;	shr $1, %%edx;	xor %%rsi, %%r13;  "	// REG ops only
		"vmovapd %%xmm6, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm6;	shl $1, %%edi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm7, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm7;	shl $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm8, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm8;	shl $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vfmadd231pd %%ymm10, %%ymm0, %%ymm9;		vfmadd231pd %%ymm2, %%ymm1, %%ymm13;	shr $1, %%edi;	xor %%rdx, %%r13;  "	// REG ops only
		"vmovapd %%xmm10, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm10;	shr $1, %%esi;	add %%r14, %%rbx;  "	// L1 load, L1 store
		"vmovapd %%xmm2, 64(%%rbx);			vfmadd231pd 32(%%rbx), %%ymm0, %%ymm2;	shr $1, %%edx;	add %%r14, %%rbx;  "	// L1 load, L1 store
		//reset RAM counter
		"sub $1, %%r12;"
		"jnz _work_no_ram_reset_hsw_xeonep_fma_2t;"
		"movabs $68266, %%r12;" 
		"mov %%rax, %%r9;"
		"add $1310720, %%r9;"
		"_work_no_ram_reset_hsw_xeonep_fma_2t:"
		//reset L2-Cache counter
		"sub $1, %%r10;"
		"jnz _work_no_L2_reset_hsw_xeonep_fma_2t;"
		"movabs $30, %%r10;"
		"mov %%rax, %%rcx;"
		"add $16384, %%rcx;"
		"_work_no_L2_reset_hsw_xeonep_fma_2t:"
		//reset L3-Cache counter
		"sub $1, %%r11;"
		"jnz _work_no_L3_reset_hsw_xeonep_fma_2t;"
		"movabs $2730, %%r11;"	
		"mov %%rax, %%r8;"
		"add $131072, %%r8;"
		"_work_no_L3_reset_hsw_xeonep_fma_2t:"
		"mov %%rax, %%rbx;"
		"mov (%%r15), %%r13;"
		"test $1, %%r13;"
		"jnz _work_loop_hsw_xeonep_fma_2t;"
                :
		: "a"(addrMem), "b"(addrHigh) 
                : "%rcx", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15", "%rdx", "%rsi", "%rdi", "%mm0", "%mm1", "%mm2", "%mm3", "%mm4", "%mm5", "%mm6", "%mm7", "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7", "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15"
		);
	return EXIT_SUCCESS;
}

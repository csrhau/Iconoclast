###############################################################################
# FIRESTARTER - A Processor Stress Test Utility
# Copyright (C) 2014 TU Dresden, Center for Information Services and High
# Performance Computing
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Contact: daniel.hackenberg@tu-dresden.de
###############################################################################

# compiler
LINUX_CC=gcc

# the compilerflags
LINUX_C_FLAGS=-O0 -fomit-frame-pointer -Wall -std=c99 -DAFFINITY

# the linkerflags
LINUX_L_FLAGS=-lpthread -lrt -static


default: linux

linux: FIRESTARTER

FIRESTARTER: generic.o  x86.o main.o  work.o  x86.o sse2_functions.o avx_functions.o fma_functions.o fma4_functions.o
	${LINUX_CC} -o FIRESTARTER  generic.o  main.o  work.o  x86.o sse2_functions.o avx_functions.o fma_functions.o fma4_functions.o ${LINUX_L_FLAGS}

generic.o: generic.c cpu.h
	${LINUX_CC} ${LINUX_C_FLAGS} -c generic.c

x86.o: x86.c cpu.h
	${LINUX_CC} ${LINUX_C_FLAGS} -c x86.c

main.o: main.c work.h cpu.h
	${LINUX_CC} ${LINUX_C_FLAGS} -c main.c

work.o: work.c work.h cpu.h
	${LINUX_CC} ${LINUX_C_FLAGS} -c work.c

fma_functions.o: fma_functions.c
	${LINUX_CC} ${LINUX_C_FLAGS} -mfma -mavx  -c fma_functions.c

fma4_functions.o: fma4_functions.c
	${LINUX_CC} ${LINUX_C_FLAGS} -mfma4 -mavx  -c fma4_functions.c

avx_functions.o: avx_functions.c
	${LINUX_CC} ${LINUX_C_FLAGS} -mavx  -c avx_functions.c

sse2_functions.o: sse2_functions.c
	${LINUX_CC} ${LINUX_C_FLAGS} -msse2  -c sse2_functions.c

clean:
	rm -f *.o
	rm -f FIRESTARTER


CC=g++
LDLIBS=-lboost_program_options 
CFLAGS=-std=c++11 -O0 -Wall -Wextra -pedantic -g
BIN=../bin/

EXECUTABLE=$(BIN)candle.x

all: $(EXECUTABLE)

clean:
	rm -f $(EXECUTABLE) *.o

.cc.o:
	$(CC) -c $(CFLAGS) $<

$(EXECUTABLE): main.o 
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

main.o:

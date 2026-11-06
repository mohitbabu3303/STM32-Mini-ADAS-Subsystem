# SIL tests for the Mini ADAS firmware logic.
# Linux/macOS: make test      Windows (MinGW): mingw32-make test
CC      ?= gcc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra -Wpedantic
FW_INC  := ../Mini_ADAS_RTOS/Core/Inc
FW_SRC  := ../Mini_ADAS_RTOS/Core/Src/adas_logic.c ../Mini_ADAS_RTOS/Core/Src/adas_can.c
SIM_SRC := ../simulation/adas_simulation.c

all: adas_test adas_sim

adas_test: test.c $(FW_SRC)
	$(CC) $(CFLAGS) -I$(FW_INC) test.c $(FW_SRC) -o $@

adas_sim: $(SIM_SRC) $(FW_SRC)
	$(CC) $(CFLAGS) -I$(FW_INC) $(SIM_SRC) $(FW_SRC) -o $@

test: adas_test
	./adas_test

clean:
	rm -f adas_test adas_sim adas_test.exe adas_sim.exe

.PHONY: all test clean

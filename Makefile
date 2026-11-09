CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -g
BUILD   = build

# Compilador cruzado para Windows (paquete mingw64-gcc en Fedora,
# gcc-mingw-w64-x86-64 en Debian/Ubuntu).
CC_WIN  = x86_64-w64-mingw32-gcc

SRC_COMMON = common/input.c common/compat.c
SRC_ALGOS  = lru/lru.c clock/clock.c
SRC_SIM    = simulator/simulator.c

.PHONY: all windows test clean

all: $(BUILD)/simulador

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/simulador: main.c $(SRC_COMMON) $(SRC_ALGOS) $(SRC_SIM) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

# Ejecutable para Windows (64 bits). Enlazado estatico: no requiere DLL.
windows: $(BUILD)/simulador.exe

$(BUILD)/simulador.exe: main.c $(SRC_COMMON) $(SRC_ALGOS) $(SRC_SIM) | $(BUILD)
	$(CC_WIN) $(CFLAGS) -static -o $@ $^

$(BUILD)/test_lru: tests/test_lru.c $(SRC_ALGOS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/test_clock: tests/test_clock.c $(SRC_ALGOS) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/test_simulator: tests/test_simulator.c $(SRC_COMMON) $(SRC_ALGOS) $(SRC_SIM) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

test: $(BUILD)/test_lru $(BUILD)/test_clock $(BUILD)/test_simulator
	./$(BUILD)/test_lru
	./$(BUILD)/test_clock
	./$(BUILD)/test_simulator

clean:
	rm -f $(BUILD)/simulador $(BUILD)/simulador.exe $(BUILD)/test_clock $(BUILD)/test_simulator

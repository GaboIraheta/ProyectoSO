CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -g
BUILD   = build

SRC_COMMON = common/input.c
SRC_ALGOS  = lru/lru.c clock/clock.c
SRC_SIM    = simulator/simulator.c

.PHONY: all test clean

all: $(BUILD)/simulador

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/simulador: main.c $(SRC_COMMON) $(SRC_ALGOS) $(SRC_SIM) | $(BUILD)
	$(CC) $(CFLAGS) -o $@ $^

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
	rm -f $(BUILD)/simulador $(BUILD)/test_clock $(BUILD)/test_simulator

PROJECT := kiss-tnc2
BUILD := build
HOST_BUILD := $(BUILD)/host

SDCC ?= $(shell command -v sdcc 2>/dev/null || test ! -x "$(HOME)/bin/sdcc" || printf '%s' "$(HOME)/bin/sdcc")
SDAS ?= $(shell command -v sdasz80 2>/dev/null || test ! -x "$(HOME)/bin/sdasz80" || printf '%s' "$(HOME)/bin/sdasz80")
MAKEBIN ?= $(shell command -v makebin 2>/dev/null || test ! -x "$(HOME)/bin/makebin" || printf '%s' "$(HOME)/bin/makebin")
CC ?= cc
PYTHON ?= python3

CPPFLAGS := -Iinclude
SDCCFLAGS := -mz80 --std-c11 --opt-code-speed --max-allocs-per-node 100000 $(CPPFLAGS)
LDFLAGS := -mz80 --nostdlib --no-std-crt0 --code-loc 0x0120 --data-loc 0x8050
HOST_CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O2 $(CPPFLAGS) -Itests -DHOST_TEST

C_SOURCES := src/state.c src/hardware.c src/buffers.c src/kiss.c src/modem.c \
	src/interrupts.c src/main.c
ASM_SOURCES := src/startup.s src/isr_stubs.s
HEADERS := include/tnc2.h
HOST_SOURCES := tests/test_firmware.c tests/hardware_mock.c src/state.c \
	src/buffers.c src/kiss.c src/modem.c src/interrupts.c
C_OBJECTS := $(patsubst src/%.c,$(BUILD)/%.rel,$(C_SOURCES))
ASM_OBJECTS := $(patsubst src/%.s,$(BUILD)/%.rel,$(ASM_SOURCES))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

.PHONY: all clean check toolchain require-sdcc layout

all: require-sdcc $(BUILD)/$(PROJECT).bin layout

require-sdcc:
	@if test -z "$(SDCC)" || test -z "$(SDAS)" || test -z "$(MAKEBIN)"; then \
		echo "SDCC Z80 tools were not found."; \
		echo "Run 'make toolchain', then add \$$HOME/bin to PATH."; \
		exit 2; \
	fi

toolchain:
	./tools/build-sdcc.sh

$(BUILD):
	mkdir -p $@

$(BUILD)/%.rel: src/%.c $(HEADERS) | $(BUILD)
	$(SDCC) $(SDCCFLAGS) -c -o $@ $<

$(BUILD)/%.rel: src/%.s | $(BUILD)
	$(SDAS) -plosgff -o $@ $<

$(BUILD)/$(PROJECT).ihx: $(OBJECTS) Makefile
	$(SDCC) $(LDFLAGS) -Wl-m -o $@ $(OBJECTS)

$(BUILD)/$(PROJECT).bin: $(BUILD)/$(PROJECT).ihx
	$(MAKEBIN) -s 32768 $< $@

layout: $(BUILD)/$(PROJECT).ihx
	$(PYTHON) tools/check-layout.py $< $(BUILD)/$(PROJECT).map

$(HOST_BUILD):
	mkdir -p $@

$(HOST_BUILD)/test_firmware: $(HOST_SOURCES) tests/hardware_mock.h $(HEADERS) | $(HOST_BUILD)
	$(CC) $(HOST_CFLAGS) -o $@ $(HOST_SOURCES)

check: $(HOST_BUILD)/test_firmware
	$<
	@if test -n "$(SDCC)" && test -n "$(SDAS)" && test -n "$(MAKEBIN)"; then \
		$(MAKE) --no-print-directory all; \
	else \
		echo "Host tests passed; skipping ROM build because SDCC is absent."; \
		echo "Run 'make toolchain' to enable complete verification."; \
	fi

clean:
	rm -rf $(BUILD)

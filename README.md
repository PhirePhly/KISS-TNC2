# KISS-TNC2

KISS-only firmware for a stock TAPR TNC-2, rewritten in C for the SDCC
Z80 compiler. `KISS-TNC.asm` is retained as the behavioral reference.

## Hardware target

- Z80 at 2.5 MHz
- 32 KiB ROM at `0x0000` and 32 KiB RAM at `0x8000`
- Z80 SIO channel A (HDLC modem) at `0xDC`/`0xDD`
- Z80 SIO channel B (asynchronous host) at `0xDE`/`0xDF`
- SIO channel B sync/hunt input clocked to provide the original 100 Hz timer

A cold start enters KISS mode directly. There is no terminal or command mode.
The firmware preserves KISS data and parameter commands 0 through 5.
Command 6 separately enables or disables software DCD, hardware DCD, and CTS
flow control. [KISS.md](KISS.md) documents every command, argument, and
default.

Half-duplex transmission defers to software DCD by default. The channel is
busy while the SIO has left HDLC hunt mode, so unsquelched audio does not
hold off transmission merely because hardware DCD is asserted. `6 2 1` enables
the hardware DCD check as well, and `6 1 0` disables software DCD.

## Build

GNU Make, a C/C++ compiler, Flex, Bison, and standard development tools are
needed to build SDCC. On AlmaLinux, install the pinned SDCC 4.6.0 toolchain
without root access:

```sh
make toolchain
export PATH="$HOME/bin:$PATH"
```

The bootstrap downloads checksum-pinned SDCC and, when system Boost headers
are unavailable, checksum-pinned Boost headers. SDCC is built from source and
installed below `$HOME` (`~/bin` and `~/share`).

Build and test:

```sh
make
make check
```

If SDCC is absent, `make` explains how to install it and `make check` still
runs the native protocol tests. Build products are placed in `build/`:

- `kiss-tnc2.ihx` — Intel HEX image
- `kiss-tnc2.bin` — 32 KiB ROM image
- `kiss-tnc2.map` — linker map used by layout checks

The build rejects ROM code at or above `0x8000`, misplaced fixed RAM objects,
invalid IM2 vectors, or missing IM2/RETI instructions.

## Firmware organization

- `src/startup.s`, `src/isr_stubs.s`: reset, IM2 vectors, ABI-safe interrupt
  entry/exit, and the few instructions C cannot express directly
- `src/hardware.c`: SIO, LEDs, PTT, and hardware extension
- `src/buffers.c`: fixed 128-byte buffer pool and frame queues
- `src/kiss.c`: streaming KISS decoder and parameter commands
- `src/modem.c`: receive handling, CSMA/persistence, PTT, and foreground work
- `src/interrupts.c`: eight SIO interrupt handlers

No heap or C runtime startup is used. RAM has a fixed layout: state at
`0x8000`, two 255-entry queues at `0x8100` and `0x8300`, and 244 128-byte
buffers filling `0x8500` through `0xFEFF`. The top 256 bytes are reserved for
the downward-growing stack.

## Hardware smoke test

1. Program `build/kiss-tnc2.bin` into a 32 KiB ROM and cold reset.
2. Confirm both LEDs perform the startup sequence and finish off.
3. At the host serial rate configured by the TNC-2 clocking, send a KISS data
   frame and verify TX delay, PTT assertion, HDLC transmission, tail time, and
   PTT release.
4. Receive valid and deliberately bad-CRC HDLC frames; only the valid frame
   should appear on the host with KISS port byte zero and correct escaping.
5. Verify persistence and DCD defer transmission in half duplex, then verify
   command 5 full-duplex operation.
6. Exercise long frames and back-to-back traffic to check buffer exhaustion
   recovery and host output queueing.

The native tests cannot prove SIO timing. Final qualification must therefore
include sustained 9600-baud host traffic and 1200-baud modem traffic on a
2.5 MHz unit while checking for receive overruns and transmit underruns.

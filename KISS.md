# KISS command reference

The firmware speaks KISS on the asynchronous host port and comes up in KISS
mode after reset. There is no command-mode exit. A byte whose low nibble is
7–15, including `0xFF`, is discarded.

## Framing

| Byte | Value | Meaning |
| --- | --- | --- |
| `FEND` | `0xC0` | Frame delimiter |
| `FESC` | `0xDB` | Escape introducer |
| `TFEND` | `0xDC` | Escaped `FEND` |
| `TFESC` | `0xDD` | Escaped `FESC` |

A frame is `FEND`, one command byte, optional data, and `FEND`. Leading `FEND`
bytes are optional: reset and the end of a data frame leave the parser ready
for the next command byte, and extra `FEND` bytes are ignored. A parameter or
hardware command returns to `FEND` hunt after its argument.

Inside command 0 data, `FESC TFEND` decodes to `0xC0` and `FESC TFESC` decodes
to `0xDB`. Any other byte after `FESC` is discarded and data collection
continues. If no buffer is available, the frame is discarded. A framing error
also discards the current frame.

The command is the low nibble. The high nibble, conventionally the KISS port,
is ignored, so every accepted command acts on modem port 0.

Frames sent to the host use the same escaping. Each received HDLC frame is
wrapped as `FEND`, port byte `0x00`, frame data, `FEND`. The final HDLC CRC
byte retained by the SIO is removed before transmission.

## Commands

| Command | Name | Argument |
| --- | --- | --- |
| 0 | Data | HDLC payload |
| 1 | TX delay | One byte, 10 ms units |
| 2 | Persistence | One byte, 0–255 |
| 3 | Slot time | One byte, 10 ms units |
| 4 | TX tail | One byte, number of closing HDLC flags |
| 5 | Full duplex | One byte |
| 6 | Set hardware | Feature byte, then `0` or `1` |

### 0: Data

The argument is queued for HDLC transmission. The firmware appends and later
removes one internal sentinel byte, so it is not transmitted. A frame with no
data bytes is ignored.

### 1: TX delay

Time between asserting PTT and sending the first byte. The default is 30,
or 300 ms. The timer has 10 ms resolution.

### 2: Persistence

The default is 63. In half or full duplex, a pending frame is eligible when
`persistence` is greater than or equal to a value derived from the Z80 refresh
register. That value is even and ranges from 0 through 254. A value of 255
always passes; smaller values defer for one slot time before trying again.

### 3: Slot time

Delay used after losing persistence or finding the channel busy. The default
is 10, or 100 ms.

### 4: TX tail

Number of HDLC flags (`0x7E`) sent after the closing CRC before PTT is
released. The default is 2. Each flag is eight bits on the 1200 Hz clock, so
this count is independent of the 10 ms timer used by TX delay and slot time.
A value of 0 releases PTT as soon as the SIO finishes the frame and begins
sending flags.

### 5: Full duplex

`0x00` selects half duplex, which is the default. Any other value selects full
duplex. Full duplex skips the carrier check but still applies persistence and
slot timing.

### 6: Set hardware

Command 6 takes a feature selector and then `0` to disable that feature or
`1` to enable it. Any other feature or value discards the command. The two
DCD features are independent.

| Feature | Name | `1` enables | `0` disables |
| --- | --- | --- | --- |
| 1 | Software DCD | HDLC-hunt carrier check; this is the default | No software carrier check |
| 2 | Hardware DCD | SIO DCD carrier check | No hardware carrier check; this is the default |
| 3 | CTS flow control | Wait for modem CTS after TX delay | Send without waiting for CTS; this is the default |

For example, `6 1 1` enables software DCD, `6 1 0` disables it, and `6 2 1`
enables hardware DCD. Both DCD features may be enabled together.

Software DCD reports busy while the SIO has left HDLC hunt mode. It detects
flag synchronization rather than validating a complete frame, but it does not
follow an energy-only hardware carrier detector. This allows unsquelched audio
when hardware DCD remains asserted.

## Defaults

| Setting | Default | Equivalent command |
| --- | --- | --- |
| TX delay | 300 ms | `1 30` |
| Persistence | 63 | `2 63` |
| Slot time | 100 ms | `3 10` |
| TX tail | 2 HDLC flags | `4 2` |
| Duplex | Half | `5 0` |
| Software DCD | On | `6 1 1` |
| Hardware DCD | Off | `6 2 0` |
| CTS flow control | Off | `6 3 0` |

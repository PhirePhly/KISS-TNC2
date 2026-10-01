#!/usr/bin/env python3
"""Reject ROM images that violate the fixed TNC-2 memory contract."""

from pathlib import Path
import re
import sys


def load_ihx(path: Path) -> dict[int, int]:
    image: dict[int, int] = {}
    upper = 0
    for number, raw in enumerate(path.read_text().splitlines(), 1):
        if not raw.startswith(":"):
            raise ValueError(f"{path}:{number}: invalid Intel HEX record")
        record = bytes.fromhex(raw[1:])
        if sum(record) & 0xFF:
            raise ValueError(f"{path}:{number}: checksum mismatch")
        count = record[0]
        address = (record[1] << 8) | record[2]
        kind = record[3]
        data = record[4 : 4 + count]
        if kind == 0:
            for offset, byte in enumerate(data):
                image[upper + address + offset] = byte
        elif kind == 4:
            upper = int.from_bytes(data, "big") << 16
        elif kind == 1:
            break
    return image


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"layout check failed: {message}")


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: check-layout.py firmware.ihx firmware.map")
    image = load_ihx(Path(sys.argv[1]))
    require(image, "ROM image is empty")
    require(max(image) < 0x8000, "ROM overlaps RAM at 0x8000")
    require([image.get(i) for i in range(4)] == [0xF3, 0x31, 0x00, 0x00],
            "reset must begin DI; LD SP,0")

    vectors = []
    for address in range(0x100, 0x110, 2):
        target = image.get(address, 0xFF) | (image.get(address + 1, 0xFF) << 8)
        vectors.append(target)
    require(all(0x120 <= target < 0x8000 for target in vectors),
            "all eight IM2 vectors must address ROM stubs")
    require(len(set(vectors)) == 8, "IM2 vectors must be distinct")

    rom = bytes(image.get(i, 0xFF) for i in range(max(image) + 1))
    require(b"\xED\x5E" in rom, "IM 2 instruction not found")
    require(b"\xED\x4D" in rom, "RETI instruction not found")

    map_text = Path(sys.argv[2]).read_text()
    for symbol, address in (
        ("_g_state", "00008000"),
        ("_g_tx_queue", "00008100"),
        ("_g_out_queue", "00008300"),
        ("_g_buffers", "00008500"),
    ):
        require(symbol in map_text, f"{symbol} missing from linker map")
        pattern = rf"{address}.*{re.escape(symbol)}|{re.escape(symbol)}.*{address}"
        require(re.search(pattern, map_text, re.IGNORECASE) is not None,
                f"{symbol} is not fixed at 0x{address[-4:]}")

    print(f"ROM layout: PASS ({max(image) + 1} bytes address span)")


if __name__ == "__main__":
    main()

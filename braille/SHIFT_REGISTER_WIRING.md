# SN74HC595 Dot-Plane Wiring Guide

This project is configured for a **dot-plane architecture**:

- Each `SN74HC595` controls **one Braille dot line** across all tiles.
- Chips are chained to support multiple dots.
- Each output bit (`Q0..Q7`) on a chip represents a tile position.

## Current Firmware Configuration

From `braille/src/main.cpp`:

- `TILE_COUNT = 4`
- `DOT_CHIP_COUNT = 6` (6-dot Braille)
- `SR_DATA_PIN = 11`
- `SR_CLOCK_PIN = 12`
- `SR_LATCH_PIN = 8`

If you change tile count in firmware, update Python `tile_count` to match.

## Dot Bit Mapping

Braille dots are encoded in a tile pattern byte using this mapping:

- Dot 1 -> bit 0
- Dot 2 -> bit 1
- Dot 3 -> bit 2
- Dot 4 -> bit 4
- Dot 5 -> bit 5
- Dot 6 -> bit 6

(`bit 3` and `bit 7` are used for dots 7 and 8 in 8-dot mode, but this wiring is currently 6-dot.)

## Recommended Chip Chain Order

The firmware shifts bytes from highest chip index down to lowest:

- Shift order: `chip 5`, `chip 4`, `chip 3`, `chip 2`, `chip 1`, `chip 0`

Recommended logical mapping:

- `chip 0` = Dot 1 plane
- `chip 1` = Dot 2 plane
- `chip 2` = Dot 3 plane
- `chip 3` = Dot 4 plane
- `chip 4` = Dot 5 plane
- `chip 5` = Dot 6 plane

This means:

- The **last physical chip in the shift chain** (closest to MCU serial input) receives `chip 5`.
- The **first logical chip (`chip 0`)** should be positioned accordingly in your chain.

If your physical chain is reversed, either rewire or adjust firmware shift order/mapping.

## Tile Bit Mapping Per Chip

For each chip byte:

- `Q0` = Tile 0
- `Q1` = Tile 1
- `Q2` = Tile 2
- `Q3` = Tile 3
- `Q4..Q7` = unused when `TILE_COUNT = 4`

So each chip controls one dot across all visible tiles.

## Serial Protocol

### Preferred Multi-Tile Command

Use:

`FRAME:XX,YY,ZZ,...`

Where each hex byte is a tile pattern (`TILE_COUNT` bytes total).

Example for 4 tiles:

`FRAME:01,03,00,40`

Interpretation:

- Tile 0: `0x01` (dot 1)
- Tile 1: `0x03` (dots 1,2)
- Tile 2: `0x00` (blank)
- Tile 3: `0x40` (dot 6)

### Legacy Commands (still supported)

- `P:XX` -> sets tile 0 pattern
- `T:<tile>:<hex>` -> sets one tile pattern
- `CLEAR` -> clears all tiles

## Python Sender Notes

`braille_converter/arduino_integration/braille_to_arduino.py` includes:

- `send_tile_patterns(...)` for `FRAME` messages
- `send_text_multi_tile(...)` to send text in tile windows
- Interactive command: `tiles <text>`

Make sure:

- Python `tile_count` matches firmware `TILE_COUNT`.


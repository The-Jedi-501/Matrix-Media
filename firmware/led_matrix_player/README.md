# `led_matrix_player.ino`

This is the ESP32 firmware — the half of the project that actually drives the hardware. It listens over serial for data coming from the Python client on my PC, and renders one of three screens on a HUB75 LED matrix (plus a secondary SSD1306 LCD for text/progress info).

This file started life as an example sketch for the matrix library and got built out from there — so you'll see two "eras" of comments in it: dense catch-up notes from when I was re-learning Arduino after a break, and newer notes as features got added. I'm keeping both because the messy version is closer to how I actually think through problems than a cleaned-up final draft would be.

## Wiring

### HUB75 Matrix

| Signal | GPIO |
|---|---|
| R1 | 32 |
| G1 | 33 |
| B1 | 15 |
| R2 | 25 |
| G2 | 26 |
| B2 | 2 |
| CH_A | 27 |
| CH_B | 4 |
| CH_C | 5 |
| CH_D | 16 |
| CH_E | 14 |
| CLK | 12 |
| LAT | 17 |
| OE | 13 |

`CH_E` is only needed for 1/32 scan panels like my 64×64 — a 32-tall panel doesn't need it wired at all.

### Push Button (screen switching)

- `PB_NO_Forward` → GPIO 35
- Normally open, external 10kΩ pull-down resistor required (35 is an input-only pin — no internal pull resistor available)
- Reads `HIGH` on press, idles `LOW`

### LCD (SSD1306, 128×64, I2C)

- SDA → GPIO 21, SCL → GPIO 22
- Address `0x3C` (falls back gracefully — see [Startup / Safety Checks](#startup--safety-checks))

## The Three Screens

Screen switching is controlled by `Counter_PB_NO_Screen_Value` (1, 2, or 3), advanced by the push button and wrapped back to 1 once it exceeds `Max_Screens_Made`.

1. **Album art** — draws `imageBuffer` straight to the panel, pixel by pixel, no transformation.
2. **Spinning CD** — same `imageBuffer`, but masked into concentric rings (hole → hub → art → outer ring) and rotated per frame. This is the one with actual math behind it (see below).
3. **Clock** — skips the pixel loop entirely and just prints `HH:MM AM/PM` in large text using the matrix's built-in text renderer.

The LCD runs independently of all three — it always shows the progress bar, song/artist text, and screen indicator (`drawScreen_1()`), regardless of which matrix screen is active.

## Serial Protocol (this file's side of it)

The parser is a state machine using boolean flags (`readingTag`, `readingText`, `reading_POS`, `reading_Time`, `readingPixels`) so the same `loop()` byte-reading code can be reused for every tag type instead of duplicating the read loop four times.

Flow: sit in `readingTag` mode building up a string character by character until a `:` shows up. Whatever was built (`ART`, `TXT`, `POS`, `TIM`) decides which mode to switch into next:

| Tag | Mode entered | Terminator | Handler |
|---|---|---|---|
| `ART:` | `readingPixels` | fixed byte count (`expectedBytes`) | `Draw_Art_Framework()` once full |
| `TXT:` | `readingText` | `\n` | `processTitleArtist()` |
| `POS:` | `reading_POS` | `\n` | `process_Time_For_Piece()` |
| `TIM:` | `reading_Time` | `\n` | `process_Time_Matrix()` |

Pixel data is the odd one out — it doesn't use a terminator character at all, since raw image bytes could coincidentally contain any byte value including `\n`. Instead it just counts bytes against `expectedBytes` (currently `12288`, i.e. 64×64×3) and switches back to tag-reading once it hits that count.

Text-based tags (`TXT`, `POS`, `TIM`) use `|` and `~` as internal separators once inside the payload — e.g. `POS:` payload splits into `current_POS | current_END_TIME ~ current_Status`.

### Serial stall safety net

If we're mid-parse (any mode other than idle) and 5 seconds (`serialTimeout`) pass without a new byte, the whole parser gets force-reset back to `readingTag = true` and every partial buffer gets cleared. This exists because a dropped/partial packet used to permanently wedge the parser in a mode where it would never see another full tag.

## CD Spin Math (`Draw_Art_Framework()`, screen 2)

This is the part of the file with actual geometry in it, so worth explaining properly:

- **Ring mask**: for each pixel, compute squared distance from center (`dx*dx + dy*dy` — skipping the square root since we're just comparing against squared radii anyway). Four thresholds (`Hole_Ring_Radius`, `Inner_Ring_Radius`, `Art_Radius`, `Outer_Ring_Radius`) sort each pixel into hole / hub / album art / outer ring / background.
- **Rotation**: standard 2D rotation matrix, but applied *backward* — for each destination pixel `(x, y)` on screen, it computes which source pixel in `imageBuffer` should be sampled, rather than pushing each source pixel forward into a destination (which leaves gaps). `cosA`/`sinA` are computed once per frame outside the pixel loop, not per-pixel, since they don't change within a frame.
- **Sign convention**: rotation uses `-spinAngle` deliberately — screen-space rotation and "which source pixel maps here" rotation run in opposite directions, so negating keeps the visual spin direction consistent.
- **Spin gating**: `spinAngle` only advances when `Status_Num == 1` (playback is actively playing), so the CD visually stops spinning on pause without needing a separate "paused" render path.
- **Sampling**: nearest-pixel (`round()`), not interpolated — simplest option, and at this resolution the difference isn't visible.

## LCD Marquee Scrolling

`Text_Logic()` + `Slider_Gaurd()` work as a pair per text field (title, artist each get their own call):

- `Text_Logic()` measures the string width. If it fits in the available space, it's drawn once, statically. If it's too wide, it's drawn character-by-character at computed x-offsets — this is what makes the marquee effect possible instead of drawing the whole string as one block.
- `Slider_Gaurd()` owns the actual scroll timing: pause at the start position for `holdDuration`, then slide left one pixel per scroll tick, then snap back off-screen right once the string has fully cleared, so it loops.

These both run on a `millis()`-gated interval (`scrollInterval`) inside `loop()`, independent of whatever the matrix side is doing.

## Push Button Debounce

Standard two-stage debounce: track the raw pin state, reset a timer any time the raw reading changes, and only promote the raw reading to the "trusted" (`_debounced`) value once it's held steady longer than `debounceDelay` (30ms). Screen advance only fires on the debounced value going `HIGH`, not on every raw wiggle.

## Startup / Safety Checks

- `dma_display->begin()` failing prints a loud `!KABOOM!` warning to serial rather than failing silently.
- LCD init (`display.begin()`) is wrapped in a check too — if the SSD1306 doesn't respond at `0x3C`, the code sets `lcdReady = false` and the matrix keeps working without it, instead of the whole sketch hanging.
- Startup runs a quick RGB → white → black fill sequence on the matrix, mostly as a "yes, wiring and driver config are actually working" sanity check before anything data-driven happens.

## Known Issues / Quirks in This File

- **Ghost pause/play artifact in CD mode**: raising `SERIAL_SIZE_RX` (the RX buffer size) was an attempt to fix intermittent visual glitches tied to serial byte timing, on the theory that the ESP32 wasn't reading bytes off the wire fast enough during CD mode and some were getting dropped/delayed. It didn't fully resolve it — documented inline as an accepted, not-yet-solved issue.
- **R/G/B color channel mismatch**: comments note green and blue appeared swapped despite wiring matching the diagram — pin definitions have "was X, now Y" annotations showing where values got flipped by trial and error rather than a clean root-cause fix.
- **No struct/class organization yet**: variables are flat globals grouped by comment headers (`MATRIX —`, `LCD —`, etc.) rather than bundled into structs. Flagged in the code itself as a cleanup target once the feature set stabilizes.
- **Multi-panel chaining code is present but unused** — commented-out `PIN_E` / `chain_length` configuration is left in place as a reference for when/if a second panel gets added, per the roadmap.

---

*Companion to the top-level [README](../../README.md) — this one's about what this specific file does, not why the project exists.*

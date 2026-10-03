# led_matrix_player.ino

This is the ESP32 firmware, the part that actually drives the hardware. It listens over serial for data coming from the Python client running on my PC, and renders one of three screens on a HUB75 LED matrix, plus a secondary SSD1306 LCD that shows text and progress info.

This file started as an example sketch for the matrix library and got built out from there, so there are two "eras" of comments in it. Some are dense catch up notes from when I was re learning Arduino after a break, and some are newer as features got added. I kept both because the messy version is honestly closer to how I actually think through problems than a cleaned up final draft would be.

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

CH_E is only needed for 1/32 scan panels like my 64x64. A 32 tall panel doesn't need it wired at all.

### Push Button (screen switching)

- PB_NO_Forward on GPIO 35
- Normally open, needs an external 10k ohm pull down resistor since 35 is an input only pin with no internal pull resistor
- Reads HIGH on press, idles LOW

### LCD (SSD1306, 128x64, I2C)

- SDA on GPIO 21, SCL on GPIO 22
- Address 0x3C (falls back gracefully, see the startup section below)

## The Three Screens

Screen switching is controlled by `Counter_PB_NO_Screen_Value` (1, 2, or 3). The push button advances it, and it wraps back to 1 once it goes past `Max_Screens_Made`.

1. **Album art.** Draws `imageBuffer` straight to the panel, pixel by pixel, no transformation.
2. **Spinning CD.** Same `imageBuffer`, but masked into concentric rings (hole, hub, art, outer ring) and rotated each frame. This is the screen with actual math behind it, more on that below.
3. **Clock.** Skips the pixel loop entirely and just prints HH:MM AM/PM in large text using the matrix's built in text renderer.

The LCD runs independently of all three. It always shows the progress bar, song and artist text, and the screen indicator through `drawScreen_1()`, no matter which matrix screen is active.

## Serial Protocol (this file's side of it)

The parser is basically a state machine made of boolean flags (`readingTag`, `readingText`, `reading_POS`, `reading_Time`, `readingPixels`), so the same byte reading code in `loop()` can get reused for every tag type instead of writing four separate read loops.

Here's the flow. Sit in `readingTag` mode building up a string character by character until a `:` shows up. Whatever got built (`ART`, `TXT`, `POS`, `TIM`) decides what mode comes next:

| Tag | Mode entered | Terminator | Handler |
|---|---|---|---|
| ART: | readingPixels | fixed byte count (expectedBytes) | Draw_Art_Framework() once full |
| TXT: | readingText | \n | processTitleArtist() |
| POS: | reading_POS | \n | process_Time_For_Piece() |
| TIM: | reading_Time | \n | process_Time_Matrix() |

Pixel data is the odd one out, it doesn't use a terminator character at all, since raw image bytes could technically contain any byte value including `\n`. Instead it just counts bytes against `expectedBytes` (currently 12288, which is 64 x 64 x 3) and flips back to tag reading once it hits that count.

The text based tags (TXT, POS, TIM) use `|` and `~` as internal separators once you're inside the payload. So a POS: payload splits out into current_POS, current_END_TIME, and current_Status.

### Serial stall safety net

If we're mid parse (any mode besides idle) and 5 seconds go by (serialTimeout) without a new byte showing up, the whole parser force resets back to readingTag = true and every partial buffer gets wiped. This exists because a dropped or partial packet used to permanently jam the parser in a mode where it would never see a full tag again.

## CD Spin Math (Draw_Art_Framework, screen 2)

This is the part of the file with actual geometry in it so it's worth walking through properly.

- **Ring mask.** For each pixel, I compute the squared distance from center (dx*dx + dy*dy, skipping the square root since I'm only comparing against squared radii anyway). Four thresholds (Hole_Ring_Radius, Inner_Ring_Radius, Art_Radius, Outer_Ring_Radius) sort each pixel into hole, hub, album art, outer ring, or background.
- **Rotation.** Standard 2D rotation matrix, but applied backward. For each pixel on screen, it figures out which source pixel in imageBuffer should get sampled, instead of pushing each source pixel forward into a destination, which leaves gaps. cosA and sinA get computed once per frame outside the pixel loop, not once per pixel, since they don't change within a frame.
- **Sign convention.** The rotation uses -spinAngle on purpose. Screen space rotation and "which source pixel maps here" rotation run in opposite directions, so negating it keeps the spin looking like it's actually going the direction I want.
- **Spin gating.** spinAngle only advances when Status_Num == 1, meaning something is actively playing. So the CD visually stops spinning on pause without needing a whole separate paused render path.
- **Sampling.** Nearest pixel (round()), not interpolated. Simplest option, and at this resolution you honestly can't tell the difference.

## LCD Marquee Scrolling

Text_Logic() and Slider_Gaurd() work as a pair, one call per text field (title and artist each get their own):

- Text_Logic() measures the string width. If it fits in the available space it just draws once, statically. If it's too wide, it draws it character by character at computed x offsets, which is what actually makes the marquee effect possible instead of drawing the whole string as one block.
- Slider_Gaurd() owns the actual scroll timing. Pause at the start position for holdDuration, then slide left one pixel per scroll tick, then snap back off screen to the right once the string has fully cleared so it loops.

Both of these run on a millis() gated interval (scrollInterval) inside loop(), separate from whatever the matrix side is doing.

## Push Button Debounce

Standard two stage debounce. Track the raw pin state, reset a timer any time the raw reading changes, and only promote the raw reading to the trusted (_debounced) value once it's held steady longer than debounceDelay (30ms). The screen only advances on the debounced value going HIGH, not on every raw wiggle from the switch bouncing.

## Startup / Safety Checks

- If dma_display->begin() fails, it prints a loud "!KABOOM!" warning to serial instead of just silently not working.
- LCD init (display.begin()) is wrapped in a check too. If the SSD1306 doesn't respond at 0x3C, the code sets lcdReady = false and the matrix keeps working fine without it, instead of the whole sketch just hanging.
- On startup it runs a quick RGB to white to black fill on the matrix, mostly as a "yes, wiring and driver config are actually working" sanity check before anything data driven happens.

## Known Issues / Quirks in This File

- **Ghost pause/play artifact in CD mode.** Raising SERIAL_SIZE_RX (the RX buffer size) was an attempt to fix intermittent visual glitches tied to serial byte timing, on the theory the ESP32 wasn't reading bytes off the wire fast enough during CD mode and some were getting dropped or delayed. It didn't fully fix it. Documented inline as an accepted, not yet solved issue.
- **R/G/B color channel mismatch.** Comments note green and blue appeared swapped even though the wiring matched the diagram. Pin definitions have "was X, now Y" notes showing where values got flipped through trial and error rather than a clean root cause fix.
- **No struct or class organization yet.** Variables are flat globals grouped by comment headers (MATRIX, LCD, etc) instead of bundled into structs. Flagged in the code itself as something to clean up once the feature set settles down.
- **Multi panel chaining code is present but unused.** The commented out PIN_E and chain_length config is left in place as a reference for later, if a second panel ever gets added.

---

*Companion to the top level [README](../../README.md). This one's about what this specific file does, not why the project exists.*

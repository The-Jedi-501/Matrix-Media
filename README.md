# Matrix-Media

A hardware/software media display system that pulls what's currently playing on my PC and shows it on an ESP32-driven LED matrix — album art, a spinning CD animation, a clock, and more, all switchable with a physical button.

---

## ⚠️ A Note Before You Read the Code

I'm a sophomore computer engineering student (dual-majoring with EE — haven't taken EE coursework yet). This is a self-directed summer learning project, not a polished commercial product, and I built it specifically to *actually learn* embedded systems and hardware-adjacent engineering, not just to get something blinking.

That means:

- I used **Google, Claude, YouTube tutorials, and a bunch of forum threads** while building this. I don't think that's something to hide — it's how I learned. But I made it a rule for myself to never copy-paste something I couldn't explain. If I can't walk you through *why* a piece of code does what it does, it doesn't go in.
- You'll find **heavily-commented "Notes" versions** of some files alongside the production code. Those aren't clutter — they're my own learning trail, kept on purpose so I (or anyone else) can see the reasoning, not just the result.
- Some parts of this project are still broken or unfinished (see [Known Issues](#known-issues--in-progress) below). I'm leaving that visible rather than cleaning it up cosmetically, because the debugging process is part of the point.
- If you're a reviewer, recruiter, or fellow student: ask me about any line of this code. That's the actual goal here — not just "it works," but "I know why it works."

---

## What It Does

A Python script on my PC reads what's currently playing (via the Windows Runtime / System Media Transport Controls APIs), grabs the album art, processes it with Pillow, and streams it over serial to an ESP32. The ESP32 drives a HUB75 RGB LED matrix and renders:

- **Album art display** — synced live to whatever's playing
- **Spinning CD animation** — backward-mapped 2D rotation with a five-zone concentric circle mask, only animating while playback is active
- **Clock screen** — HHMMSS display, NTP-synced over WiFi
- **Physical screen switching** — a debounced push-button cycles between display modes

## Why I Built This

Three-project arc I set for myself this year:
1. **This LED matrix media player** — software/firmware integration, protocol design, real-time rendering
2. **A 10×10×10 LED cube** — the real EE challenge, next up
3. **A speaker build** — sequenced last, after I've actually had circuits coursework

This project was meant to be the "software-heavy, hardware-adjacent" entry point before jumping into things that need real EE fundamentals.

## Architecture

```
PC (Python)                          ESP32 (Arduino C++)
─────────────                        ───────────────────
WinRT/SMTC API   ──┐
  ↓ playback info  │
Pillow             │   Custom serial   HUB75 matrix driver
  ↓ album art       ├──  protocol   ──►  (I2S-DMA library)
Serial (pyserial)  ─┘   (tag-based)      → CD animation
                                          → Clock screen
                                          → Screen switching
```

### The Protocol

A custom tag-based serial protocol, designed on purpose to avoid delimiter-collision bugs:

- Fixed-length binary framing for pixel data (3,072 bytes per frame)
- Terminator-based framing for text tags
- Lesson learned the hard way: a `POS:` tag using raw timedelta strings broke the parser because the colons in `HH:MM:SS` collided with the tag's own delimiter. Fix is documented in `protocol.md` (currently deferred, not yet patched).

Full spec lives in [`protocol.md`](./protocol.md).

## Hardware

- ESP32
- 64×32 HUB75 RGB LED matrix (primary, currently working)
- Waveshare 64×64 P2.5 matrix panel (see Known Issues — not stable yet)
- SSD1306 OLED (128×64, I2C)
- Arduino Nano, servo motors, gyro/accelerometer, shift registers — reserved for future features

## Software / Libraries

- **Firmware:** Arduino C++, [ESP32-HUB75-MatrixPanel-I2S-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA)
- **PC client:** Python, `winsdk` (WinRT bindings), Pillow, `pyserial`

## Repo Structure

```
Matrix-Media/
├── firmware/
│   └── led_matrix_player/     # ESP32 sketch
├── pc-client/                 # Python playback reader + serial sender
├── protocol.md                # Serial protocol spec
└── README.md
```

## Known Issues / In Progress

- **Waveshare 64×64 panel**: two of four horizontal bands render dark/wrong despite correct E-pin wiring and multiple driver/scan config attempts. Draft GitHub issue prepared for the library maintainer. Currently running the demo on the old 32-tall panel instead.
- **Old 32-tall panel**: developed a stuck-green bottom half, likely from physical disturbance of the G2 line during connector handling.
- **`pc-client/` cleanup**: a few loose scratch files (`Notes.py`, `IDK.py`, etc.) still need to be folded in or removed.
- **`POS:` tag desync**: known parser bug from colons in timedelta strings — root cause identified, fix not yet applied.

## Roadmap

- [ ] Integrate clock screen into main firmware
- [ ] Static background image screen (local file, reusing the `ART:` pipeline)
- [ ] 74HC595 shift register for LED mode indicators
- [ ] Song progress bar (`BAR:` tag, SMTC timeline data)
- [ ] Volume display via `pycaw`
- [ ] Gesture control (MediaPipe + `pyautogui`)
- [ ] WiFi, then Bluetooth/BLE transport

**Deferred to a later phase:** Raspberry Pi migration, 3D-printed enclosure, second matrix chaining.

## What I Learned

A few things worth calling out, because they weren't obvious to me going in:

- `dir()` is genuinely useful documentation when working with undocumented WinRT objects
- `millis()` overflow is handled correctly by unsigned subtraction — the 49-day wraparound isn't actually a problem if you don't fight the modular arithmetic
- Fixed-width protocol tags save you from a whole category of parsing bugs
- ESP32 GPIO 6–8 are flash pins — touching them causes boot loops; 34/35/36/39 are input-only with no internal pull resistors
- Arduino `.ino` files *must* match their parent folder name exactly, or the IDE won't touch them

---

*Built by [The-Jedi-501](https://github.com/The-Jedi-501) — feedback and questions welcome.*

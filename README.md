<p align="right"><a href="README.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# NABO Cyber Fortune

Offline entertainment firmware for the FoloToy AI Passport: ESP32-C3, 8 MB Flash, 240×320 portrait screen. NABO welcomes you on boot; no Wi-Fi setup is required.

**[v0.1.3 firmware](https://github.com/Liutupi/nabo-fortune/releases/tag/v0.1.3)** · [Downloads](https://github.com/Liutupi/nabo-fortune/releases/latest) · [Flashing](docs/FLASHING.md) · [Release notes and validation](docs/RELEASE_v0.1.3.md)

<img src="validation/v0.1.3/daily-options.png" width="200" alt="Daily Fortune options"> <img src="validation/v0.1.3/daily-ssq.png" width="200" alt="Daily SSQ sample result">

240×320 previews from the firmware's host renderer, using the public example profile. These are entertainment numbers, not a prediction.

## Modes

| Mode | Content |
|---|---|
| Daily Fortune | A daily zodiac, tarot, lot and hexagram ticket; Up/Down also selects **Daily SSQ** |
| Daily SSQ | One stable daily set of 6 distinct ascending red balls (01–33) and 1 blue ball (01–16), with paged personalization inputs |
| Coin hexagram | Six tosses of three coins, changing-line rules and full readings |
| Fortune sticks | Hold to shake, drop a stick, unfold the scroll, read the poem and plain-language explanation |
| Tarot | 22 original NABO major arcana; shuffle, select, extract and flip |
| Bone weighing | Four birth weights, moving scale weight, plain-language/poem/detail pages |
| Zodiac | Constellation drawing, four star ratings, lucky number and color |
| Ziwei | Twelve palaces, major/minor stars and palace readings |
| Qimen | Chai-bu chart generation, animated placement and question readings |

All results are for entertainment. Daily SSQ combines the birth profile, lunar dates/four pillars, daily ticket, bone weights, Ziwei and Qimen fixed at 12:00 East-8. The same date/profile stays stable across repeat views and restarts. It cannot predict draws or improve winning odds; “daily” denotes a calendar date, not a lottery issue. See [the v1 contract](docs/DAILY_SSQ.md).

## Controls and stored data

- Up/Down selects; OK confirms; double-Up returns to the main wheel.
- Hold/release OK to shake coins or sticks. Up/Down pages through readings.
- In Daily Fortune, choose the original ticket or Daily SSQ, then OK to print. In the SSQ result, Up/Down cycles through numbers and explanations; OK returns to the two choices.
- Tarot extraction takes 1.2 seconds, pauses 0.6 seconds, then flips for 1.4 seconds. The revealed card stays until OK.
- Bone weighing stops at the balanced scale; OK opens all readings.
- Birth data is stored in NVS. After a cold power-on, calibrate the clock. Five minutes without input enters sleep; the upper key wakes the device.
- Body text is 16 px and hints are at least 14 px. The boot voice says “NABO，在此恭候”.

## Install or upgrade

This independent firmware replaces the installed application. Download the release ZIP and SHA256SUMS.txt, verify the checksum, then extract it. A compatible `upgrade` writes the app and assets and preserves NVS. A merged full image at **0x0** may overwrite NVS and PHY data; use it for an intentional full installation. A backup is optional; no provided mode performs a whole-chip erase. See [flashing instructions](docs/FLASHING.md).

The assets partition starts at **0x200000**. Do not use offsets from the Xiaozhi or vocabulary firmware. Releases contain no user NVS, personal birth data, device backups or raw serial logs.

## Source, resources and build

- `firmware/`: independent ESP-IDF 5.5.3 project.
- `prototype/`: offline browser prototype; open prototype/index.html, or use the root Windows launcher.
- `game-art/fortune/`: NABO identity, costumes, Grok starting images and original v1/v2 videos, props, 22 cards/original art, font bitmaps and text sources.
- `tools/fonts/`: source fonts and OFL licenses; tools build sprites/fonts/assets and release bundles.
- `tests/`, `validation/`: logic comparisons, renderer regressions and sanitized device evidence.

```sh
npm install
python tools/pack_assets_fortune.py
# Activate ESP-IDF v5.5.3; choose HOST_PYTHON if the IDF Python lacks host-render libraries.
./tools/validate.sh
```

Windows can use firmware/tools/build.ps1. Build setup is in firmware/README.md; tests are in tests/README.md and the [Daily SSQ guide](docs/DAILY_SSQ.md). The firmware does not depend on Node; package.json pins development-only chart/table libraries. Native Windows host builds do not use ASan/UBSan.

## Validation and limits

The SSQ increment passed 1,795 JS/C cases, 19 mutation checks, 10,741 renderer frames with zero incremental/full differences, font coverage and offline browser input/date/reload checks. The 0.1.3-dev device increment passed verified writes with byte-identical NVS, serial flows/restart, and user-confirmed screen numbers and three physical keys. Release-image checks are recorded separately in [v0.1.3 release notes](docs/RELEASE_v0.1.3.md); older device acceptance is never reassigned to a new binary.

The v0.1.2 baseline covered eight modes, ten scene cycles, power-off profile retention and functional five-minute sleep/wake. Some stick-shaking frames remain slow; original NABO clips are 12 fps. This increment does not newly establish speaker quality, sleep/wake or power consumption. No sleep-current measurement is claimed.

## License

Own program source is MIT. NABO characters, art, video and audio are excluded from MIT; third-party software/fonts retain their licenses. See [NOTICE](NOTICE.md) and [third-party notices](THIRD_PARTY_NOTICES.md). This project is not affiliated with FoloToy.

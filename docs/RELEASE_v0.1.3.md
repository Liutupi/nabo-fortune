<p align="right"><a href="RELEASE_v0.1.3.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# NABO Cyber Fortune v0.1.3

[Download v0.1.3](https://github.com/Liutupi/nabo-fortune/releases/tag/v0.1.3).

Daily Fortune now offers **Daily SSQ** alongside the original ticket. Up/Down selects, OK starts NABO's calculation and printing, and Up/Down pages through the six red/one blue result and its inputs. The same East-8 date/profile stays fixed across repeat views and restarts. Inputs include birth/lunar/four-pillar data, the daily zodiac/tarot/lot/hexagram ticket, bone weights, Ziwei and Qimen at noon. Entertainment only: no draw prediction or increased winning odds. See [the v1 contract](DAILY_SSQ.md).

The eight-position menu, profile format and 8 MB layout are unchanged. Existing compatible installations can use the ZIP's `upgrade` mode to retain NVS. A merged full image is written at **0x0** and may reset NVS/PHY; an app-only image must never be written there. Backups are optional and the installer does not perform a whole-chip erase. See [flashing](FLASHING.md).

## Validation and exact image identity

Host checks include 1,795 JS/C SSQ cases, 19 mutation checks, 10,741 incrementally/full-rendered frames without differences, Chinese font coverage, offline browser keyboard/date/reload checks, M4 sound/layout rejection checks and six guarded-installer tests. macOS render checks use ASan/UBSan. Native Windows sanitizer execution is not claimed.

Development-image device evidence is recorded in validation/SSQ-v1-device.json: version 0.1.3-dev was flashed and readback-verified; NVS was unchanged byte-for-byte; automatic SSQ/original-ticket/restart routes passed; the user confirmed screen numbers, basis paging and all three physical keys. Sampled free heap was at least 97476 bytes and largest block 73728 bytes. The tested development full image was `03013c4035fcbb3bd6822b0730af4c347563105dc6c14db1d50341dcd19838e4`.

Formal v0.1.3 image identity and its separate device checks are bound by validation/v0.1.3-build.json, validation/v0.1.3-device.json and validation/RELEASE-v0.1.3.json. These records distinguish release-image checks from the development-image visual acceptance. We do not reassign v0.1.2 or development-binary hardware results to another binary.

This increment does not newly verify speaker quality, five-minute sleep/wake or sleep current. The v0.1.2 historical baseline and its known slow stick-animation frames remain documented separately. Opening a USB monitor can cause an extra USB reset; software restart with the connection held open was verified to retain calibrated time.

## Build, assets and license

Activate ESP-IDF 5.5.3, install package.json development dependencies and the host Python rendering libraries, then run `./tools/validate.sh`. Use HOST_PYTHON for a host interpreter separate from IDF; macOS Framework Python needs its real Python.app executable for ASan preloading. The firmware is ESP32-C3 / 8 MB / no PSRAM and remains offline.

The release includes the merged full binary, four component binaries, a ZIP, manifest.json and SHA256SUMS.txt. The manifest binds the tagged source commit and image hashes. Verify SHA256SUMS.txt and the ZIP's per-member bundle-SHA256SUMS.txt. All firmware is built from source; no device backup or personal NVS is used as a release artifact. Matching ELF/MAP remain local for debugging.

Own program source is MIT; NABO art/video/audio are not MIT-licensed. Third-party software/fonts keep their original notices. This project is not affiliated with FoloToy.

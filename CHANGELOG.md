<p align="right"><a href="CHANGELOG.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Changelog

## v0.1.3 · 2026-10-24

- Add Daily SSQ under Daily Fortune, in both the browser prototype and firmware.
- Combine birth/lunar/four-pillar data, daily zodiac/tarot/lot/hexagram, bone weights, Ziwei and fixed-noon Qimen into one stable daily set of six red balls and one blue ball, with paged explanations. Entertainment only; this does not predict draws or improve winning odds.
- Preserve the eight-position menu, original daily ticket, NVS profile format and partition layout.
- Add JS/C comparison, date/restart/paging regression, offline browser checks and macOS sanitizer support.
- Stop requiring an original-firmware backup or a whole-chip erase. Compatible upgrades retain NVS; backups are an explicit optional choice.
- Keep development-image acceptance separate from release-image verification; see [v0.1.3 release notes](docs/RELEASE_v0.1.3.md).

## v0.1.2 · 2026-10-22 · First public firmware

- Eight offline modes, 22 original NABO tarot cards, 29 props, Grok animations, full source and original resources.
- 16 px body text and hints of at least 14 px, paged results, full bone explanations, 52 poems and four weight components.
- “NABO, waiting here” boot voice, 11 redesigned sound effects with softened attack.
- Fix truncated long card names exhausting image handles and hiding cards/scale parts.
- Enlarge tarot art and extend extraction/flip timing. Retain the balanced scale around nine seconds; OK opens its explanation.
- Optimize rotation and yield on slow frames. Eight modes and ten scene cycles showed no unexpected reboot.
- User and serial checks confirmed birth-profile retention across power-off, five-minute sleep and upper-key wake.

ESP-IDF 5.5.3; ESP32-C3 / 8 MB. Application: 505648 / 2031616 bytes; assets: 3677860 / 6291456 bytes. Sampled minimum free heap 88144 bytes, largest block 24576 bytes. Statistics and limitations: validation/M4-device.md.

App SHA256: `d46453682debe1f4199b66bf2d5e84eda9f256964210cb883c41c9e0aaa72834`.

Assets SHA256: `fe7922bfa7f986c99b8100c6fec727310667150a3842e11789234ec1296c0664`.

Merged SHA256: `60a4c53d426b43c2e163936479ea7f6885baafb26486bb9be50948590d00c1cd`.

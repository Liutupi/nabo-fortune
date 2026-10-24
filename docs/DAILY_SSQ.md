<p align="right"><a href="DAILY_SSQ.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Daily SSQ

In **Daily Fortune**, use Up/Down to select **今日双色球** (Daily SSQ), then OK. NABO calculates for 1.2 seconds and prints a ticket. The result shows six ascending, distinct red numbers (01–33) and one blue number (01–16). Up/Down cycles through the numbers and the explanation pages; OK returns to the two choices. Double-Up keeps its existing return-to-menu behavior. The original fortune ticket remains the default choice.

This is offline, personalized entertainment. It does not predict draws or improve the probability of winning. No draw history, network connection, betting or purchasing service is involved. The date is the **East-8 calendar date**, not a lottery issue or an assertion that a draw takes place today. Calibrate the device clock and save your birth profile; the explanation explicitly labels the existing example profile when it is in use. An open result retains its printed date; print again to use a new date or profile.

## Stable v1 generation

The input is the current year/month/day and the existing birth year/month/day, hour branch and gender. Supported calendar dates are 1901-01-01 through 2100-12-31; the existing profile editor retains its narrower range. Invalid dates, hour branches and genders are rejected.

`FortuneCore.dailySsq` and `fc_daily_ssq` recompute these inputs without changing the shared interactive RNG:

- Birth and current lunar dates, leap-month flags and four pillars.
- The existing daily ticket: zodiac sign/stars, tarot and orientation, lot number/level, hexagram, and suitable/unsuitable activities.
- A daily lucky number (1–9), using a separate reproducible stream.
- Four bone weights and their sum.
- Ziwei soul/body palaces, five-element class, soul/body stars, palace stems/names, major/minor star positions, brightness and transformations.
- Qimen at **12:00 East-8 on that date**: chart class, period, bureau, stems, star/door/deity positions and leading palaces.

The ordered numeric values are mixed as unsigned 32-bit words, starting with version tag `0x53535101` and FNV-style multiply/XOR operations. Signed sentinel values are normalized to uint32; neither C padding nor translated text is hashed. The resulting seed drives a private mulberry32 stream. Rejection sampling chooses six red balls without replacement, sorts them, then chooses a blue ball. The existing daily ticket algorithm and its golden answers remain unchanged.

Re-entering, reprinting, restarting, changing the current hour or using other fortune modes does not change the same date/profile result. Different dates/profiles can coincidentally produce the same combination. A future algorithm change must use a new version tag and update the fixed regression vector.

The explanation shows the actual birth profile, lunar date/day pillar, daily sign/stars/lucky number, tarot, lot, hexagram, bone total, Ziwei class/palaces, and fixed-noon Qimen result. These are inputs to the personalization process, not evidence of predictive power. The screen says “娱乐选号，不预测开奖” and the explanation says it does not improve winning probability.

## Reproduce validation

Install the root package.json development dependencies (`npm install`). Python needs Pillow, numpy and fontTools for assets/rendering; font regeneration additionally needs the source fonts under tools/fonts. Generated firmware fonts remain tracked; assets are packaged locally.

```sh
python tools/gen_fonts_fortune.py
node tools/gen_fortune_text.js prototype/index.html firmware/main/fortune/fortune_text.c
./tools/validate.sh --static
# Activate ESP-IDF v5.5.3 before either firmware command:
./tools/validate.sh --firmware
./tools/validate.sh
```

`HOST_PYTHON` selects the host Python independently from IDF's Python. On macOS Framework Python, use the real `Python.app/Contents/MacOS/Python` executable so ASan is preloaded before Python's launcher runs. The render runner uses DYLD_INSERT_LIBRARIES on macOS and LD_PRELOAD on Linux. Windows retains the existing PowerShell host build without sanitizers.

The gate runs existing logic/golden and 19 mutation checks, the new JS/C SSQ comparison, M4 sound/bundle checks, font coverage, ASan/UBSan UI rendering, daily SSQ controls/stability, and image lifetime regression. Firmware validation builds, merges and verifies the complete 8 MB layout including the updated assets. The program identifies this increment as **0.1.3**; v0.1.2 hardware acceptance does not apply to this build.

Browser regression is separate and requires a local HTTP server and a Playwright installation:

```sh
python -m http.server 8765 --bind 127.0.0.1
# In another terminal; optionally set PLAYWRIGHT_MODULE and CHROMIUM_EXECUTABLE:
node tests/test_ssq_browser.cjs
```

It blocks remote requests and tests real keys, calculation/printing, paging, repeat/reload stability, East-8 midnight while the browser uses Los Angeles time, and the original ticket. Screenshots and results go to ignored `private/ssq-validation/`.

Build and host tests do not establish physical screen, button, sound, runtime memory, sleep or power behavior. Flashing requires separate approval for this exact build and target device. The existing partition layout is unchanged; a merged 0x0 installation may replace NVS, while a compatible segmented update can preserve the birth profile. Do not infer permission to erase Flash or reuse old device results.

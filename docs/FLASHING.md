<p align="right"><a href="FLASHING.zh_CN.md">简体中文</a> · <strong>English</strong></p>

# Flashing and upgrades

For FoloToy AI Passport, ESP32-C3 and 8 MB Flash only. Confirm the intended device and its MAC before writing. Run these commands in the extracted release ZIP; from source, also pass `--release-dir <extracted-folder>`.

## Verify the download

Download the v0.1.3 ZIP and SHA256SUMS.txt from the release. Verify the ZIP's SHA256, extract it, and check bundle-SHA256SUMS.txt. manifest.json binds the source commit, four component images, merged image and matching application ELF identity. No user NVS, birth profile or device backup is included.

Install esptool 4.x (tested with 4.12.0), then inspect the confirmed target:

```sh
python -m pip install 'esptool>=4.12,<5'
python flash_release.py --mode inspect --port <PORT> --expected-mac <MAC>
```

## Preserve an existing compatible installation

```sh
python flash_release.py --mode upgrade --port <PORT> --expected-mac <MAC>
```

The tool verifies artifact hashes, the live MAC, ESP32-C3/8 MB, disabled secure boot/encryption and the exact partition table. It writes only the app at 0x10000 and assets at 0x200000, verifies them and reboots. It does not write NVS or PHY. Layout mismatch stops the upgrade.

## Install from another layout

This replaces the application and partition layout. Existing settings/data may be incompatible or overwritten; save any records you need beforehand.

```sh
python flash_release.py --mode install --port <PORT> --expected-mac <MAC> --confirm-full-install
```

This writes the verified bootloader, table, app and assets as segments. Neither mode runs a whole-chip erase. An original-firmware readback is not required. If you explicitly want a private full-Flash backup, add `--backup-dir <private-local-folder>`; never upload that backup.

## Merged image / browser installer

`nabo-fortune-v0.1.3-full.bin` is the complete merged image, written from **0x0**. A merged installation pads the gaps between components and may reset NVS/PHY data. Use it only for an intentional full installation; use the compatible segmented upgrade above to keep existing profiles. Never flash an application-only binary at 0x0.

| Region | Offset | Capacity |
|---|---|---|
| Bootloader | 0x0 | Until 0x8000 |
| Partition table | 0x8000 | 0x1000 |
| NVS | 0x9000 | 0x6000 |
| PHY | 0xf000 | 0x1000 |
| App | 0x10000 | 0x1f0000 |
| Assets | 0x200000 | 0x600000 |

Do not use Xiaozhi/vocabulary asset offsets. After flashing, check startup, calibrate the East-8 clock after cold power-on, and confirm your birth profile. Daily SSQ is under Daily Fortune. A successful write does not prove visible rendering, physical buttons, sound or sleep current. Opening/reopening some USB serial monitors can cause an additional USB reset and the expected clock-calibration prompt.

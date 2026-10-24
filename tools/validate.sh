#!/bin/sh
# Shared host/firmware gate. Never opens a device or writes Flash.
set -eu
cd "$(dirname "$0")/.."
mode=${1:-all}
case "$mode" in all|--static|--firmware) ;; *) echo 'Usage: tools/validate.sh [--static|--firmware]' >&2; exit 2;; esac
HOST_PYTHON=${HOST_PYTHON:-python3}
export HOST_PYTHON
"$HOST_PYTHON" tools/pack_assets_fortune.py
if [ "$mode" != --firmware ]; then
  "$HOST_PYTHON" tests/test_fortune.py
  "$HOST_PYTHON" tests/test_fortune.py --mutate
  "$HOST_PYTHON" tests/test_ssq.py
  "$HOST_PYTHON" tests/test_release_tools.py
  "$HOST_PYTHON" tests/test_m4_host.py --sound-only
  "$HOST_PYTHON" tests/fortune_host/check_glyphs.py firmware/assets/fortune_assets.bin firmware/assets/assets_manifest.json firmware/main/fortune
  sh tests/fortune_host/build.sh
  out="$PWD/private/ssq-validation"
  mkdir -p "$out"
  sh tests/fortune_host/run.sh "$PWD/firmware/assets/fortune_assets.bin" "$out/all" run_all.py
  sh tests/fortune_host/run.sh "$PWD/firmware/assets/fortune_assets.bin" "$out/ui" test_ssq_ui.py
  sh tests/fortune_host/run.sh "$PWD/firmware/assets/fortune_assets.bin" "$out/asset-lifetime.json" test_asset_lifetime.py
fi
if [ "$mode" != --static ]; then
  [ "$(idf.py --version)" = 'ESP-IDF v5.5.3' ] || { echo 'Activate ESP-IDF v5.5.3 first' >&2; exit 1; }
  (cd firmware
   idf.py -DIDF_TARGET=esp32c3 build
   idf.py merge-bin -o nabo-fortune-merged.bin
   python tools/verify_firmware.py build)
  "$HOST_PYTHON" tests/test_m4_host.py
fi
echo "Validation PASS ($mode)"

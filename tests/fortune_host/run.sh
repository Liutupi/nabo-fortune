#!/bin/sh
# 用法：sh run.sh <assets.bin> <out_dir> [脚本 run_ui.py]
set -eu
cd "$(dirname "$0")"
export ASAN_OPTIONS=detect_leaks=0
if [ "$(uname -s)" = Darwin ]; then
  DYLD_INSERT_LIBRARIES=$(${CC:-gcc} -print-file-name=libclang_rt.asan_osx_dynamic.dylib)
  export DYLD_INSERT_LIBRARIES
else
  LD_PRELOAD=$(${CC:-gcc} -print-file-name=libasan.so):$(${CC:-gcc} -print-file-name=libubsan.so)
  export LD_PRELOAD
fi
exec "${HOST_PYTHON:-python3}" "${3:-run_ui.py}" "$1" "$2"

#!/bin/sh
# 用法：sh run.sh <assets.bin> <out_dir> [脚本 run_ui.py]
cd "$(dirname "$0")"
ASAN_OPTIONS=detect_leaks=0 LD_PRELOAD=$(gcc -print-file-name=libasan.so):$(gcc -print-file-name=libubsan.so) python3 ${3:-run_ui.py} "$1" "$2"

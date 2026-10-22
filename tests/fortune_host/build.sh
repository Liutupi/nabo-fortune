#!/bin/sh
# 宿主机编译 UI 共享库（ASan/UBSan）。用法：sh build.sh [额外 CFLAGS]
cd "$(dirname "$0")"; F=../../firmware/main/fortune
gcc -std=c11 -O1 -g -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=undefined -fPIC -shared $@ -o libfchost.so fc_host.c \
  $F/ui/*.c $F/gfx/fc_gfx.c $F/gfx/fc_res.c $F/fortune_misc.c $F/fortune_tables.c $F/fortune_calendar.c $F/fortune_qimen.c $F/fortune_ziwei.c $F/fortune_text.c $F/fortune_names.c -I$F/gfx

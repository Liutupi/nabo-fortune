param([string]$Compiler='gcc')
$ErrorActionPreference='Stop'
$sourceRoot=Join-Path $PSScriptRoot '..\..\firmware\main\fortune'
$files=@(Join-Path $PSScriptRoot 'fc_host.c')
$files+=@(Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'ui') -Filter '*.c' | ForEach-Object FullName)
$files+=@('gfx\fc_gfx.c','gfx\fc_res.c','fortune_misc.c','fortune_ssq.c','fortune_tables.c','fortune_calendar.c','fortune_qimen.c','fortune_ziwei.c','fortune_text.c','fortune_names.c' | ForEach-Object {Join-Path $sourceRoot $_})
& $Compiler -std=c11 -O2 -g -Wall -Wextra -shared -o (Join-Path $PSScriptRoot 'libfchost.so') @files "-I$sourceRoot\gfx"
exit $LASTEXITCODE

$ErrorActionPreference='Stop'
$artRoot=Split-Path -Parent $PSScriptRoot
. (Join-Path $artRoot 'costume\export-art.ps1')
$template=Join-Path $artRoot 'costume\final-transparent.png'
$palette=[string[]]([FortuneArtExport]::Palette+@('#DCA64E','#A66D36','#4A2A24','#241B24'))
$ids=@('N-IDLE','N-WELCOME','N-SHELL','N-SHELL-TOSS','N-TUBE','N-WOW','N-NOD','N-SIGH','N-CALC','N-PRINT')
$alignment=@()
$checks=@()
foreach($id in $ids) {
 $original=Join-Path $PSScriptRoot ('originals\'+$id+'.png')
 if($id -in @('N-IDLE','N-WELCOME','N-WOW','N-NOD','N-SIGH','N-PRINT')) {$original=$template}
 $activePalette=if($id -like 'N-SHELL*'){$palette}else{[FortuneArtExport]::Palette}
 $rgba=Join-Path $PSScriptRoot ('transparent\'+$id+'.png')
 $transform=[FortuneArtExport]::AlignExport($original,$rgba,$template,$null,[string[]]$activePalette)
 $b=[System.Drawing.Bitmap]::new($rgba)
 $out=[System.Drawing.Bitmap]::new(1024,1024)
 $g=[System.Drawing.Graphics]::FromImage($out)
 $g.Clear([System.Drawing.ColorTranslator]::FromHtml('#00B140'));$g.DrawImageUnscaled($b,0,0);$g.Dispose()
 $outFile=Join-Path $PSScriptRoot ($id+'.png')
 $out.Save($outFile,[System.Drawing.Imaging.ImageFormat]::Png)
 $hat=[FortuneArtExport]::HatBounds($b)
 $alignment += [pscustomobject]@{id=$id;scale=$transform[0];offset_x=$transform[1];offset_y=$transform[2];hat_x=$hat.X;hat_y=$hat.Y;hat_width=$hat.Width;hat_height=$hat.Height}
 $checks += [pscustomobject]@{id=$id;width=$out.Width;height=$out.Height;opaque_interior_colors=[FortuneArtExport]::OpaqueColors($b);background='#00B140';keying_border_mismatches=[FortuneArtExport]::BorderMismatch($out,'#00B140');sha256=(Get-FileHash -LiteralPath $outFile -Algorithm SHA256).Hash.ToLower()}
 $b.Dispose();$out.Dispose()
 foreach($size in @(120,100,76,56)) {
  # Source interiors already have the shared 23-colour palette. Resampling only
  # creates boundary antialiasing; do not remap golden shell colours to skin.
  [FortuneArtExport]::Export($rgba,(Join-Path $PSScriptRoot ('previews\'+$id+'-'+$size+'.png')),$size,$size,'#0D0820',$false,$false)
 }
}
$alignment | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'alignment.json') -Encoding utf8
$checks | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'export-checks.json') -Encoding utf8
$checks | Format-Table id,width,height,opaque_interior_colors,keying_border_mismatches -AutoSize

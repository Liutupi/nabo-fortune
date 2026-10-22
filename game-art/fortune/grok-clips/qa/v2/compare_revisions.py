"""Inspection-only revision comparison. Source videos remain unchanged."""
import json
import os
import shutil
import subprocess
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parent
ROOT = OUT.parent.parent
FFMPEG = os.environ.get('FFMPEG') or shutil.which('ffmpeg')
if not FFMPEG:
    from imageio_ffmpeg import get_ffmpeg_exe
    FFMPEG = get_ffmpeg_exe()
BG = (13, 8, 32)
times = [0, .5, 1, 1.5, 2, 2.5, 3, 4]
labels = ['N-TUBE', 'N-TUBE_v2', 'N-CALC', 'N-CALC_v2']
font = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 17)
small = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 13)
sheet = Image.new('RGB', (1700, 910), BG)
draw = ImageDraw.Draw(sheet)
draw.text((12,12), 'GROK v1 / v2   -   tube motion and calculation gaze', font=font, fill='white')
for row, name in enumerate(labels):
    proc = subprocess.run([FFMPEG, '-v', 'error', '-i', str(ROOT/(name+'.mp4')),
                           '-map', '0:v:0', '-an', '-f', 'rawvideo', '-pix_fmt', 'rgb24', 'pipe:1'],
                          capture_output=True, check=True)
    frames = np.frombuffer(proc.stdout, np.uint8).reshape(-1,480,480,3)
    y = 50 + row*214
    draw.text((8,y+80), name, font=font, fill='#35f0dc')
    for col, seconds in enumerate(times):
        index = min(round(seconds*24),len(frames)-1)
        frame = Image.fromarray(frames[index])
        frame.save(OUT/(name+f'-native-{seconds:.2f}s.png'))
        # Full avatar for tube, original-resolution face detail for eye gaze.
        if 'CALC' in name:
            frame = frame.crop((110,185,385,355)).resize((192,119),Image.Resampling.LANCZOS)
            image_y = y+47
        else:
            frame = frame.resize((192,192),Image.Resampling.LANCZOS)
            image_y = y+20
        x = 160+col*192
        sheet.paste(frame,(x,image_y))
        draw.text((x+3,y+1),f'{seconds:.2f}s',font=small,fill='#b5a9cf')
sheet.save(OUT/'revision-comparison.png')

old = {x['id']:x for x in json.loads((OUT.parent/'metadata-and-indicators.json').read_text(encoding='utf-8'))}
new = {x['id']:x for x in json.loads((OUT/'metadata-and-indicators.json').read_text(encoding='utf-8'))}
comparison=[]
for ident in ('N-TUBE','N-CALC'):
    comparison.append({'id':ident,'v1_sha256':old[ident]['source_sha256'],
                       'v2_sha256':new[ident+'_v2']['source_sha256'],
                       'v1_cap_max_displacement_at_120':old[ident]['cap_center_max_abs_from_first_at_120'],
                       'v2_cap_max_displacement_at_120':new[ident+'_v2']['cap_center_max_abs_from_first_at_120'],
                       'v1_endpoints_mae':old[ident]['first_last_foreground_mean_abs_rgb_error'],
                       'v2_endpoints_mae':new[ident+'_v2']['first_last_foreground_mean_abs_rgb_error']})
(OUT/'revision-comparison.json').write_text(json.dumps(comparison,indent=2),encoding='utf-8')
print(json.dumps(comparison,indent=2))

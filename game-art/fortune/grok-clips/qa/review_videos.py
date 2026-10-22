"""Read-only video review: metadata, analysis thumbnails and contact sheets.

Writes only review artifacts beside the source clips. No production sprites,
retiming, keying, video edits, firmware assets or acceptance thresholds.
"""
import argparse
import hashlib
import json
import os
import shutil
import math
import re
import subprocess
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
QA_ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--ids', nargs='+')
parser.add_argument('--out-subdir', default='.')
args = parser.parse_args()
OUT = (QA_ROOT / args.out_subdir).resolve()
if not OUT.is_relative_to(QA_ROOT):
    raise ValueError('Review output must remain inside the existing QA directory')
OUT.mkdir(parents=True, exist_ok=True)
FFMPEG = os.environ.get('FFMPEG') or shutil.which('ffmpeg')
if not FFMPEG:
    from imageio_ffmpeg import get_ffmpeg_exe
    FFMPEG = get_ffmpeg_exe()
IDS = ['N-IDLE', 'N-WELCOME', 'N-SHELL', 'N-SHELL-TOSS', 'N-TUBE', 'N-WOW', 'N-NOD', 'N-SIGH', 'N-CALC', 'N-PRINT']
if args.ids:
    IDS = args.ids
LOOPS = {'N-IDLE', 'N-SHELL', 'N-TUBE', 'N-CALC'}
BG = (13, 8, 32)
SIZE = 256
font = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 17)
small = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 12)
all_info = []
overview_rows = []

def dark_preview(arr):
    a = arr.astype(np.int16)
    border = np.concatenate([a[:6].reshape(-1, 3), a[:, :6].reshape(-1, 3), a[:, -6:].reshape(-1, 3)])
    bg = np.median(border, axis=0)
    mask = np.abs(a - bg).sum(axis=2) > 75
    shown = arr.copy()
    shown[~mask] = BG
    return shown, mask, bg

def hat_box(arr):
    a = arr.astype(np.int16)
    mask = ((a[..., 0] > 165) & (a[..., 1] > 155) & (a[..., 2] > 180)
            & (np.abs(a[..., 0] - a[..., 1]) < 35)
            & (a[..., 2] >= a[..., 0] - 10) & (a[..., 2] >= a[..., 1]))
    mask[int(SIZE * .45):] = False
    ys, xs = np.where(mask)
    return [int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())] if len(xs) else None

for ident in IDS:
    source = ROOT / (ident + '.mp4')
    proc = subprocess.run([str(FFMPEG), '-hide_banner', '-i', str(source), '-map', '0:v:0', '-an', '-f', 'null', '-'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    log = proc.stderr.decode('utf-8', errors='replace')
    (OUT / (ident + '-decode.txt')).write_text(log, encoding='utf-8')
    duration_match = re.search(r'Duration: (\d+):(\d+):(\d+\.\d+)', log)
    duration = sum(float(v)*factor for v, factor in zip(duration_match.groups(), [3600,60,1]))
    stream = next(line.strip() for line in log.splitlines() if 'Video: h264' in line)
    dims = re.search(r'(\d{2,5})x(\d{2,5})', stream)
    fps = float(re.search(r'([\d.]+) fps', stream).group(1))
    decoded = subprocess.run([str(FFMPEG), '-v', 'error', '-i', str(source), '-map', '0:v:0', '-an', '-vf', f'scale={SIZE}:{SIZE}:flags=area', '-f', 'rawvideo', '-pix_fmt', 'rgb24', 'pipe:1'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    frames = np.frombuffer(decoded.stdout, dtype=np.uint8).reshape(-1, SIZE, SIZE, 3)
    n = len(frames)
    hats = np.array([hat_box(a) for a in frames], dtype=float)
    centers = np.column_stack(((hats[:, 0]+hats[:, 2])/2, (hats[:, 1]+hats[:, 3])/2))
    widths = hats[:, 2]-hats[:, 0]+1
    heights = hats[:, 3]-hats[:, 1]+1
    first, first_mask, first_bg = dark_preview(frames[0])
    last, last_mask, last_bg = dark_preview(frames[-1])
    union = first_mask | last_mask
    delta = np.abs(first.astype(float)-last.astype(float)).mean(axis=2)
    entry = {
        'id': ident, 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'bytes': source.stat().st_size, 'duration_seconds': duration,
        'width': int(dims.group(1)), 'height': int(dims.group(2)), 'fps': fps,
        'decoded_frames': n, 'decode_ok': proc.returncode == 0,
        'contains_audio': 'Audio: aac' in log, 'contains_attached_cover': 'attached pic' in log,
        'analysis_size': SIZE, 'loop_required': ident.split('_v')[0] in LOOPS,
        'cap_center_span_at_120': ((centers.max(axis=0)-centers.min(axis=0))*120/SIZE).round(3).tolist(),
        'cap_center_max_abs_from_first_at_120': (np.abs(centers-centers[0]).max(axis=0)*120/SIZE).round(3).tolist(),
        'cap_width_span_at_120': round(float(np.ptp(widths)*120/SIZE), 3),
        'cap_height_span_at_120': round(float(np.ptp(heights)*120/SIZE), 3),
        'first_last_foreground_mean_abs_rgb_error': round(float(delta[union].mean()), 3),
        'first_last_silhouette_iou': round(float((first_mask & last_mask).sum()/union.sum()), 5),
        'border_median_first': first_bg.tolist(), 'border_median_last': last_bg.tolist(),
        'metrics_are_review_indicators_not_automatic_acceptance': True,
    }
    all_info.append(entry)
    # Every other original frame: full ordered 12 fps review at 24 fps source.
    indices = list(range(0, n, max(1, round(fps/12))))
    if indices[-1] != n-1: indices.append(n-1)
    cols, cellw, cellh = 8, 128, 148
    rows = math.ceil(len(indices)/cols)
    sheet = Image.new('RGB', (cols*cellw, rows*cellh+46), BG)
    draw = ImageDraw.Draw(sheet)
    draw.text((8, 7), f'{ident} / ordered frames / analysis only, source unchanged', font=font, fill='white')
    for k, idx in enumerate(indices):
        preview, _, _ = dark_preview(frames[idx])
        image = Image.fromarray(preview).resize((120,120), Image.Resampling.LANCZOS)
        x, y = (k%cols)*cellw, 46+(k//cols)*cellh
        sheet.paste(image,(x+4,y))
        draw.text((x+5,y+122),f'{idx/fps:.2f}s  #{idx}',font=small,fill='#b5a9cf')
    sheet.save(OUT/(ident+'-ordered.png'))
    # Six evenly spaced samples plus cap-logo crops.
    picks = np.rint(np.linspace(0,n-1,6)).astype(int)
    row = Image.new('RGB',(6*184+160,222),BG)
    d = ImageDraw.Draw(row); d.text((6,90),ident,font=font,fill='#35f0dc')
    logo = Image.new('RGB',(6*192,154),BG)
    ld = ImageDraw.Draw(logo)
    for k, idx in enumerate(picks):
        preview,_,_ = dark_preview(frames[idx])
        row.paste(Image.fromarray(preview).resize((184,184),Image.Resampling.LANCZOS),(160+k*184,22))
        d.text((160+k*184+5,3),f'{idx/fps:.2f}s',font=small,fill='#b5a9cf')
        crop = Image.fromarray(frames[idx]).crop((80,5,181,84)).resize((192,150),Image.Resampling.NEAREST)
        logo.paste(crop,(192*k,4))
        ld.text((192*k+3,2),f'{idx/fps:.2f}s',font=small,fill='#ff3da6')
    logo.save(OUT/(ident+'-cap.png'))
    overview_rows.append(row)
    print(json.dumps(entry,ensure_ascii=False),flush=True)

parts = math.ceil(len(overview_rows)/5)
for part in range(parts):
    sheet = Image.new('RGB',(1264,5*222+50),BG)
    d = ImageDraw.Draw(sheet);d.text((12,12),f'NABO GROK VIDEO REVIEW {part+1}/{parts} / uniformly sampled frames',font=font,fill='white')
    for k,row in enumerate(overview_rows[part*5:(part+1)*5]):sheet.paste(row,(0,50+k*222))
    sheet.save(OUT/f'overview-{part+1}.png')
(OUT/'metadata-and-indicators.json').write_text(json.dumps(all_info,ensure_ascii=False,indent=2),encoding='utf-8')

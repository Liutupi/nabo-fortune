"""Opt-in serial M4 exercise. Does not flash; clears its synthetic birth profile at the end."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import time

import serial
from serial.tools import list_ports

p=argparse.ArgumentParser()
p.add_argument('--port',required=True)
p.add_argument('--expected-mac',required=True)
p.add_argument('--out',required=True)
p.add_argument('--keep-profile',action='store_true',help='Skip profile/time writes, calibration edits and restarts; preserve current personal data')
args=p.parse_args()
project=Path(__file__).resolve().parents[1]
image=(project/'firmware/build/nabo-fortune-passport.bin').read_bytes()
version=image[48:80].split(b'\0')[0].decode('ascii')
app_sha256=hashlib.sha256(image).hexdigest()
ports={x.device:x for x in list_ports.comports()}
port=ports[args.port]
assert port.vid == 0x303A and (port.serial_number or '').lower() == args.expected_mac.lower()
out=Path(args.out); out.mkdir(parents=True,exist_ok=True)
log=(out/'serial.log').open('w',encoding='utf-8')
records=[]
s=None
last_uptime=0
exercise_started=False
reply_retries=0


def connect():
    global s
    if s is not None:
        s.close()
    s=serial.Serial()
    s.port=args.port; s.baudrate=115200; s.timeout=.1; s.dtr=False; s.rts=False
    s.open()


def read_line():
    raw=s.readline()
    line=raw.decode('utf-8',errors='replace').strip()
    if line:
        log.write(line+'\n'); log.flush()
        assert not any(x in line for x in ('Guru Meditation','assert failed','CORRUPT HEAP','Stack canary','task_wdt:')),line
        if exercise_started:
            assert 'fortune: button key=' not in line, 'Physical input during serial exercise: '+line
    return line


def wait(seconds):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        read_line()


def cmd(text,expect=None,allow_restart=False,_retry=False):
    global last_uptime,reply_retries
    s.write((text+'\n').encode())
    end=time.monotonic()+10
    while time.monotonic()<end:
        line=read_line()
        if not line.startswith('NF '): continue
        if expect and expect not in line:
            raise AssertionError((text,expect,line))
        m=re.search(r'uptime_ms=(\d+)',line)
        if m:
            now=int(m[1])
            if not allow_restart: assert now >= last_uptime,(text,last_uptime,now)
            last_uptime=now
        records.append({'command':text,'reply':line})
        return line
    if not _retry and (text in ('status','perf') or text.startswith('go ')):
        reply_retries+=1
        print('Retry missing serial acknowledgement:',text,flush=True)
        return cmd(text,expect,allow_restart,True)
    raise TimeoutError(text)


def tap(key,hold=.08):
    cmd(f'key {key} 1')
    wait(hold)
    cmd(f'key {key} 0')


def go(scene):
    cmd('go '+scene,'scene='+scene)
    wait(.18)


def soft_restart():
    global last_uptime
    cmd('restart','restarting')
    s.close(); time.sleep(2)
    connect(); last_uptime=0
    return cmd('status',allow_restart=True)


connect()
# USB can retain replies and button logs from before this process. Drain to a
# quiet boundary before the first request; subsequent physical events still fail.
s.reset_input_buffer()
quiet_until=time.monotonic()+.5
while time.monotonic()<quiet_until:
    if read_line(): quiet_until=time.monotonic()+.5
initial=cmd('status')
assert 'version='+version+' ' in initial, 'Connected firmware version differs from local test image'
exercise_started=True
now=datetime.datetime.now()
cmd('time 2026-02-30 25:80','error=invalid_time')
cmd('pf 2000 2 30 4 0','error=invalid_profile')
if not args.keep_profile:
    assert '(示例)' in initial,'Refusing to overwrite an existing personal profile'
    cmd('time '+now.strftime('%Y-%m-%d %H:%M'),'time=')
    cmd('pf 2000 2 29 5 1','pf=2000-2-29 h=5 g=1')
    go('clock'); tap('u'); tap('d')
    for _ in range(5): tap('o')
    cmd('status','scene=menu')
    go('birth'); tap('u'); tap('d')
    for _ in range(5): tap('o')
    cmd('status','scene=menu')
    persisted=soft_restart()
    assert 'pf=2000-2-28 h=5 g=1' in persisted,persisted  # 2000→1999→2000 clamps the leap day to 28.
    assert '(未校准)' not in persisted,persisted
cmd('perf')

# Full interaction routes; durations allow animations to reach their result cards.
go('daily'); tap('o'); wait(7); tap('d'); wait(.35); tap('u'); cmd('perf')
go('shake')
for _ in range(6):
    tap('o',.6); wait(2.6)
wait(2); tap('o'); wait(1.6); tap('d'); wait(.35); tap('u'); cmd('perf')
go('lot'); tap('o',1.9); wait(8); tap('d'); wait(.35); tap('u'); cmd('perf')
go('tarot'); wait(4); tap('d'); tap('o'); wait(4.5); cmd('perf')
for scene,delay in (('bone',9.6),('star',4),('ziwei',3.8),('qimen',5)):
    go(scene); tap('o'); wait(delay)
    if scene in ('bone','ziwei','qimen'): tap('o'); wait(.9)
    if scene in ('bone','ziwei','qimen'): tap('d'); wait(.35); tap('u')
    cmd('perf')
for cue in range(11):
    cmd('sfx '+str(cue),'sfx='+str(cue)); wait(.65)

# Ten rounds across all twelve scenes. Uptime must remain monotonic throughout.
scenes=('boot','menu','birth','daily','shake','lot','tarot','bone','star','ziwei','qimen','clock')
for cycle in range(10):
    for scene in scenes:
        go(scene)
    cmd('status')
    print('stress round',cycle+1,flush=True)
go('tarot'); tap('u'); wait(.06); tap('u'); cmd('status','scene=menu')
if not args.keep_profile:
    cmd('pf clear','profile_clear=ESP_OK')
    clean=soft_restart()
    assert '(示例)' in clean,clean
go('menu')
final=cmd('status','scene=menu')
cmd('perf')
heaps=[int(m.group(1)) for r in records if (m:=re.search(r'heap=(\d+)',r['reply']))]
largest=[int(m.group(1)) for r in records if (m:=re.search(r'largest=(\d+)',r['reply']))]
result={
    'version':version,'app_sha256':app_sha256,
    'serial_reply_retries':reply_retries,
    'date':now.isoformat(timespec='seconds'),'port':args.port,'mac':args.expected_mac,
    'initial_status':initial,'final_status':final,
    'profile_preserved_mode':args.keep_profile,
    'software_restart_profile_and_clock_retained':None if args.keep_profile else True,
    'leap_day_clamped_on_nonleap_year_change':None if args.keep_profile else True,
    'synthetic_profile_cleared_to_demo':None if args.keep_profile else True,
    'invalid_date_commands_rejected':True,'all_eight_play_routes_exercised':True,
    'scenes_in_stress':list(scenes),'stress_rounds':10,'unexpected_restarts':0,
    'double_up_returned_to_menu':True,'sfx_triggers':11,
    'image_misses_zero':all('image_misses=0' in r['reply'] for r in records if r['command']=='perf'),
    'min_heap':min(heaps),'min_largest':min(largest),
    'records':records,
    'not_verified':['physical buttons','LCD visual appearance','speaker sound quality','power-off persistence','deep sleep and physical wake'],
}
assert result['image_misses_zero'],'An image was missing during the device exercise'
(out/'result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='records'},ensure_ascii=False,indent=2))
s.close(); log.close()

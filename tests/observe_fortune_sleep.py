"""Read-only observation of the real five-minute idle deadline and manual wake."""
import argparse,json,re,time
from pathlib import Path
import serial
from serial.tools import list_ports

p=argparse.ArgumentParser()
p.add_argument('--port',required=True);p.add_argument('--expected-mac',required=True);p.add_argument('--out',required=True)
args=p.parse_args();out=Path(args.out);out.mkdir(parents=True,exist_ok=True)
assert any(x.device==args.port and (x.serial_number or '').lower()==args.expected_mac.lower() for x in list_ports.comports())

def open_port():
    q=serial.Serial();q.port=args.port;q.baudrate=115200;q.timeout=.1;q.dtr=False;q.rts=False;q.open();return q

s=open_port();start=time.monotonic();last_input=start
result={'sleep_request_seen':False,'usb_disconnected_after_sleep':False,'deep_sleep_wake_seen':False,'physical_input_events':0}
f=(out/'sleep.log').open('w',encoding='utf-8')
s.write(b'status\n')
initial_uptime=None;last_device_input=None;sleep_at=None;wake_deadline=None
print('Observation started; no further commands until manual wake.',flush=True)
while time.monotonic()-start < 690:
    if s is None:
        try:s=open_port()
        except serial.SerialException:time.sleep(.5);continue
    try:line=s.readline().decode('utf-8',errors='replace').strip()
    except serial.SerialException:
        s.close();s=None
        if sleep_at is not None:result['usb_disconnected_after_sleep']=True
        time.sleep(.5);continue
    if not line:
        if sleep_at is not None and time.monotonic()-sleep_at>240:break
        if sleep_at is None and time.monotonic()-last_input>390:break
        continue
    f.write(line+'\n');f.flush()
    assert not any(x in line for x in ('task_wdt:','Guru Meditation','assert failed')),line
    if line.startswith('NF ') and initial_uptime is None:
        m=re.search(r'uptime_ms=(\d+)',line)
        if m:initial_uptime=last_device_input=int(m[1])
        result['initial_status']=line
    if 'fortune: button key=' in line and sleep_at is None:
        m=re.search(r'\((\d+)\)',line)
        if m:last_device_input=int(m[1])
        last_input=time.monotonic();result['physical_input_events']+=1
    if 'fortune: idle, entering deep sleep' in line:
        m=re.search(r'\((\d+)\)',line)
        if m and last_device_input is not None:
            result['idle_before_sleep_ms']=int(m[1])-last_device_input
            assert 299000 <= result['idle_before_sleep_ms'] <= 302000,result
        result['sleep_request_seen']=True;sleep_at=time.monotonic()
        print('SLEEP_REQUEST_OBSERVED',json.dumps(result,ensure_ascii=False),flush=True)
    if 'deep sleep' in line or '深睡' in line:print(line,flush=True)
    if 'fortune: reset reason 8, clock kept' in line:
        assert sleep_at is not None
        result['deep_sleep_wake_seen']=True
    if sleep_at is not None and 'NABO fortune ready' in line:
        result['ready_after_wake']=True
        s.write(b'status\n')
    if result.get('ready_after_wake') and line.startswith('NF '):
        result['status_after_wake']=line;break
f.close()
if s is not None:s.close()
(out/'sleep-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)

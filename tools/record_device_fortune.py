"""Publish redacted M4 evidence. Raw serial/profile data remain in private/."""
import datetime,hashlib,json,re
from pathlib import Path
R=Path(__file__).resolve().parents[1]
build=json.loads((R/'validation/m4-build.json').read_text(encoding='utf-8'))
device=json.loads((R/'private/m4-readable-device/result.json').read_text(encoding='utf-8'))
upgrade=json.loads((R/'private/readable-upgrade/upgrade.json').read_text(encoding='utf-8'))
assert upgrade['app_sha256']==build['images']['app']['sha256']
assert upgrade['assets_sha256']==build['images']['assets']['sha256']
assert device['app_sha256']==build['images']['app']['sha256']
assert device['version']==build['version']
def profile(s):return re.search(r'pf=.*? batt=',s).group(0)
assert profile(device['initial_status'])==profile(device['final_status'])
names=['pre-route','daily','shake','lot','tarot','bone','star','ziwei','qimen','stress']
perf=[]
for row in device['records']:
    if row['command']!='perf':continue
    d=dict(re.findall(r'(frames|avg_ms|max_ms|over_33ms)=(\d+)',row['reply']))
    perf.append({'phase':names[len(perf)],**{k:int(v) for k,v in d.items()}})
sleep_file=R/('private/m4-v'+build['version'].replace('.','')+'-sleep/sleep-result.json')
sleep=json.loads(sleep_file.read_text(encoding='utf-8')) if sleep_file.exists() else {}
sleep_public={k:v for k,v in sleep.items() if not k.endswith('status') and k!='status_after_wake'}
if sleep.get('ready_after_wake'):
    before=sleep['initial_status'];after=sleep['status_after_wake']
    sleep_public['profile_retained_after_wake']=profile(before)==profile(after)
    sleep_public['wake_clock_is_calibrated']='(未校准)' not in after
    def clock(s):return datetime.datetime.strptime(re.search(r'time=(\d{4}-\d\d-\d\d \d\d:\d\d)',s).group(1),'%Y-%m-%d %H:%M')
    sleep_public['clock_advanced_minutes']=(clock(after)-clock(before)).total_seconds()/60
    assert sleep_public['profile_retained_after_wake'],'Birth profile differs after wake'
    assert sleep_public['wake_clock_is_calibrated'],'Wake clock became unknown'
    if sleep.get('physical_input_events')==0:
        assert 4 <= sleep_public['clock_advanced_minutes'] <= 10,'Clock did not advance through real sleep'
feedback=json.loads((R/'validation/M4-user-feedback.json').read_text(encoding='utf-8'))
assert feedback['version']==build['version']
power_file=R/('private/powercycle-v'+build['version'].replace('.','')+'/result.json')
power=json.loads(power_file.read_text(encoding='utf-8')) if power_file.exists() else {}
if power:assert power['version']==build['version']
result={
 'date':'2026-10-22','version':build['version'],'acceptance':'manual_retest_accepted_other_device_checks_pending',
 'app_sha256':build['images']['app']['sha256'],'assets_sha256':build['images']['assets']['sha256'],
 'device':{'chip':'ESP32-C3 rev 1.1','flash_bytes':8388608,'mac_suffix':'a9:58'},
 'installation':{'original_full_backup_digest_verified':True,'original_backup_is_private':True,
                 'mode':'app_only' if upgrade.get('assets_written') is False else 'app_and_assets_only','existing_bootloader_and_nvs_preserved':True,
                 'partition_table_compared_before_write':True,'app_and_assets_readback_digest_matched':True,
                 'merged_bootloader_not_flashed_in_this_upgrade':True},
 'serial':{k:device[k] for k in ('profile_preserved_mode','invalid_date_commands_rejected',
           'all_eight_play_routes_exercised','stress_rounds','unexpected_restarts',
           'double_up_returned_to_menu','sfx_triggers','image_misses_zero','serial_reply_retries','min_heap','min_largest')},
 'profile_before_and_after_equal':True,'performance_windows':perf,'sleep':sleep_public,
 'user_retest':feedback,'power_cycle':power,
 'private_serial_log_sha256':hashlib.sha256((R/'private/m4-readable-device/serial.log').read_bytes()).hexdigest(),
 'pending':[],
 'limits':['Serial key injection is not physical button verification.',
           'Average frame times include waits/result pages; lot still has frames above 33 ms.',
           'Host PCM without clipping does not establish speaker quality.']}
if not power.get('profile_equal_after_user_power_cycle'):result['pending'].append('power-off profile persistence')
if not (sleep.get('sleep_request_seen') and sleep.get('functional_sleep_and_wake_passed') and sleep.get('manual_screen_off_and_upper_wake_confirmed') and sleep.get('ready_after_wake') and 299000<=sleep.get('idle_before_sleep_ms',0)<=302000):result['pending'].append('five-minute deep sleep and manual wake')
if not feedback.get('accepted'):result['pending'].append('user physical retest')
if not result['pending']:result['acceptance']='complete_local_device_validation'
release_file=R/f"validation/RELEASE-v{build['version']}.json"
if release_file.exists():
    release=json.loads(release_file.read_text(encoding='utf-8'))
    assert release['images']=={k:build['images'][k]['sha256'] for k in ('bootloader','partition_table','app','assets')}
    result['release_boot_validation']=release
    result['installation']['existing_bootloader_and_nvs_preserved']=False
    result['installation']['nvs_preserved']=True
    result['installation']['bootloader_updated_to_release_build']=True
    result['installation']['merged_bootloader_not_flashed_in_this_upgrade']=False
(R/'validation/M4-device.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:result[k] for k in ('version','acceptance','serial','sleep','pending')},ensure_ascii=False,indent=2))

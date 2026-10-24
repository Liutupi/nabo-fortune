"""Exercise real firmware keys, printing, paging and time/profile changes under ASan/UBSan."""
import ctypes as C
import os
from pathlib import Path
import sys
import numpy as np
from run_ui import UI, L, UP, DOWN, OK, sheet

class Clock(C.Structure):
    _fields_=[('y',C.c_int16),('m',C.c_int8),('d',C.c_int8),('hh',C.c_int8),('mm',C.c_int8),('wd',C.c_int8)]
class Profile(C.Structure):
    _fields_=[('y',C.c_int16),('m',C.c_int8),('d',C.c_int8),('h',C.c_int8),('g',C.c_int8),('demo',C.c_bool)]

def main(blob,out):
    os.makedirs(out,exist_ok=True)
    u=UI(blob,seed=20261024)
    pf=Profile.in_dll(L,'PF')
    pf.y,pf.m,pf.d,pf.h,pf.g,pf.demo=1985,6,15,4,0,True
    def clock(y,m,d,h=0,mi=0):L.fc_set_now(C.byref(Clock(y,m,d,h,mi,0)))
    def pixels():return np.ctypeslib.as_array(L.host_fb(),shape=(320*240,)).copy().reshape(320,240)[30:316,:]
    shots=[]
    def snap(tag):
        p=os.path.join(out,tag+'.png');u.png(p);shots.append(p)
    def choose():
        u.go('daily');u.run(400);u.tap(DOWN);u.run(400)
    def draw():u.tap(OK);u.run(7000)
    clock(2026,10,24)
    # The normal menu reaches the daily scene, whose default remains the old ticket.
    u.go('menu');u.run(400);u.tap(OK);u.run(400)
    assert L.host_scene()==b'daily'
    snap('ssq_0choices');u.tap(DOWN);u.run(400);snap('ssq_1selected')
    u.key(OK,True);u.run(500);snap('ssq_2calculating');u.key(OK,False);u.run(1200);snap('ssq_2printing');u.run(7000)
    first=pixels();snap('ssq_3numbers')
    pages=[]
    for i in range(7):
        u.tap(DOWN);u.run(400);pages.append(pixels());snap(f'ssq_4basis_{i}')
        if np.array_equal(pages[-1],first):break
    assert len(pages)>2 and np.array_equal(pages[-1],first),'paging did not wrap to numbers'
    # Print again, change scene, reinitialise host (restart), and vary every hour.
    u.tap(OK);u.run(400);draw();assert np.array_equal(first,pixels()),'reprint changed numbers'
    for h in [1,11,12,23]:
        clock(2026,10,24,h,59);choose();draw();assert np.array_equal(first,pixels()),f'hour {h} changed numbers'
    clock(2026,10,25);choose();draw();assert not np.array_equal(first,pixels()),'next day did not change ticket'
    clock(2026,10,24);pf.h=5;choose();draw();assert not np.array_equal(first,pixels()),'profile was ignored';pf.h=4
    assert u.bad==0 and L.fc_ui_image_misses()==0
    earlier_frames=u.frames
    u=UI(blob,seed=999)
    clock(2026,10,24,23,59);choose();draw();assert np.array_equal(first,pixels()),'restart/global seed changed ticket'
    # Return and print the original daily ticket, and check all detail pages.
    u.tap(OK);u.run(400);u.tap(UP);u.run(400);draw();snap('daily_original')
    assert not np.array_equal(first,pixels())
    for _ in range(5):u.tap(DOWN);u.run(400)
    # Repeated mode/scene changes should neither exhaust image handles nor overflow lists.
    for _ in range(12):
        choose();u.tap(OK);u.run(400);u.go('menu');u.run(400)
    assert u.bad==0 and L.fc_ui_image_misses()==0
    sheet(shots,os.path.join(out,'sheet_ssq.png'),cols=4)
    print(f'SSQ UI PASS: paging, reprint, hours, midnight, profile, restart, original ticket; frames={earlier_frames+u.frames}, mismatches={u.bad}, image_misses={L.fc_ui_image_misses()}')

if __name__=='__main__':main(sys.argv[1],sys.argv[2])

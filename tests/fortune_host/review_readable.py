"""Native-size visual review plus complete bone-text wrapping checks."""
import ctypes as C
import json,os,sys
from run_ui import UI,L,UP,DOWN,OK,sheet

blob,out=sys.argv[1:3];os.makedirs(out,exist_ok=True)
u=UI(blob,seed=20261022);shots=[]
def snap(name):
    path=os.path.join(out,name+'.png');u.png(path);shots.append(path)
for scene in ('menu','birth','clock'):
    u.go(scene);u.run(500);snap(scene)
u.go('daily');u.tap(OK);u.run(8500);snap('daily')
u.tap(DOWN);u.run(200);snap('daily_page2')
L.host_set_lot(1);u.go('lot');u.key(OK,1);u.run(2200);u.key(OK,0);u.run(10000);snap('lot')
u.tap(DOWN);u.run(200);snap('lot_page2')
u.go('tarot');u.run(4000);snap('tarot_fan');u.tap(OK);u.run(1500);snap('tarot_poise');u.run(3000);snap('tarot')
u.go('bone');u.tap(OK);u.run(1600);snap('bone_drop');u.run(5200);snap('bone_slide');u.run(2800);snap('bone_balance')
u.tap(OK);u.run(900);snap('bone_white')
u.tap(DOWN);u.run(200);snap('bone_poem')
u.tap(DOWN);u.run(200);snap('bone_parts')
u.go('ziwei');u.tap(OK);u.run(3500);snap('ziwei_chart');u.tap(OK);u.run(1400);snap('ziwei_detail')
u.tap(OK);u.tap(DOWN);u.run(200);u.tap(OK);u.run(200);snap('ziwei_next_palace')
u.tap(DOWN);u.run(200);snap('ziwei_next_page')
u.go('qimen');u.tap(OK);u.run(4800);snap('qimen_chart');u.tap(OK);u.run(1400);snap('qimen_detail')
u.tap(DOWN);u.run(200);snap('qimen_page2')
u.tap(DOWN);u.run(200);snap('qimen_page3')
u.go('star');u.tap(OK);u.run(4000);snap('star')
font=C.addressof(C.c_char.in_dll(L,'F_SANS11'))
L.wrap_px.argtypes=[C.c_void_p,C.c_char_p,C.c_int,C.POINTER(C.c_char_p),C.POINTER(C.c_int),C.c_int]
L.fc_text_width.argtypes=[C.c_void_p,C.c_char_p]
counts=[]
for text in (C.c_char_p*9).in_dll(L,'FT_BONE_TEXT'):
    lines=(C.c_char_p*16)();lens=(C.c_int*16)()
    n=L.wrap_px(font,text,192,lines,lens,16)
    assert sum(lens[:n])==len(text)
    assert n<=7,(n,text)
    for i in range(n):assert L.fc_text_width(font,lines[i][:lens[i]])<=192
    counts.append(n)
poem_font=C.addressof(C.c_char.in_dll(L,'F_KAI13'))
poem_max_width=0
for raw in (C.c_char_p*52).in_dll(L,'FT_BONE_POEM'):
    text=raw.decode();parts=text.split('，')
    verses=[x+'，' for x in parts[:-1]]+([parts[-1]] if parts[-1] else [])
    assert len(verses)<=6
    for verse in verses:
        width=L.fc_text_width(poem_font,verse.encode());assert width<=192
        poem_max_width=max(poem_max_width,width)
sheet(shots,os.path.join(out,'review-sheet.png'),cols=4)
assert L.fc_ui_image_misses()==0
result={'frames':u.frames,'mismatchFrames':u.bad,'maxOps':u.maxops,'image_misses':L.fc_ui_image_misses(),'bone_text_line_counts':counts,'all_bone_text_bytes_displayed':True,'bone_poems_fit':52,'max_poem_verse_width':poem_max_width}
open(os.path.join(out,'readability-check.json'),'w').write(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))

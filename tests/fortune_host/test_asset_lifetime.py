"""Regression: repeated long-named tarot art must not hide later cards/scale parts."""
import ctypes as C,json,struct,sys
from pathlib import Path
from run_ui import UI,L
u=UI(sys.argv[1],seed=42)
L.prop.argtypes=[C.c_char_p];L.prop.restype=C.c_void_p
raw=Path(sys.argv[1]).read_bytes()
props=[]
for i in range(struct.unpack_from('<H',raw,6)[0]):
    name=raw[8+40*i:40+40*i].split(b'\0')[0].decode()
    if name.startswith('prop/'):props.append(name[5:])
long_name=b'tarot-10-wheel-of-fortune'
first=L.prop(long_name)
assert first,'First wheel-of-fortune lookup failed'
for i in range(200):
    assert L.prop(long_name)==first,f'Long-named tarot did not reuse its stable handle at repeat {i}'
for turn in range(3):
    for name in props:assert L.prop(name.encode()),f'Image missing after tarot repeats: {name}'
for name in ('scale-beam','scale-pan','scale-weight','weight-nian','weight-yue','weight-ri','weight-shi'):
    assert L.prop(name.encode()),name
class Image(C.Structure):
    _fields_=[('data',C.c_void_p),('pal',C.c_void_p),('offs',C.c_void_p),
              ('w',C.c_uint16),('h',C.c_uint16),('frames',C.c_uint16),('loop',C.c_uint8)]
L.fc_img_frame.argtypes=[C.POINTER(Image),C.c_int]
L.fc_img_frame.restype=C.POINTER(C.c_uint8)
cards=[]
for name in props:
    if not name.startswith('tarot-'):continue
    im=C.cast(L.prop(name.encode()),C.POINTER(Image))
    frame=L.fc_img_frame(im,0)
    assert frame,name
    visible=sum(v!=0 for v in frame[:im.contents.w*im.contents.h])
    assert visible>im.contents.w*im.contents.h//3,(name,visible)
    cards.append({'name':name,'visible_pixels':visible})
assert len(cards)==22
assert L.fc_ui_image_misses()==0
result={'long_tarot_repeats':200,'all_prop_lookups':len(props)*3,'scale_parts_present':True,'all_22_cards_decode_nontransparent':True,'cards':cards}
if len(sys.argv)>2:Path(sys.argv[2]).write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='cards'}))

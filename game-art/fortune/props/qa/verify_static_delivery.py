"""Read-only asset checks and review contact sheets; no production-art edits."""
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ART=Path(__file__).resolve().parents[2]
PROJECT=ART.parents[1]
PROPS=ART/'props'
TAROT=ART/'tarot'
BG=(13,8,32)
FONT=ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',15)
SMALL=ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf',12)
TARGETS={
 'coin-front':(22,22),'coin-back':(22,22),'shell':(68,40),'tube':(64,110),'stick':(6,80),'scroll':(208,216),
 'seal-shangshang':(52,40),'seal-shang':(52,40),'seal-zhong':(52,40),'seal-xia':(52,40),
 'scale-beam':(182,6),'scale-pan':(56,16),'scale-weight':(18,16),
 'weight-nian':(12,12),'weight-yue':(12,12),'weight-ri':(12,12),'weight-shi':(12,12),
 'printer':(212,28),'card-back-s':(44,72),'card-back-l':(82,135),'title':(200,70),
 **{f'trigram-{i}':(20,14) for i in range(8)}
}

def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def inspect(p):
    im=Image.open(p).convert('RGBA');a=np.asarray(im)
    solid=a[...,3]==255;visible=a[...,3]>0
    colors=len(np.unique(a[solid,:3],axis=0)) if solid.any() else 0
    return {'path':p.relative_to(ART).as_posix(),'width':im.width,'height':im.height,
            'sha256':digest(p),'alpha_values':np.unique(a[...,3]).tolist(),
            'opaque_rgb_colors':colors,'ink_bbox':list(im.getbbox()),'visible_pixels':int(visible.sum())}

props=[]
for ident,size in TARGETS.items():
    native=inspect(PROPS/(ident+'.png'));master=inspect(PROPS/(ident+'-4x.png'))
    assert (native['width'],native['height'])==size,ident
    assert (master['width'],master['height'])==(size[0]*4,size[1]*4),ident
    assert native['visible_pixels']>0 and master['visible_pixels']>0,ident
    assert native['opaque_rgb_colors']<=64 and master['opaque_rgb_colors']<=64,ident
    if ident!='shell':assert set(native['alpha_values'])<= {0,255},ident
    if ident.startswith('seal-'):source='seal-blank'
    elif ident.startswith('weight-'):source='weight-blank'
    elif ident.startswith('card-back-'):source='card-back'
    else:source=ident
    generated=PROPS/'originals'/(source+'.png')
    source_path=generated.relative_to(ART).as_posix() if generated.exists() else ('props/shell-original.png' if ident=='shell' else 'props/export-static-assets.ps1')
    props.append({'id':ident,'native':native,'master_4x':master,'source':source_path})

for ident in ('coin-front','coin-back'):
    a=np.asarray(Image.open(PROPS/(ident+'.png')).convert('RGBA'))
    assert a[11,11,3]==0,ident+' square hole'
assert digest(PROPS/'coin-front.png')!=digest(PROPS/'coin-back.png')
bits=[7,3,5,1,6,2,4,0]
trigram_names=['乾','兑','离','震','巽','坎','艮','坤']
for i,b in enumerate(bits):
    a=np.asarray(Image.open(PROPS/f'trigram-{i}.png').convert('RGBA'))
    for row in range(3):
        line=a[3+row*4,:,3]>0
        starts=int(np.count_nonzero(line & ~np.r_[False,line[:-1]]))
        assert starts==(1 if b & (1<<(2-row)) else 2),(i,row,starts)

plan=json.loads((TAROT/'generation-plan.json').read_text(encoding='utf-8'))
cards=[];frame_reference=None
for card in plan['cards']:
    ident=card['id'];native=inspect(TAROT/(ident+'.png'));master=inspect(TAROT/'masters'/(ident+'-4x.png'))
    raw=Image.open(TAROT/'originals'/(ident+'.png'))
    assert (native['width'],native['height'])==(82,135),ident
    assert (master['width'],master['height'])==(328,540),ident
    assert raw.width>=328 and raw.height>=540,ident
    assert native['alpha_values']==[255],ident
    assert native['opaque_rgb_colors']<=64 and master['opaque_rgb_colors']<=64,ident
    a=np.asarray(Image.open(TAROT/'masters'/(ident+'-4x.png')).convert('RGB'))
    assert np.all(a[22:50,22:306]==BG),ident+' number panel'
    assert np.all(a[474:518,22:306]==BG),ident+' name panel'
    mask=np.ones((540,328),bool);mask[56:468,24:304]=False
    frame=a[mask]
    if frame_reference is None:frame_reference=frame
    else:assert np.array_equal(frame,frame_reference),ident+' consistent frame'
    cards.append({'id':ident,'index':card['index'],'chinese_name':'隐士' if card['index']==9 else card['chinese_name'],
                  'native':native,'master_4x':master,'source':f'tarot/originals/{ident}.png',
                  'source_sha256':digest(TAROT/'originals'/(ident+'.png'))})
assert len(props)==29 and len(cards)==22

protected=json.loads((PROPS/'qa/protected-input-hashes.json').read_text(encoding='utf-8-sig'))
for entry in protected:assert digest(PROJECT/entry['path'])==entry['sha256'],entry['path']+' changed'
prior=json.loads((ART/'grok-start/master-checks.json').read_text(encoding='utf-8-sig'))
for entry in prior:
    if entry['file'].startswith('props') or entry['file'].startswith('costume'):
        assert digest(ART/entry['file'].replace('\\','/'))==entry['sha256'],entry['file']+' changed'

(PROPS/'asset-manifest.json').write_text(json.dumps({
 'schema_version':1,'date':'2026-10-22','asset_root':'game-art/fortune','assets':props,
 'trigram_order':trigram_names,'trigram_bits_bottom_to_top':bits,
 'text_safe_rects':{'scroll':[30,36,148,144]},
 'generated_source_designs':11,'note':'Geometry, exact Chinese glyphs and output sizing are in export-static-assets.ps1. Approved shell is reused unchanged. Use explicit asset paths, not wildcard source PNGs.'
},ensure_ascii=False,indent=2),encoding='utf-8')
(TAROT/'asset-manifest.json').write_text(json.dumps({
 'schema_version':1,'date':'2026-10-22','asset_root':'game-art/fortune','cards':cards,
 'number_rect_native':[6,5,70,8],'name_rect_native':[6,119,70,10],
 'illustration_rect_native':[6,14,70,103],
 'baked_text':'Only original NABO cap logo; no numerals or card names. Engine supplies all labels.',
 'index_order_note':'8 strength / 11 justice; 9 hermit is labelled 隐士 in current prototype, synonym 隐者; 20 filename uses judgement.'
},ensure_ascii=False,indent=2),encoding='utf-8')

def batch_sheet(batch,filename):
    sheet=Image.new('RGB',(1456,496),BG);d=ImageDraw.Draw(sheet)
    for col,c in enumerate(batch):
        im=Image.open(TAROT/(c['id']+'.png')).convert('RGBA');x=16+col*180
        sheet.paste(im.resize((164,270),Image.Resampling.NEAREST),(x,42),im.resize((164,270),Image.Resampling.NEAREST))
        sheet.paste(im,(x+41,342),im)
        d.text((x,10),f"{c['index']:02d}  {c['chinese_name']}",font=FONT,fill='#35f0dc')
    sheet.save(TAROT/filename)
batch_sheet(cards[:8],'batch-00-07.png');batch_sheet(cards[8:15],'batch-08-14.png');batch_sheet(cards[15:],'batch-15-21.png')
sheet=Image.new('RGB',(1456,1030),BG);d=ImageDraw.Draw(sheet)
d.text((18,12),'NABO / 22 original major arcana / costume C',font=FONT,fill='white')
for i,c in enumerate(cards):
    x=16+(i%8)*180;y=58+(i//8)*322
    im=Image.open(TAROT/(c['id']+'.png')).convert('RGBA').resize((164,270),Image.Resampling.NEAREST)
    sheet.paste(im,(x,y),im);d.text((x,y+278),f"{c['index']:02d} {c['chinese_name']}",font=FONT,fill='#35f0dc')
sheet.save(TAROT/'all-22-overview.png')

sheet=Image.new('RGB',(1220,1780),BG);d=ImageDraw.Draw(sheet)
d.text((20,15),'NABO / 29 static props / native size',font=FONT,fill='white')
for i,p in enumerate(props):
    im=Image.open(ART/p['native']['path']).convert('RGBA');x=20+(i%5)*244;y=55+(i//5)*285
    d.text((x,y),p['id'],font=SMALL,fill='#35f0dc');d.text((x,y+20),f'{im.width} x {im.height}',font=SMALL,fill='white')
    sheet.paste(im,(x+(220-im.width)//2,y+52+(216-im.height)//2),im)
sheet.save(PROPS/'qa/props-native-overview.png')
focus=['coin-front','coin-back','seal-shangshang','seal-shang','seal-zhong','seal-xia','weight-nian','weight-yue','weight-ri','weight-shi','tube','printer','title']
sheet=Image.new('RGB',(1300,1020),BG);d=ImageDraw.Draw(sheet)
for i,ident in enumerate(focus):
    im=Image.open(PROPS/(ident+'.png')).convert('RGBA');x=20+(i%4)*325;y=15+(i//4)*248
    scale=min(4,280/im.width,195/im.height);size=(round(im.width*scale),round(im.height*scale))
    im=im.resize(size,Image.Resampling.NEAREST);sheet.paste(im,(x,y+30),im);d.text((x,y),ident,font=SMALL,fill='#35f0dc')
sheet.save(PROPS/'qa/props-detail-check.png')

summary={'date':'2026-10-22','native_assets':len(props)+len(cards),'props':len(props),'tarot_cards':len(cards),
 'master_4x_count':len(props)+len(cards),'expected_dimensions_ok':True,'shared_tarot_frame_identical':True,
 'tarot_label_reservations_blank':True,'coins_have_transparent_square_holes':True,'eight_trigram_topologies_ok':True,
 'new_props_alpha_binary':True,'reused_shell_and_costume_unchanged':True,'protected_inputs_hash_match':True,
 'max_native_opaque_colors':max(p['native']['opaque_rgb_colors'] for p in props+cards),
 'max_master_opaque_colors':max(p['master_4x']['opaque_rgb_colors'] for p in props+cards),
 'generated_high_resolution_originals':len(list((PROPS/'originals').glob('*.png')))+len(list((TAROT/'originals').glob('*.png'))),
 'reuse_note':'Approved turtle shell has its original high-resolution source separately in props/shell-original.png.',
 'not_verified':['engine integration','indexed sprite binary packing','animation playback with these props','RGB565 device rendering']}
(PROPS/'qa/verification.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False,indent=2))

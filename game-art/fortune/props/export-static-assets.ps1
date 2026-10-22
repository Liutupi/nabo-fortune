param([switch]$PropsOnly,[switch]$TarotOnly)
$ErrorActionPreference='Stop'
$artRoot=Split-Path -Parent $PSScriptRoot
. (Join-Path $artRoot 'costume\export-art.ps1')
if (-not ('FortuneStaticArt' -as [type])) {
$refs=[AppDomain]::CurrentDomain.GetAssemblies() | Where-Object {-not $_.IsDynamic -and $_.Location} | ForEach-Object {$_.Location} | Sort-Object -Unique
Add-Type -ReferencedAssemblies $refs -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class FortuneStaticArt {
 public static readonly string[] Palette={
  "#FAF8FF","#C6C1DA","#F5CAA9","#E5A086","#50383C","#241E28",
  "#572DAB","#331B72","#B59AEA","#8C73C6","#35F0DC","#23AFA8",
  "#124C8C","#302B3A","#54505E","#6C82B3","#3D4B72","#A7573A","#171321",
  "#FFC94A","#A66D36","#4A2A24","#F2E4C0","#D7BC83","#E0402C","#9E2826",
  "#4F1825","#FF3DA6","#9E2768","#0D0820","#1B123B","#91C8C1","#497E83","#A8B0C4","#616575"
 };
 static Color C(string hex){return ColorTranslator.FromHtml(hex);}
 public static Bitmap Finish(Bitmap input){
  {
   Bitmap b=new Bitmap(input.Width,input.Height,PixelFormat.Format32bppArgb);
   using(Graphics g=Graphics.FromImage(b)){g.CompositingMode=CompositingMode.SourceCopy;g.DrawImageUnscaled(input,0,0);}
   Color[] colors=Array.ConvertAll(Palette,x=>C(x));
   var bd=b.LockBits(new Rectangle(0,0,b.Width,b.Height),ImageLockMode.ReadWrite,PixelFormat.Format32bppArgb);
   byte[] px=new byte[bd.Stride*b.Height];Marshal.Copy(bd.Scan0,px,0,px.Length);
   for(int y=0;y<b.Height;y++)for(int x=0;x<b.Width;x++){
    int j=y*bd.Stride+x*4;
    if(px[j+3]<128)px[j]=px[j+1]=px[j+2]=px[j+3]=0;
    else {
     int best=0,md=int.MaxValue;
     for(int k=0;k<colors.Length;k++){
      int dr=px[j+2]-colors[k].R,dg=px[j+1]-colors[k].G,db=px[j]-colors[k].B,d=2*dr*dr+4*dg*dg+3*db*db;
      if(d<md){md=d;best=k;}
     }
     px[j]=colors[best].B;px[j+1]=colors[best].G;px[j+2]=colors[best].R;px[j+3]=255;
    }
   }
   Marshal.Copy(px,0,bd.Scan0,px.Length);b.UnlockBits(bd);return b;
  }
 }
 static Rectangle Bounds(Bitmap b){
  int l=b.Width,t=b.Height,r=-1,d=-1;
  for(int y=0;y<b.Height;y++)for(int x=0;x<b.Width;x++)if(b.GetPixel(x,y).A>=128){l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x);d=Math.Max(d,y);}
  if(r<l)throw new Exception("Empty source");return Rectangle.FromLTRB(l,t,r+1,d+1);
 }
 static Graphics GraphicsFor(Bitmap b){
  Graphics g=Graphics.FromImage(b);g.SmoothingMode=SmoothingMode.AntiAlias;
  g.CompositingMode=CompositingMode.SourceOver;g.InterpolationMode=InterpolationMode.HighQualityBicubic;
  g.PixelOffsetMode=PixelOffsetMode.HighQuality;return g;
 }
 public static Bitmap Fit(string source,int w,int h){
  Bitmap result=new Bitmap(w,h,PixelFormat.Format32bppArgb);
  using(Bitmap src=new Bitmap(source))using(Graphics g=GraphicsFor(result)){
   g.Clear(Color.Transparent);Rectangle box=Bounds(src);
   double s=Math.Min(w/(double)box.Width,h/(double)box.Height);
   int tw=(int)Math.Round(box.Width*s),th=(int)Math.Round(box.Height*s);
   g.DrawImage(src,new Rectangle((w-tw)/2,(h-th)/2,tw,th),box,GraphicsUnit.Pixel);
  }
  return result;
 }
 public static Bitmap FitExact(string source,int w,int h){
  Bitmap result=new Bitmap(w,h,PixelFormat.Format32bppArgb);
  using(Bitmap src=new Bitmap(source))using(Graphics g=GraphicsFor(result)){
   g.Clear(Color.Transparent);g.DrawImage(src,new Rectangle(0,0,w,h),Bounds(src),GraphicsUnit.Pixel);
  }
  return result;
 }
 static GraphicsPath TextPath(string text,string family,RectangleF target){
  GraphicsPath path=new GraphicsPath();
  using(FontFamily ff=new FontFamily(family))path.AddString(text,ff,(int)FontStyle.Bold,100,PointF.Empty,StringFormat.GenericTypographic);
  RectangleF b=path.GetBounds();float s=Math.Min(target.Width/b.Width,target.Height/b.Height);
  float tx=target.X+(target.Width-b.Width*s)/2-b.X*s,ty=target.Y+(target.Height-b.Height*s)/2-b.Y*s;
  using(Matrix m=new Matrix(s,0,0,s,tx,ty))path.Transform(m);return path;
 }
 static void Text(Graphics g,string text,RectangleF rect,string color){
  using(GraphicsPath p=TextPath(text,"楷体",rect))using(Brush b=new SolidBrush(C(color)))g.FillPath(b,p);
 }
 static void WeightGlyph(Graphics g,string text){
  string[] rows;
  if(text=="年")rows=new string[]{"0100000","0111110","0101000","0011110","0101000","1111111","0001000"};
  else if(text=="月")rows=new string[]{"0111110","0100010","0111110","0100010","0111110","0100010","1000110"};
  else if(text=="日")rows=new string[]{"0111110","0100010","0100010","0111110","0100010","0100010","0111110"};
  else rows=new string[]{"0000010","1111111","1010010","1110110","1010010","1110010","0000110"};
  g.SmoothingMode=SmoothingMode.None;
  using(Brush ink=new SolidBrush(C("#4A2A24")))for(int y=0;y<7;y++)for(int x=0;x<7;x++)if(rows[y][x]=='1')g.FillRectangle(ink,12+x*4,12+y*4,4,4);
 }
 public static void SavePair(Bitmap master,string folder,string id,int w,int h){
  using(Bitmap clean=Finish(master)){
   clean.Save(System.IO.Path.Combine(folder,id+"-4x.png"),ImageFormat.Png);
   using(Bitmap native=new Bitmap(w,h,PixelFormat.Format32bppArgb)){
    using(Graphics g=GraphicsFor(native)){g.Clear(Color.Transparent);g.DrawImage(clean,new Rectangle(0,0,w,h));}
    using(Bitmap small=Finish(native))small.Save(System.IO.Path.Combine(folder,id+".png"),ImageFormat.Png);
   }
  }
 }
 public static void Prop(string source,string folder,string id,int w,int h,string text){
  using(Bitmap b=FitExact(source,w*4,h*4)){
   using(Graphics g=GraphicsFor(b)){
    if(id=="tube")Text(g,"签",new RectangleF(76,200,104,128),"#4A2A24");
    if(id.StartsWith("seal-"))Text(g,text,new RectangleF(28,28,152,104),"#FAF8FF");
    if(id.StartsWith("weight-"))WeightGlyph(g,text);
   }
   if(id.StartsWith("coin-")){
    int side=16;for(int y=(b.Height-side)/2;y<(b.Height+side)/2;y++)for(int x=(b.Width-side)/2;x<(b.Width+side)/2;x++)b.SetPixel(x,y,Color.Transparent);
   }
   SavePair(b,folder,id,w,h);
  }
 }
 public static void Geometric(string folder,string id,int w,int h,int bits){
  using(Bitmap b=new Bitmap(w*4,h*4,PixelFormat.Format32bppArgb))using(Graphics g=GraphicsFor(b)){
   g.Clear(Color.Transparent);
   if(id=="stick"){
    using(Brush ink=new SolidBrush(C("#4A2A24")))g.FillRectangle(ink,2,0,20,320);
    using(Brush bamboo=new SolidBrush(C("#F2E4C0")))g.FillRectangle(bamboo,6,4,12,312);
    using(Brush shade=new SolidBrush(C("#A66D36")))g.FillRectangle(shade,16,4,4,312);
    using(Brush red=new SolidBrush(C("#E0402C")))g.FillRectangle(red,6,4,12,56);
    using(Pen joint=new Pen(C("#A66D36"),3)){g.DrawLine(joint,6,160,18,160);g.DrawLine(joint,6,240,18,240);}
   }else if(id.StartsWith("trigram-")){
    for(int row=0;row<3;row++){
     int y=8+row*16;bool yang=(bits&(1<<(2-row)))!=0;
     using(Brush gold=new SolidBrush(C("#FFC94A")))using(Brush shadow=new SolidBrush(C("#A66D36"))){
      if(yang){g.FillRectangle(shadow,4,y,72,8);g.FillRectangle(gold,4,y,72,6);}
      else {g.FillRectangle(shadow,4,y,28,8);g.FillRectangle(shadow,48,y,28,8);g.FillRectangle(gold,4,y,28,6);g.FillRectangle(gold,48,y,28,6);}
     }
    }
   }else if(id=="title"){
    using(GraphicsPath p=TextPath("NABO","Segoe UI Black",new RectangleF(36,8,728,116)))
    using(Pen outer=new Pen(C("#171321"),16))using(Pen edge=new Pen(C("#35F0DC"),8))using(Brush fill=new SolidBrush(C("#FF3DA6"))){g.DrawPath(outer,p);g.DrawPath(edge,p);g.FillPath(fill,p);}
    using(GraphicsPath p=TextPath("赛博算命机","黑体",new RectangleF(24,152,752,112)))
    using(Pen outer=new Pen(C("#171321"),12))using(Pen edge=new Pen(C("#9E2768"),6))using(Brush fill=new SolidBrush(C("#FFC94A"))){g.DrawPath(outer,p);g.DrawPath(edge,p);g.FillPath(fill,p);}
   }
   SavePair(b,folder,id,w,h);
  }
 }
 static GraphicsPath Rounded(float x,float y,float w,float h,float r){
  GraphicsPath p=new GraphicsPath();float d=r*2;
  p.AddArc(x,y,d,d,180,90);p.AddArc(x+w-d,y,d,d,270,90);p.AddArc(x+w-d,y+h-d,d,d,0,90);p.AddArc(x,y+h-d,d,d,90,90);p.CloseFigure();return p;
 }
 public static void Tarot(string source,string folder,string id){
  using(Bitmap scene=Fit(source,280,412))using(Bitmap master=new Bitmap(328,540,PixelFormat.Format32bppArgb)){
   using(Graphics g=GraphicsFor(master)){
    g.Clear(C("#0D0820"));g.DrawImageUnscaled(scene,24,56);
    using(GraphicsPath a=Rounded(4,4,320,532,16))using(Pen gold=new Pen(C("#FFC94A"),4))g.DrawPath(gold,a);
    using(GraphicsPath a=Rounded(16,16,296,508,10))using(Pen gold=new Pen(C("#A66D36"),2))g.DrawPath(gold,a);
    // Blank top and bottom typography reservations. Engine draws both labels.
    using(Brush bg=new SolidBrush(C("#0D0820"))){g.FillRectangle(bg,22,22,284,28);g.FillRectangle(bg,22,474,284,44);}
   }
   using(Bitmap clean=Finish(master)){
    clean.Save(System.IO.Path.Combine(folder,"masters",id+"-4x.png"),ImageFormat.Png);
    using(Bitmap small=new Bitmap(82,135,PixelFormat.Format32bppArgb)){
     using(Graphics g=GraphicsFor(small)){g.Clear(C("#0D0820"));g.DrawImage(clean,new Rectangle(0,0,82,135));}
     using(Bitmap outp=Finish(small))outp.Save(System.IO.Path.Combine(folder,id+".png"),ImageFormat.Png);
    }
   }
  }
 }
}
'@
}
if(-not $TarotOnly){
 $jobs=@(
  @('coin-front','coin-front',22,22,''),@('coin-back','coin-back',22,22,''),
  @('tube','tube',64,110,''),@('scroll','scroll',208,216,''),
  @('seal-shangshang','seal-blank',52,40,'上上'),@('seal-shang','seal-blank',52,40,'上'),
  @('seal-zhong','seal-blank',52,40,'中'),@('seal-xia','seal-blank',52,40,'下'),
  @('scale-beam','scale-beam',182,6,''),@('scale-pan','scale-pan',56,16,''),@('scale-weight','scale-weight',18,16,''),
  @('weight-nian','weight-blank',12,12,'年'),@('weight-yue','weight-blank',12,12,'月'),
  @('weight-ri','weight-blank',12,12,'日'),@('weight-shi','weight-blank',12,12,'时'),
  @('printer','printer',212,28,''),@('card-back-s','card-back',44,72,''),@('card-back-l','card-back',82,135,'')
 )
 foreach($j in $jobs){[FortuneStaticArt]::Prop((Join-Path $PSScriptRoot ('originals\'+$j[1]+'.png')),$PSScriptRoot,$j[0],$j[2],$j[3],$j[4])}
 [FortuneStaticArt]::Geometric($PSScriptRoot,'stick',6,80,0)
 [FortuneStaticArt]::Geometric($PSScriptRoot,'title',200,70,0)
 $bits=@(7,3,5,1,6,2,4,0)
 for($i=0;$i -lt 8;$i++){[FortuneStaticArt]::Geometric($PSScriptRoot,('trigram-'+$i),20,14,$bits[$i])}
}
if(-not $PropsOnly){
 $tarot=Join-Path $artRoot 'tarot'
 Get-ChildItem -LiteralPath (Join-Path $tarot 'originals') -Filter '*.png' | Where-Object {$_.BaseName -match '^\d\d-'} | ForEach-Object {[FortuneStaticArt]::Tarot($_.FullName,$tarot,$_.BaseName)}
}
[FortuneStaticArt]::Palette | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'round34-palette.json') -Encoding utf8

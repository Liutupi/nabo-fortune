$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not ('FortuneArtExport' -as [type])) {
$drawingRefs = [AppDomain]::CurrentDomain.GetAssemblies() | Where-Object { -not $_.IsDynamic -and $_.Location } | ForEach-Object { $_.Location } | Sort-Object -Unique
Add-Type -ReferencedAssemblies $drawingRefs -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;
public static class FortuneArtExport {
 public static readonly string[] Palette = {
  "#FAF8FF", "#C6C1DA", "#F5CAA9", "#E5A086", "#50383C", "#241E28",
  "#572DAB", "#331B72", "#B59AEA", "#8C73C6", "#35F0DC", "#23AFA8",
  "#124C8C", "#302B3A", "#54505E", "#6C82B3", "#3D4B72", "#A7573A", "#171321"
 };
 public static Bitmap Flat(Bitmap input) {return FlatColors(input,Palette);}
 public static Bitmap FlatColors(Bitmap input,string[] palette) {
  Bitmap b = new Bitmap(input.Width,input.Height,PixelFormat.Format32bppArgb);
  using(Graphics g=Graphics.FromImage(b)) {g.CompositingMode=CompositingMode.SourceCopy;g.DrawImageUnscaled(input,0,0);}
  var bd=b.LockBits(new Rectangle(0,0,b.Width,b.Height),ImageLockMode.ReadWrite,PixelFormat.Format32bppArgb);
  byte[] px=new byte[bd.Stride*b.Height]; Marshal.Copy(bd.Scan0,px,0,px.Length);
  Color[] colors=Array.ConvertAll(palette, x => ColorTranslator.FromHtml(x));
  for(int y=0;y<b.Height;y++) for(int x=0;x<b.Width;x++) {
   int j=y*bd.Stride+x*4;
   if(px[j+3]<16){px[j]=px[j+1]=px[j+2]=px[j+3]=0;continue;}
   if(px[j+3]>=240)px[j+3]=255;
   int best=0,md=int.MaxValue;
   for(int k=0;k<colors.Length;k++) {
    int dr=px[j+2]-colors[k].R,dg=px[j+1]-colors[k].G,db=px[j]-colors[k].B;
    int d=2*dr*dr+4*dg*dg+3*db*db;
    if(d<md){md=d;best=k;}
   }
   px[j]=colors[best].B;px[j+1]=colors[best].G;px[j+2]=colors[best].R;
  }
  Marshal.Copy(px,0,bd.Scan0,px.Length);b.UnlockBits(bd);return b;
 }
 public static Rectangle Bounds(Bitmap b,int threshold) {
  int l=b.Width,t=b.Height,r=-1,d=-1;
  for(int y=0;y<b.Height;y++) for(int x=0;x<b.Width;x++) if(b.GetPixel(x,y).A>=threshold) {
   l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x);d=Math.Max(d,y);
  }
  if(r<l)throw new Exception("Empty image");return Rectangle.FromLTRB(l,t,r+1,d+1);
 }
 public static Rectangle HatBounds(Bitmap b) {
  int l=b.Width,t=b.Height,r=-1,d=-1;
  for(int y=0;y<b.Height*0.42;y++)for(int x=0;x<b.Width;x++) {
   Color c=b.GetPixel(x,y);
   if(c.A>=240&&c.R>170&&c.G>165&&c.B>190&&Math.Abs(c.R-c.G)<35) {
    l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x);d=Math.Max(d,y);
   }
  }
  if(r<l)throw new Exception("Hat not found");return Rectangle.FromLTRB(l,t,r+1,d+1);
 }
 public static int OpaqueColors(Bitmap b) {
  bool[] seen=new bool[16777216];int count=0;
  var bd=b.LockBits(new Rectangle(0,0,b.Width,b.Height),ImageLockMode.ReadOnly,PixelFormat.Format32bppArgb);
  byte[] px=new byte[bd.Stride*b.Height];Marshal.Copy(bd.Scan0,px,0,px.Length);b.UnlockBits(bd);
  for(int y=0;y<b.Height;y++)for(int x=0;x<b.Width;x++) {
   int j=y*bd.Stride+x*4;if(px[j+3]==255) {
    int color=(px[j+2]<<16)|(px[j+1]<<8)|px[j];if(!seen[color]){seen[color]=true;count++;}
   }
  }
  return count;
 }
 public static int BorderMismatch(Bitmap b,string background) {
  int expected=ColorTranslator.FromHtml(background).ToArgb(),errors=0;
  for(int y=0;y<b.Height;y++)for(int x=0;x<b.Width;x++)if(y<8||x<8||x>=b.Width-8) {
   if(b.GetPixel(x,y).ToArgb()!=expected)errors++;
  }
  return errors;
 }
 public static double[] AlignExport(string source,string destination,string template,string background,string[] palette) {
  using(Bitmap raw=new Bitmap(source))using(Bitmap reference=new Bitmap(template)) {
   Rectangle a=HatBounds(raw),b=HatBounds(reference);
   double scale=b.Width/(double)a.Width,ox=b.X-a.X*scale,oy=b.Y-a.Y*scale;
   using(Bitmap flat=FlatColors(raw,palette))using(Bitmap aligned=new Bitmap(reference.Width,reference.Height,PixelFormat.Format32bppArgb)) {
    using(Graphics g=Graphics.FromImage(aligned)) {
     g.Clear(Color.Transparent);g.CompositingMode=CompositingMode.SourceCopy;
     g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.PixelOffsetMode=PixelOffsetMode.HighQuality;
     g.DrawImage(flat,new RectangleF((float)ox,(float)oy,(float)(raw.Width*scale),(float)(raw.Height*scale)));
    }
    using(Bitmap clean=FlatColors(aligned,palette)) {
     if(String.IsNullOrEmpty(background))clean.Save(destination,ImageFormat.Png);
     else using(Bitmap output=new Bitmap(reference.Width,reference.Height,PixelFormat.Format32bppArgb)) {
      using(Graphics g=Graphics.FromImage(output)){g.Clear(ColorTranslator.FromHtml(background));g.DrawImageUnscaled(clean,0,0);}
      output.Save(destination,ImageFormat.Png);
     }
    }
   }
   return new double[]{scale,ox,oy};
  }
 }
 public static void Export(string source,string destination,int width,int height,string background,bool flatten,bool normalize) {
  using(Bitmap raw=new Bitmap(source)) {
   Bitmap processed=flatten?Flat(raw):(Bitmap)raw.Clone();
   using(processed) using(Bitmap scaled=new Bitmap(width,height,PixelFormat.Format32bppArgb)) {
    Rectangle bounds=normalize?Bounds(processed,128):new Rectangle(0,0,raw.Width,raw.Height);
    Rectangle target=new Rectangle(0,0,width,height);
    if(normalize) {
     double s=Math.Min(width/(double)bounds.Width,height/(double)bounds.Height);
     int w=(int)Math.Round(bounds.Width*s),h=(int)Math.Round(bounds.Height*s);
     target=new Rectangle((width-w)/2,(height-h)/2,w,h);
    }
    using(Graphics g=Graphics.FromImage(scaled)) {
     g.Clear(Color.Transparent);g.CompositingMode=CompositingMode.SourceCopy;
     g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.PixelOffsetMode=PixelOffsetMode.HighQuality;
     g.DrawImage(processed,target,bounds,GraphicsUnit.Pixel);
    }
    Bitmap cleaned=flatten?Flat(scaled):(Bitmap)scaled.Clone();
    using(cleaned) {
     if(String.IsNullOrEmpty(background))cleaned.Save(destination,ImageFormat.Png);
     else using(Bitmap outp=new Bitmap(width,height,PixelFormat.Format32bppArgb)) {
      using(Graphics g=Graphics.FromImage(outp)) {g.Clear(ColorTranslator.FromHtml(background));g.DrawImageUnscaled(cleaned,0,0);}
      outp.Save(destination,ImageFormat.Png);
     }
    }
   }
  }
 }
}
'@
}


/*
BufferA definiert die Flamen, dabei werden in pnum die Anzahl festgelgt ( bei 800*450 also 450 / 50 ergeben sich 10 Koordinatenpaare, 
dabei wird der Maus der 10te Wert zugeordnet. Wird die Maus auf 0,0 gestellt, so wird ein Pentatgramm gezeichnet.

BufferB erzeugt den Raucheffekt

iChannel0 definiert die Zufallswerte für die Schrift
iChannel1 definiert das Ruckeln und Flickern
iChannel2 definiert das Hintergrundbild (Blending)
*/
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Blending' to iChannel3
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel2
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// writing random endless scribbles
// by summing up low band noised curvature

// fragColor: red = writing, blue = burn mask

#define PI2 6.28318530717959f
//#define PNUM 40

__DEVICE__ float2 filterUV1(float2 uv, float2 iResolution) 
{
    // iq's improved texture filtering (https://www.shadertoy.com/view/XsfGDn)
    float2 _x=uv*iResolution;
    float2 p = _floor(_x);
    float2 f = fract_f2(_x);
    f = f*f*(3.0f-2.0f*f);
    return (p+f)/iResolution;
}

__DEVICE__ float4 getPixel(int _x, int _y, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    return texture(iChannel0,to_float2((float)(_x)+0.5f,(float)(_y)+0.5f)/iResolution);
}

__DEVICE__ bool isPixel(int _x, int _y, float2 fragCoord, float2 iResolution)
{
    float2 c=fragCoord/iResolution*iResolution;
    return ( (int)(c.x)==_x && (int)(c.y)==_y );
}

__DEVICE__ float2 readPos(int i, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    return swi2(getPixel(i,0, iResolution, iChannel0),x,y);
}

__DEVICE__ bool writePos(float2 pos, int i, inout float4 *fragColor, float2 fragCoord, float2 iResolution)
{
    if (isPixel(i,0,fragCoord, iResolution)) { (*fragColor).x=pos.x; (*fragColor).y=pos.y; return true; }
    return false;
}

__DEVICE__ float4 getRand(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel1)
{
    return texture(iChannel1,filterUV1(pos/to_float2(400,300), iResolution));
}

__DEVICE__ float dotDist(float2 pos,float2 fragCoord)
{
    return length(pos-fragCoord);
}

// iq: https://iquilezles.org/articles/distfunctions
__DEVICE__ float lineDist(float2 a,float2 b,float2 p)
{
    float2 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h );
}

__DEVICE__ float4 drawDot(float2 pos,float r, float2 fragCoord)
{
    return to_float4_s(clamp(r-length(pos-fragCoord),0.0f,1.0f)/r*3.0f);
}

#define N(x) (swi2(x,y,x)*to_float2(1,-1))

// gives a parametric position on a pentagram with radius 1 within t=0..5
// (maybe there's more elegant ways to do this...)
__DEVICE__ float2 pentaPos(float t)
{
    float w=_sqrtf((5.0f+_sqrtf(5.0f))*0.5f);
    float s=_sqrtf(1.0f-w*w*0.25f);
    float ang=-_floor(t)*PI2*2.0f/5.0f;
    float2 x=to_float2(_cosf(ang),_sinf(ang));
    return -1.0f*N(x)*s+x*w*(fract(t)-0.5f);
}

__KERNEL__ void WritingsFromHellJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_CHECKBOX0(KEY_I, 0);
    
    CONNECT_SLIDER1(LineSize1, -1.0f, 10.0f, 5.0f);
    CONNECT_SLIDER2(LineSize2, -1.0f, 10.0f, 3.9f);
    
    CONNECT_SLIDER3(PNUM, -1.0f, 50.0f, 10.0f);
    
    CONNECT_POINT0(PosOffset, 0.0f, 0.0f );
    CONNECT_POINT1(PosOffset2, 0.0f, 0.0f );
    
    CONNECT_SLIDER4(Swing1, -1.0f, 5.0f, 1.0f);
    CONNECT_SLIDER5(Swing2, -1.0f, 5.0f, 0.5f);
    CONNECT_SLIDER6(Swing3, -1.0f, 5.0f, 0.5f);
    
    //Blending
    CONNECT_SLIDER7(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER8(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER9(BlendMul, -10.0f, 10.0f, 1.0f);

    fragCoord+=0.5f; 

    float time=(float)(iFrame)*1.0f/60.0f;
    float2 uv=fragCoord/iResolution;
    float v=0.0f;
    for(int i=0;i<50;i++) v+=texture(iChannel0,swi2(getRand(to_float2(i,0), iResolution, iChannel1),x,y)).x/50.0f;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    int pnum = (int)(_fminf(iResolution.y/50.0f,(float)(PNUM-1)));
    bool write=false;
    
    float4 tex;
    
    for(int i=0;i<PNUM;i++)
    {
      bool isMouse = (i==pnum);
      // breaking here if i>pnum didnt work in windows (failed to unloll loop)
      if(i<=pnum) {
        float2 pos;
            
        pos=readPos(i, iResolution, iChannel0);
        float2 oldpos=pos;
        
        float ang = (getRand(pos, iResolution, iChannel1)+getRand(pos+to_float2(1,3)*time, iResolution, iChannel1)).x*PI2;
        pos+=to_float2(0.7f,0)+PosOffset2
            +(to_float2(4,5)+PosOffset)*to_float2(Swing1*_cosf(15.0f*time+(float)(i)),
                                 Swing2*_sinf(15.0f*time+(float)(i)+0.5f)+
                                 Swing3*_sinf(21.0f*time+(float)(i)+0.5f))*getRand(pos, iResolution, iChannel1).x;
        //+to_float2(0.2f,2)*to_float2(_cosf(ang),_sinf(ang));
        //vec4 c = drawDot(mod_f(pos,iResolution),2.5f,fragCoord);

        if(isMouse) 
        {
          pos=swi2(iMouse,x,y);
          //if(swi2(iMouse,x,y)==to_float2(0) && mod_f(iTime+5.0f,37.7f)>18.0f)
          if((iMouse.x == 0.0f && iMouse.y == 0.0f) && mod_f(iTime+5.0f,37.7f)>18.0f)
          {
              pos=pentaPos(iTime*0.5f)*0.45f*iResolution.y+iResolution*0.5f;
              pos+=(swi2(getRand(pos*0.6f+iTime*to_float2(0.1f,1.0f), iResolution, iChannel1),x,y)-0.5f)*7.0f/500.0f*iResolution.y;
          }
          if(length(oldpos-pos)>40.0f) oldpos=pos;
        }

              
        float2 mpos=mod_f(pos,iResolution);
        //float dd = dotDist(mpos,fragCoord);
        float dd = lineDist(mpos,oldpos-(pos-mpos),fragCoord);
        //float4 c = to_float4(clamp((isMouse?5.0f:3.9f)-dd,0.0f,1.9f),0.0f,_fmaxf(0.0f,1.0f-dd/40.0f),0);
        float4 c = to_float4(clamp((isMouse?LineSize1:LineSize2)-dd,0.0f,1.9f),0.0f,_fmaxf(0.0f,1.0f-dd/40.0f),0);
        
        //if(mpos==oldpos-(pos-mpos)) c=to_float4_s(0.0f); // ignore 0-length segments
        if(mpos.x==oldpos.x-(pos.x-mpos.x) && mpos.y==oldpos.y-(pos.y-mpos.y)) c=to_float4_s(0.0f); // ignore 0-length segments
        
        
        if(getRand(pos*0.3f+time, iResolution, iChannel1).z>0.8f && !isMouse) 
            pos+=to_float2(10,0);
        else
            fragColor = _fmaxf(fragColor,c);        


        if(writePos(pos, i, &fragColor,fragCoord, iResolution)) write=true;
        }
    }

    if(!write)
    {
       fragColor.z=_fmaxf(-1.0f,fragColor.z-0.002f);
       fragColor.x=_fmaxf(0.0f,fragColor.x-0.003f);
    }

        
    if(iTime<2.0f || KEY_I) 
    {
      fragColor=to_float4(0,0,0.6f,0);
      for(int i=0;i<PNUM;i++)
      {
        if(i<=pnum){
          float4 rnd=texture(iChannel1,to_float2((float)(i)+0.5f,0.5f)/iResolution);
          writePos(to_float2(20.0f+rnd.x*40.0f,iResolution.y/(float)(pnum)*(float)(i+1)),i, &fragColor,fragCoord, iResolution);
        }
      }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Blending' to iChannel4
// Connect Buffer B 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer B 'Texture: RGBA Noise Medium' to iChannel3
// Connect Buffer B 'Previsualization: Buffer A' to iChannel2
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// single pass CFD
// ---------------
// this is some "computational flockarooid dynamics" ;)
// the self-advection is done purely rotational on all scales. 
// therefore i dont need any divergence-free velocity field. 
// with stochastic sampling i get the proper "mean values" of rotations 
// over time for higher order scales.
//
// try changing "RotNum" for different accuracies of rotation calculation
// for even RotNum uncomment the line #define SUPPORT_EVEN_ROTNUM

#define RotNum 5
//#define SUPPORT_EVEN_ROTNUM

#define Res  iResolution
#define Res1 iResolution

//#define keyTex iChannel3
//#define KEY_I texture(keyTex,to_float2((105.5f-32.0f)/256.0f,(0.5f+0.0f)/3.0f)).x



__DEVICE__ float4 randS(float2 uv, float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return texture(iChannel3,uv)-to_float4_s(0.5f);
}

__DEVICE__ float2 getGradBlue(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float eps=1.4f;
    float2 d=to_float2(eps,0);
    return to_float2(
                      texture(iChannel0,fract((pos+swi2(d,x,y))/swi2(Res,x,y))).z
                     -texture(iChannel0,fract((pos-swi2(d,x,y))/swi2(Res,x,y))).z,
                      texture(iChannel0,fract((pos+swi2(d,y,x))/swi2(Res,x,y))).z
                     -texture(iChannel0,fract((pos-swi2(d,y,x))/swi2(Res,x,y))).z
                    )/(eps*2.0f);
}

__DEVICE__ float getRot(float2 pos, float2 b, mat2 m, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float2 p = b;
    float rot=0.0f;
    for(int i=0;i<RotNum;i++)
    {
      float2 v=swi2(texture(iChannel0,fract_f2((pos+p)/swi2(Res,x,y))),x,y);

      rot+=dot(v,swi2(p,y,x)*to_float2(1,-1));
      p = mul_mat2_f2(m,p);
    }
    return rot/(float)(RotNum)/dot(b,b);
}

__DEVICE__ float4 getC2(float2 uv, float2 iResolution, __TEXTURE2D__ iChannel2) 
{
  // line 0 holds writer infos so take 1st line instead
  if(uv.y*iResolution.y<1.0f) uv.y+=1.0f/iResolution.y;
  return _tex2DVecN(iChannel2,uv.x,uv.y,15);
}

__KERNEL__ void WritingsFromHellJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(KEY_I, 0);
    
    //Blending
    CONNECT_SLIDER7(Blend, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER8(BlendOff, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER9(BlendMul, -10.0f, 10.0f, 1.0f);

    fragCoord+=0.5f;

    const float ang = 2.0f*3.1415926535f/(float)(RotNum);
    mat2 m = to_mat2(_cosf(ang),_sinf(ang),-_sinf(ang),_cosf(ang));
    mat2 mh = to_mat2(_cosf(ang*0.5f),_sinf(ang*0.5f),-_sinf(ang*0.5f),_cosf(ang*0.5f));

    float2 uv = fragCoord/iResolution;
    float2 pos = fragCoord;
    float rnd = randS(to_float2((float)(iFrame)/Res.x,0.5f/Res1.y), iResolution, iChannel3).x;
    
    float2 b = to_float2(_cosf(ang*rnd),_sinf(ang*rnd));
    float2 v=to_float2_s(0);
    float bbMax=0.7f*Res.y*1.0f; bbMax*=bbMax;
    for(int l=0;l<8;l++)
    {
        if ( dot(b,b) > bbMax ) break;
        float2 p = b;
        for(int i=0;i<RotNum;i++)
        {
#ifdef SUPPORT_EVEN_ROTNUM
            v+=swi2(p,y,x)*getRot(pos+p,-(mul_mat2_f2(mh,b), m, iResolution, iChannel0);
#else
            // this is faster but works only for odd RotNum
            v+=swi2(p,y,x)*getRot(pos+p,b, m, iResolution, iChannel0);
#endif
            p = mul_mat2_f2(m,p);
        }
        b*=2.0f;
    }
    float4 c2=getC2(fract_f2(uv), iResolution, iChannel2);
    float strength = clamp(1.0f-1.0f*c2.z,0.0f,1.0f);
    fragColor=texture(iChannel0,fract_f2((pos+v*strength*(2.0f/*+2.0f*iMouse.y/Res.y*/)*to_float2(-1,1)*1.0f)/Res));
    fragColor=_mix(fragColor,swi4(c2,x,x,z,w)*to_float4(1,-1,1,1),0.3f*clamp(1.0f-strength,0.0f,1.0f));
    
    // damping
    swi2S(fragColor,x,y, _mix(swi2(fragColor,x,y),to_float2_s(0.0f),0.02f));
    
    // add a little "motor" in the center
    //vec2 scr=(fragCoord/swi2(Res,x,y))*2.0f-to_float2_s(1.0f);
    //swi2(fragColor,x,y) += (0.01f*swi2(scr,x,y) / (dot(scr,scr)/0.1f+0.3f));

    //Blending  
    if(Blend>0.0f)
    {
       float4 tex = texture(iChannel4, fragCoord/iResolution);
       if ( tex.w > 0.0f)
         fragColor = _mix(fragColor, tex, Blend);
    }


    
    if(iFrame<=4 || KEY_I) fragColor=texture(iChannel2,fragCoord/Res);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Texture: Rusty Metal' to iChannel4       
// Connect Buffer C 'Texture: RGBA Noise Medium' to iChannel3 
// Connect Buffer C 'Texture: RGBA Noise Small' to iChannel2 
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

__DEVICE__ float getVign(float2 fragCoord, float2 iResolution)
{
  float vign=1.0f;
    
  float rs=length(fragCoord-iResolution*0.5f)/iResolution.x/0.7f;  
  vign*=1.0f-rs*rs*rs;
    
  float2 co=2.0f*(fragCoord-0.5f*iResolution)/iResolution;
  vign*=_cosf(0.75f*length(co));
  vign*=0.5f+0.5f*(1.0f-_powf(co.x*co.x,16.0f))*(1.0f-_powf(co.y*co.y,16.0f));
    
  return vign;
}

__KERNEL__ void WritingsFromHellJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    CONNECT_COLOR1(Color1, 1.0f, 0.2f, 0.0f, 1.0f);
    CONNECT_COLOR2(Color2, 0.4f, 0.4f, 0.3f, 1.0f);
    CONNECT_COLOR3(Color3, 1.0f, 1.0f, 0.5f, 1.0f);
    CONNECT_COLOR4(ColorBKG, 0.2f, 0.12f, 0.06f, 0.0f);
    CONNECT_SLIDER0(Flicker, -1.0f, 1.0f, 0.25f);

    //CONNECT_CHECKBOX1(Rattle, 1);

    fragCoord+=0.5f;

    // camera rattle
    float4 rattle=texture(iChannel3,to_float2(iTime*0.1234f*0.5f,0.5f/256.0f));
    
    float2 uv = fract_f2(((fragCoord / iResolution-0.5f)*(1.0f+rattle.z*0.01f)+0.5f) + swi2(rattle,x,y)*0.005f);
    
    float4 c = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float4 old = texture(iChannel1,fragCoord / iResolution);
    // brightness flickering
    float4 flicker=texture(iChannel3,to_float2(iTime*0.2f,0.5f/256.0f));
    
    // yellow-red fade
    fragColor= 1.5f*_mix(abs_f4(swi4(c,x,x,w,w)*Color1),0.6f*abs_f4(swi4(c,x,x,x,w)),(1.0f-smoothstep(0.35f,0.45f,c.z))*(1.0f-smoothstep(0.25f,0.35f,c.x)));
    
    fragColor+=  abs_f4(swi4(c,y,y,y,w))*Color2                                     // bright core
                +(0.8f+0.2f*flicker)*Color3*clamp(swi4(c,z,z,z,w)-0.5f,0.0f,1.0f);  // halo
    
    // mix bg image
    fragColor=_mix(to_float4_s(1),ColorBKG*1.2f+0.4f*_tex2DVecN(iChannel4,uv.x,uv.y,15).x*to_float4(1,1,1,0),-fragColor+1.1f);

    fragColor*=(flicker*Flicker+0.75f)*2.3f*fragColor;          // fragColor^2 contrast
    fragColor*=getVign(fragCoord, iResolution);               // vignetting
    fragColor=_mix(fragColor,old*to_float4(0.7f,1,1,1),0.6f); // slight motion blur (camera latency)

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0


// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// just a dummy shader because i needed some feedback of own color (not possible in image tab)
__KERNEL__ void WritingsFromHellJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
  fragColor = texture(iChannel0,fragCoord/iResolution) + Color-0.5f;
  fragColor.w = Color.w;

  SetFragmentShaderComputedColor(fragColor);
}
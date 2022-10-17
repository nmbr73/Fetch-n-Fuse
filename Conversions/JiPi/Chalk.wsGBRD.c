
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel1
// Connect Buffer A 'Texture: Video' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// created by florian berger (flockaroo) - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// trying to resemle some chalk on blackboard hand drawing style

// this buffer holds the prerendered the gradient

#define Res0 iResolution //to_float2(textureSize(iChannel0,0).xy)
#define Res1 iResolution //to_float2(textureSize(iChannel1,0).xy)

#define Res  iResolution




__DEVICE__ float4 getRand(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel1)
{
    return texture(iChannel1,pos/Res1/iResolution.y*1200.0f);
}

#define UVScale (to_float2(Res0.y/Res0.x,1)/Res.y)

__DEVICE__ float4 getCol(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float2 uv=pos/swi2(Res0,x,y);
    uv=clamp(uv,0.5f/Res0,1.0f-0.5f/Res0);
    float4 c1=_tex2DVecN(iChannel0,uv.x,uv.y,15);
    float d=clamp(dot(swi3(c1,x,y,z),to_float3(-0.5f,1.0f,-0.5f)),0.0f,1.0f);
    float4 c2=to_float4_s(0.5f);
    return _fminf(_mix(c1,c2,1.8f*d),to_float4_s(0.7f));
}

__DEVICE__ float4 getColHT(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1)
{
   return smoothstep(to_float4_s(0.95f),to_float4_s(1.05f),getCol(pos,iResolution,iChannel0)+getRand(pos*0.2f, iResolution, iChannel1));
}

__DEVICE__ float getVal(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
   float4 c=getCol(pos, iResolution, iChannel0);
   return dot(swi3(c,x,y,z),to_float3_s(0.333f));
}

#define SQR3 1.73205081f
__DEVICE__ float2 getGrad(float2 pos, float eps, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float3 d=to_float3_aw(eps/UVScale/Res0,0);
    pos-=0.33f*swi2(d,x,y);
    float v0=getVal(pos, iResolution, iChannel0);
    return (to_float2(getVal(pos+swi2(d,x,z), iResolution, iChannel0),getVal(pos+swi2(d,z,y), iResolution, iChannel0))-v0)/swi2(d,x,y);
}

__KERNEL__ void ChalkFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;
    float2 ResN = Res0;
    ResN *= _fminf(ResN.y,Res.y)/ResN.y;
    ResN *= _fminf(ResN.x,Res.x)/ResN.x;
    float2 fc=fragCoord*Res0/ResN;
    swi2S(fragColor,x,y, getGrad(fc,0.15f, iResolution, iChannel0));
    fragColor.z=getVal(fc, iResolution, iChannel0);
    if(fragCoord.x<1.0f && fragCoord.y<1.0f)
    swi2S(fragColor,z,w, ResN);
    if(fragCoord.x>ResN.x || fragCoord.y>ResN.y) 
    {
        fragColor=to_float4(0,0,0,1);
        //discard;
        //SetFragmentShaderComputedColor(fragColor);
        //return;
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: RGBA Noise Medium' to iChannel1
// Connect Buffer B 'Texture: Video' to iChannel0

// created by florian berger (flockaroo) - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// trying to resemle some chalk on blackboard hand drawing style


__DEVICE__ mat2 ROTM(float ang) { return to_mat2(_cosf(ang),_sinf(ang),-_sinf(ang),_cosf(ang)); }

__DEVICE__ float2 quad01(int idx) 
{ 

  return idx<3?make_float2(idx%2,idx/2):1.0f-make_float2((5-idx)%2,(5-idx)/2); 
}

__DEVICE__ float4 getRandB(int idx, float2 iResolution, __TEXTURE2D__ iChannel1) {
    int2 res=to_int2_cfloat(iResolution);//textureSize(iChannel1,0);
    //return texelFetch(iChannel1,to_int2(idx%res.x,(idx/res.x)%res.y),0); 
    return texture(iChannel1,(make_float2(to_int2(idx%res.x,(idx/res.x)%res.y))+0.5f)/iResolution); 
}

__KERNEL__ void ChalkFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;
    
    float StrokeLen=1.2f;
    float StrokeW=0.4f;
    int NumStrokes=3;
    int NumBatches=40;
    
    float3 c=to_float3_s(0);
    for(int i=0;i<NumStrokes*NumBatches;i++){
      float2 sc=fragCoord/iResolution*2.0f-1.0f;
      int strokeIdx=i;
      int strokeIdx0=strokeIdx;
      strokeIdx=strokeIdx%NumStrokes;
      int batchIdx=strokeIdx0/NumStrokes;
      float ang=(float)(batchIdx)*0.17f+_floor(iTime*3.0f);
      float dang=((float)(strokeIdx%2)-0.5f)*StrokeW/StrokeLen*1.2f;
      mat2 m=ROTM(dang);
      float2 sc0=(swi2(getRandB(batchIdx,iResolution, iChannel1),z,w)-0.5f)*2.0f;
      sc-=sc0;
      sc=mul_mat2_f2(ROTM(ang) , sc);
      //sc0=to_float2(0);
      float strokeFact=(float)(strokeIdx)/(float)(NumStrokes)-0.5f;
      //float segFact=sc.x/(StrokeLen*0.5f);
      sc+=StrokeW*0.8f*(float)(NumStrokes)*to_float2(0,0.7f*strokeFact);
      sc=mul_mat2_f2(ROTM(dang) , sc);
      float2 uv=sc/(to_float2(StrokeLen,StrokeW));
      //uv=mul_mat2_f2(m , uv);
      uv.y+=uv.x*uv.x*1.5f;
      uv+=0.5f;
      //float4 r=textureLod(iChannel1,(uv+to_float2(0,i))*to_float2(0.02f,1.0f),1.7f);
      float4 r=texture(iChannel1,(uv+to_float2(0,i))*to_float2(0.02f,1.0f));
      float3 s;
      s = clamp(to_float3_s(0) + r.x,0.0f,1.0f);
      s*=_mix(_expf(-12.0f*uv.x)+_expf(-12.0f*(1.0f-uv.x)),1.0f,0.5f);
      float a=1.0f+r.x;
      a*=1.0f-smoothstep(0.85f,1.0f,_fabs(uv.x-0.5f)*2.0f);
      a*=1.0f-smoothstep(0.85f,1.0f,_fabs(uv.y-0.5f)*2.0f);
      a*=0.3f;
      a=clamp(a,0.0f,1.0f);
      c=c*(1.0f-a)+s*a;
    }
    
    swi3S(fragColor,x,y,z, c);
    fragColor.w=1.0f;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel2

// Connect Image 'Texture: RGBA Noise Medium' to iChannel3
// Connect Image 'Texture: Video' to iChannel1

// created by florian berger (flockaroo) - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// trying to resemle some chalk on blackboard hand drawing style


#define PI2 6.28318530717959f
#define N(a) (swi2((a),y,x)*to_float2(1,-1))
#define AngleNum 5
#define SampNum 24

//#define Res0 to_float2(textureSize(iChannel0,0).xy)
//#define Res1 to_float2(textureSize(iChannel1,0).xy)
//#define Res2 to_float2(textureSize(iChannel2,0).xy)

//#define Res  iResolution



#define zoom 1.0f


__DEVICE__ float4  getRandI(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel3)
{
    return texture(iChannel3,pos/Res1/iResolution.y*1200.0f);
}

#define UVScale (to_float2(Res0.y/Res0.x,1)/Res.y)



#define OutlineOffs 0.0f

__DEVICE__ float4 getColI(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0, float bright)
{
    pos=(pos-0.5f*swi2(Res,x,y))*zoom+0.5f*swi2(Res,x,y);
    float2 r0=swi2(texture(iChannel0,0.5f/Res),z,w);
    float2 sc=_mix(r0.y/Res0.y,r0.x/Res0.x,0.5f)/Res;    // compromise between "fit all" and "fit one"
    float2 uv = pos*sc+0.5f*(r0/Res0-Res*sc);
    uv=clamp(uv,0.5f/Res0,1.0f-0.5f/Res0);
    return (to_float4_s(1.0f)-bright*swi4(_tex2DVecN(iChannel0,uv.x,uv.y,15),z,z,z,w));
}

__DEVICE__ float2 getGradI(float2 pos, float eps, float2 iResolution, __TEXTURE2D__ iChannel0, float contourStrength)
{
    pos=(pos-0.5f*swi2(Res,x,y))*zoom+0.5f*swi2(Res,x,y);
    float2 r0=swi2(texture(iChannel0,0.5f/Res),z,w);
    float2 sc=_mix(r0.y/Res0.y,r0.x/Res0.x,0.5f)/Res;    // compromise between "fit all" and "fit one"
    float2 uv = pos*sc+0.5f*(r0/Res0-Res*sc);
    uv=clamp(uv,0.5f/Res0,1.0f-0.5f/Res0);
    return (contourStrength)*swi2(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y)*r0/swi2(Res,x,y);
}


__DEVICE__ float htPattern(float2 pos, float phase, float2 iResolution, float iTime, __TEXTURE2D__ iChannel3, float flicker, float flickerFreq)
{
  
    //const float flicker=1.0f;
    //const float flickerFreq=10.0f;
  
    float pat=0.0f;
    float cnt=0.0f;
    float2 offs=to_float2(0.001f,0.1f)*_floor(iTime*flickerFreq)/10.0f*flicker;
    float phaseOffs = 10.0f*getRandI(_floor(iTime*flickerFreq)*to_float2(0.01f,0.1f), iResolution, iChannel3).x*flicker;
    float2 gr=/*getGrad(_floor(pos/13.0f)*13.0f,1.0f)+*/1.01f*normalize(pos-0.5f*Res);
    for(float ang=0.0f;ang<PI2;ang+=PI2/4.3f)
    {
        float2 b=normalize(sin_f2(to_float2(0,PI2/4.0f) + ang + phase + phaseOffs + 0.6f )*to_float2(0.5f,1.5f));
        float2 uv=((pos.x-pos.y*pos.y*0.0004f)*b+(pos.y+pos.x*pos.x*0.0004f)*N(b))/Res1*to_float2(7,0.3f)*0.3f;
        pat+=0.5f*texture(iChannel3,uv*0.25f+offs).x;
        pat+=1.0f*texture(iChannel3,uv+offs).x;
        cnt+=1.5f;
    }
    return pat/cnt; 
}

__DEVICE__ float halfTone(float val,float2 pos, float phase, float2 iResolution, float iTime, __TEXTURE2D__ iChannel3, float flicker, float flickerFreq)
{
    return smoothstep(0.6f,1.4f,val+htPattern(pos,phase,iResolution,iTime,iChannel3, flicker, flickerFreq));
}



__KERNEL__ void ChalkFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_COLOR0(Color1, 0.0f, 0.0f, 0.0f, 1.0f);
    CONNECT_COLOR1(paperTint, 0.8f, 0.68f, 0.72f, 1.0f);
    CONNECT_SLIDER0(flicker, -1.0f, 5.0f, 1.0f);
    CONNECT_SLIDER1(flickerFreq, -1.0f, 50.0f, 10.0f);
    
    CONNECT_SLIDER2(Fact, -1.0f, 10.0f, 0.0f);
    
    CONNECT_CHECKBOX0(ChalkOnly, 0);
    CONNECT_CHECKBOX1(Col2, 0);

    CONNECT_SLIDER3(BlackFill, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(bright, -1.0f, 10.0f, 1.1f);
    CONNECT_SLIDER5(contourStrength, -1.0f, 10.0f, 1.0f);
    CONNECT_SLIDER6(reflection, -1.0f, 3.0f, 0.5f);
    CONNECT_SLIDER7(reflectStrength, -1.0f, 3.0f, 0.3f);
    CONNECT_SLIDER8(reflectSize, -1.0f, 3.0f, 0.35f);

    fragCoord+=0.5f; 
    
    
//    const float BlackFill=1.0f;
//    const float bright=1.1f;
//    const float contourStrength=1.0f;
//    const float reflection=0.5f;
//    const float reflectStrength=0.3f;
//    const float reflectSize=0.35f;
    #define reflectPos (Res*(0.2f+0.15f*sin_f2(iTime+to_float2(0,2.0f))))
//    const float3 paperTint={1*0.8f,0.85f*0.8f,0.9f*0.8f};

    const float BGAlpha=0.0f;

    float2 pos = fragCoord+0.0f*sin_f2(iTime*1.0f*to_float2(1,1.7f))*iResolution.y/400.0f;
    float2 pos0=pos;
    float3 col = to_float3_s(0);
    float3 col2 = to_float3_s(0);
    float sum=0.0f;
    float2 g0=getGradI(pos,1.0f, iResolution, iChannel0, contourStrength);
    float dang=PI2/(float)(AngleNum);
    for(int i=0;i<AngleNum;i++)
    {
        float ang=dang*(float)(i)+0.1f;
        float2 v=sin_f2(to_float2(PI2/4.0f,0)+ang);
        for(int j=0;j<SampNum;j++)
        {
            float2 dpos  = swi2(v,y,x)*(to_float2(1,-1)*(float)(j)*iResolution.y/25.0f/(float)(SampNum)+OutlineOffs*Res.x/15.0f);
            float2 dpos2 = swi2(v,x,y)*((float)(j*j)/(float)(SampNum*SampNum)*0.3f
                               //*(length(100.0f*g0)) // higher/lower gradients get curved/hatched
                               +0.08f)*iResolution.y/25.0f;
            float2 g;
            float fact=1.0f+Fact;
            float fact2;

            for(float s=-1.0f;s<=1.0f;s+=2.0f)
            {
                float2 pos2=pos+1.0f/zoom*(s*dpos+dpos2);
                float2 pos3=pos+1.0f/zoom*swi2((s*dpos+dpos2),y,x)*to_float2(1,-1)*2.0f;
                float ht=1.0f;
                g=getGradI(pos2,1.0f, iResolution, iChannel0, contourStrength)*ht;
                g*=_powf(getRandI(pos2*0.8f*iResolution.y/1080.0f, iResolution, iChannel3).x*2.0f,4.0f*_sqrtf(iResolution.y/1200.0f));
                
                float fact3=dot(g,v)-0.5f*_fabs(dot(g,swi2(v,y,x)*to_float2(1,-1)))/**(1.0f-getVal(pos2))*/;
                fact2=dot(normalize(g+to_float2_s(0.0001f)),swi2(v,y,x)*to_float2(1,-1));
             
                fact3=clamp(fact3,0.0f,0.05f);
                fact2=_fabs(fact2);
                
                fact3*=1.0f-1.0f*(float)(j)/(float)(SampNum);
                fact*=fact3;
                col += 0.3f*fact3;
                sum+=fact2;
            }
            col += 2.0f*_powf(fact,0.5f);
        }
    }
    col2 = col; // debug
    col/=(float)(SampNum*AngleNum)*0.75f/_sqrtf(iResolution.y);
    //col2/=sum;

    col=1.0f-col*1.2f;
    col*=col*col;

    float3 coldeb=col;   // ChalkOnly  

    float2 s=sin_f2(swi2(pos,x,y)*0.1f/_sqrtf(iResolution.y/400.0f));
    float r=length(pos-iResolution*0.5f)/iResolution.x;
    float vign=1.0f-r*r*r;
    float3 c=swi3(getColI(pos, iResolution, iChannel0, bright),x,y,z);
    float _bright=dot(swi3(getColI(pos, iResolution, iChannel0, bright),x,y,z),to_float3_s(0.3333f));
    float blackTone=halfTone(_bright*1.5f+0.25f,(pos0-Res*0.5f)*zoom,_floor(_sqrtf(_bright)*8.0f)/8.0f*2.7f, iResolution, iTime, iChannel3, flicker, flickerFreq);

    blackTone = _mix(1.0f,     blackTone,BlackFill);
    float refl=clamp(_powf((col.x*(blackTone)),1.0f),0.0f,1.0f);
    float3 col3= swi3(paperTint,x,y,z);
    col3*=to_float3_s(col.x)*blackTone;
    col3+=0.1f*swi3(getRandI(pos*0.7f, iResolution, iChannel3),x,x,x);
    col3*=(1.0f-0.65f*swi3(texture(iChannel2,fragCoord/iResolution),x,y,z));
    
  fragColor = to_float4_aw(col3*0.9f+0.2f*swi3(getRandI(pos*0.7f, iResolution, iChannel3),x,y,z)-0.2f*swi3(getRandI(pos*0.7f-0.6f, iResolution, iChannel3),x,y,z),1);
  float reflEnv=clamp((_sinf(fragCoord.x/iResolution.x*7.0f+2.5f+1.7f*iTime))*(fragCoord.y/iResolution.y),0.0f,1.0f);
  float2 reflp=reflectPos*to_float2(1,-1)+to_float2(0,iResolution.y); 
  if(iMouse.x>0.5f) reflp=swi2(iMouse,x,y);
  reflEnv = _expf(-_powf(length(reflp-fragCoord)/iResolution.x/reflectSize,2.0f));

  swi3S(fragColor,x,y,z, 1.0f-swi3(fragColor,x,y,z));
  swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + refl*to_float3(0.8f,0.9f,1.0f)*1.1f*reflEnv*reflectStrength);
  fragColor.w=_mix(1.0f,1.0f-_fminf(min(fragColor.x,fragColor.y),fragColor.z),BGAlpha);

  if(ChalkOnly)
    fragColor = to_float4_aw(coldeb,1.0f);
  if(Col2)
    fragColor = to_float4_aw(col2,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
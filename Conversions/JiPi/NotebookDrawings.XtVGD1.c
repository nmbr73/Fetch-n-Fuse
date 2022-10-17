
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel1
// Connect Image '/media/a/35c87bcb8d7af24c54d41122dadb619dd920646a0bd0e477e7bdc6d12876df17.webm' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// trying to resemle some hand drawing style


#define SHADERTOY
#ifdef SHADERTOY
#define Res0 iResolution
#define Res1 iResolution
#else
#define Res0 textureSize(iChannel0,0)
#define Res1 textureSize(iChannel1,0)
#define iResolution Res0
#endif

#define Res  iResolution

#define randSamp iChannel1
#define colorSamp iChannel0


__DEVICE__ float4 getRand(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel1)
{
  return texture(iChannel1,pos/Res1/iResolution.y*1080.0f);
}

__DEVICE__ float4 getCol(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  // take aspect ratio into account
  float2 uv=((pos-swi2(Res,x,y)*0.5f)/Res.y*Res0.y)/swi2(Res0,x,y)+0.5f;
  float4 c1=_tex2DVecN(iChannel0,uv.x,uv.y,15);
  float4 e=smoothstep(to_float4_s(-0.05f),to_float4_s(-0.0f),to_float4_f2f2(uv,to_float2_s(1)-uv));
  c1=_mix(to_float4(1,1,1,0),c1,e.x*e.y*e.z*e.w);
  float d=clamp(dot(swi3(c1,x,y,z),to_float3(-0.5f,1.0f,-0.5f)),0.0f,1.0f);
  float4 c2=to_float4_s(0.7f);
  return _fminf(_mix(c1,c2,1.8f*d),to_float4_s(0.7f));
}

__DEVICE__ float4 getColHT(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1)
{
  return smoothstep(to_float4_s(0.95f),to_float4_s(1.05f),getCol(pos, iResolution,iChannel0)*0.8f+0.2f+getRand(pos*0.7f,iResolution,iChannel1));
}

__DEVICE__ float getVal(float2 pos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
  float4 c=getCol(pos, iResolution, iChannel0);
  return _powf(dot(swi3(c,x,y,z),to_float3_s(0.333f)),1.0f)*1.0f;
}

__DEVICE__ float2 getGrad(float2 pos, float eps, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float2 d=to_float2(eps,0);
    return to_float2(
        getVal(pos+swi2(d,x,y), iResolution, iChannel0)-getVal(pos-swi2(d,x,y), iResolution, iChannel0),
        getVal(pos+swi2(d,y,x), iResolution, iChannel0)-getVal(pos-swi2(d,y,x), iResolution, iChannel0)
    )/eps/2.0f;
}

#define AngleNum 3

#define SampNum 16
#define PI2 6.28318530717959f

__KERNEL__ void NotebookDrawingsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float2 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1)
{

    float2 pos = fragCoord+4.0f*sin_f2(iTime*1.0f*to_float2(1,1.7f))*iResolution.y/400.0f;
    float3 col = to_float3_s(0);
    float3 col2 = to_float3_s(0);
    float sum=0.0f;
    for(int i=0;i<AngleNum;i++)
    {
        float ang=PI2/(float)(AngleNum)*((float)(i)+0.8f);
        float2 v=to_float2(_cosf(ang),_sinf(ang));
        for(int j=0;j<SampNum;j++)
        {
            float2 dpos  = swi2(v,y,x)*to_float2(1,-1)*(float)(j)*iResolution.y/400.0f;
            float2 dpos2 = swi2(v,x,y)*(float)(j*j)/(float)(SampNum)*0.5f*iResolution.y/400.0f;
            float2 g;
            float fact;
            float fact2;

            for(float s=-1.0f;s<=1.0f;s+=2.0f)
            {
              float2 pos2=pos+s*dpos+dpos2;
              float2 pos3=pos+swi2((s*dpos+dpos2),y,x)*to_float2(1,-1)*2.0f;
              g=getGrad(pos2,0.4f, iResolution, iChannel0);
              fact=dot(g,v)-0.5f*_fabs(dot(g,swi2(v,y,x)*to_float2(1,-1)))/**(1.0f-getVal(pos2, iResolution, iChannel0))*/;
              fact2=dot(normalize(g+to_float2_s(0.0001f)),swi2(v,y,x)*to_float2(1,-1));
                
              fact=clamp(fact,0.0f,0.05f);
              fact2=_fabs(fact2);
                
              fact*=1.0f-(float)(j)/(float)(SampNum);
              col += fact;
              col2 += fact2*swi3(getColHT(pos3, iResolution, iChannel0,iChannel1),x,y,z);
              sum+=fact2;
            }
        }
    }
    col/=(float)(SampNum*AngleNum)*0.75f/_sqrtf(iResolution.y);
    col2/=sum;
    col.x*=(0.6f+0.8f*getRand(pos*0.7f, iResolution, iChannel1).x);
    col.x=1.0f-col.x;
    col.x*=col.x*col.x;

    float2 s=sin_f2(swi2(pos,x,y)*0.1f/_sqrtf(iResolution.y/400.0f));
    float3 karo=to_float3_s(1);
    karo-=0.5f*to_float3(0.25f,0.1f,0.1f)*dot(exp_f2(-s*s*80.0f),to_float2_s(1));
    float r=length(pos-iResolution*0.5f)/iResolution.x;
    float vign=1.0f-r*r*r;
    fragColor = to_float4_aw((col.x*col2*karo*vign),1);
    //fragColor=getCol(fragCoord, iResolution, iChannel0);

  SetFragmentShaderComputedColor(fragColor);
}
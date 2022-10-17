
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel1
// Connect Image '/media/a/35c87bcb8d7af24c54d41122dadb619dd920646a0bd0e477e7bdc6d12876df17.webm' to iChannel0
// Connect Image 'Texture: Pebbles' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// trying to resemble van gogh drawing style

#define Res  iResolution
#define Res0 iResolution //iChannelResolution[0].xy
#define Res1 iResolution //iChannelResolution[1].xy
#define Res2 iResolution //iChannelResolution[2].xy

__DEVICE__ float4 getCol(float2 pos, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2)
{
    float2 uv=pos/Res0;
    
    float4 c1 = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    uv = uv*to_float2(-1,-1)*0.39f+0.015f*to_float2(_sinf(iTime*1.1f),_sinf(iTime*0.271f));
    // had to use .xxxw because tex on channel2 seems to be a GL_RED-only tex now (was probably GL_LUMINANCE-only before)
    float4 c2 = to_float4(0.5f,0.7f,1.0f,1.0f)*1.0f*swi4(_tex2DVecN(iChannel2,uv.x,uv.y,15),x,x,x,w);
    float d=clamp(dot(swi3(c1,x,y,z),to_float3(-0.5f,1.0f,-0.5f)),0.0f,1.0f);
    return _mix(c1,c2,1.8f*d);
}

__DEVICE__ float getVal(float2 pos, float level, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2)
{
    return length(swi3(getCol(pos, iTime, iResolution, iChannel0, iChannel2),x,y,z))+0.0001f*length(pos-0.5f*Res0);
}
    
__DEVICE__ float2 getGrad(float2 pos,float delta, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel2)
{
    float l = 1.0f*_log2f(delta);
    float2 d=to_float2(delta,0);
    return to_float2(
                        getVal(pos+swi2(d,x,y),l, iTime, iResolution, iChannel0, iChannel2)-getVal(pos-swi2(d,x,y),l, iTime, iResolution, iChannel0, iChannel2),
                        getVal(pos+swi2(d,y,x),l, iTime, iResolution, iChannel0, iChannel2)-getVal(pos-swi2(d,y,x),l, iTime, iResolution, iChannel0, iChannel2)
                    )/delta;
}

__DEVICE__ float4 getRand(float2 pos, int iFrame, float2 iResolution, __TEXTURE2D__ iChannel1) 
{
    float2 uv=pos/Res1;
    uv+=1.0f*(float)(iFrame)*to_float2(0.2f,0.1f)/Res1;
    
    return _tex2DVecN(iChannel1,uv.x,uv.y,15);
}

__DEVICE__ float4 getColDist(float2 pos, float iTime, int iFrame, float2 iResolution, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2)
{
  return _floor(0.8f*getCol(pos, iTime, iResolution, iChannel0, iChannel2)+1.1f*getRand(1.2f*pos, iFrame, iResolution, iChannel1));
    float fact = clamp(length(getGrad(pos,5.0f, iTime, iResolution, iChannel0, iChannel2))*20.0f,0.0f,1.0f);
  return _floor(0.8f*getCol(pos, iTime, iResolution, iChannel0, iChannel2)+1.1f*_mix(getRand(0.7f*pos, iFrame, iResolution, iChannel1),getRand(1.7f*pos, iFrame, iResolution, iChannel1),fact));
}

#define SampNum 16

__KERNEL__ void JeanclaudvangoghJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

  float2 pos = fragCoord/Res*Res0;
  float2 uv = fragCoord / iResolution;
  float3 col=to_float3_s(0);
  float cnt=0.0f;
  float fact=1.0f;
  for(int i=0;i<1*SampNum;i++)
  {
      col+=fact* swi3(getColDist(pos, iTime, iFrame, iResolution, iChannel0, iChannel1, iChannel2),x,y,z);
      float2 gr=getGrad(pos,4.0f, iTime, iResolution, iChannel0, iChannel2);
      pos+=0.6f*normalize(_mix(swi2(gr,y,x)*to_float2(1,-1),-gr,0.2f));
      fact*=0.87f;
      cnt+=fact;
  }
  col/=cnt;
  fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
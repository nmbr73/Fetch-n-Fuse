
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Particles idea taken from https://www.shadertoy.com/view/ll3SWs

#define A 6

__DEVICE__ float hash1(float i)
{
   return fract(_sinf(i*0.156854f) * 43758.5453f);
}

__DEVICE__ float hash2(float2 p)
{
   return fract(_sinf(dot(p, to_float2(12.9898f, 78.233f))) * 43758.5453f);
}

__DEVICE__ float2 randCoord(float i) {
    return to_float2(hash1(i),hash1(i+12.54682f));
}

__DEVICE__ float arrivingParticle(float2 coord, out float4 *partData, float2 iResolution, __TEXTURE2D__ iChannel0) {
  *partData = to_float4_s(0);
    float c=0.0f;
    for (int i=-A; i<A; i++) {
        for (int j=-A; j<A; j++) {
            float2 arrCoord = coord + to_float2(i,j);
            float4 data = texture(iChannel0, arrCoord/iResolution);
            if (dot(data,data)<0.1f) continue;
            float2 nextCoord = swi2(data,x,y) + swi2(data,z,w);
            float2 offset = abs_f2(coord - nextCoord);
            // somehow I got this fluid-like effect changing the 
            // "greedly pick one particle" algorithm 
            // for an average of arriving particles and 
            // changing the condition below 
            if (length(offset)<1.7f) { 
                *partData += data;
            c++;
            }
        }
    }
    *partData/=c;
    return c;
}

__KERNEL__ void PaintExplotionsJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;
float AAAAAAAAAAAAAAAAAAA;
    float2 uv = fragCoord/iResolution;
    float t=iTime*3.0f;
    float2 co = uv-randCoord(_floor(t))*to_float2(1.0f,0.6f);
    if (fract(t)<0.3f && length(co)<0.05f) {
        fragColor = to_float4_f2f2(fragCoord, 4.0f*normalize(co)*(1.0f-hash2(uv)*0.5f)+to_float2(0.0f,3.0f));
        
        SetFragmentShaderComputedColor(fragColor);  
        return;
    }
    if (fragCoord.y<30.0f+_sinf(uv.x*5.0f+t)*10.0f) {
        fragColor = to_float4(fragCoord.x,fragCoord.y,0,0);
        
        SetFragmentShaderComputedColor(fragColor);  
        return;
    }
    float4 partData;
    float p = arrivingParticle(fragCoord, &partData, iResolution, iChannel0);
    if (p<1.0f) {
      fragColor = to_float4_s(0.0f);
        
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    swi2S(partData,x,y, swi2(partData,x,y) + swi2(partData,z,w));
    swi2S(partData,z,w, swi2(partData,z,w) * 0.99f);
    swi2S(partData,z,w, swi2(partData,z,w) - to_float2(0.0f,0.05f));
    if (partData.y<30.0f) partData.w*=-1.0f;
    partData.w=_fmaxf(-4.0f,partData.w);
    fragColor = partData;
float tttttttttttttttttttt;
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


#define samples 30.0f
#define glow_size 0.07f 
#define glow_brightness 4.0f

__DEVICE__ float hashI(float2 p)
{
   return fract(_sinf(dot(p, to_float2(12.9898f, 78.233f))) * 43758.5453f);
}

__KERNEL__ void PaintExplotionsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;
float IIIIIIIIIIIIIIIIIIIIII;
    float c=0.0f;
    float2 uv=fragCoord/iResolution;
    float mt=mod_f(iTime,5.0f);
     for (float i=0.0f; i<samples; i++) {
        float t=i*0.354185f+mt;
        float a=hashI(uv+t)*6.28f;
        float l=hashI(uv+t+13.3548f)*glow_size;
        float2 smp = to_float2(_cosf(a),_sinf(a))*l;
        c+=step(0.1f,texture(iChannel0, uv+smp).x)*(glow_size-l*0.9f)/glow_size; 
     }
    float4 part = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float3 uvcol=to_float3_aw(normalize(abs_f2(uv+0.1f)),1.0f);
    float3 col=(c/samples)*uvcol*glow_brightness+step(0.1f,part.x)*uvcol;
    col*=to_float3(0.8f,0.6f,0.15f);
    col*=1.0f-_fabs(uv.x-0.5f)*2.0f;
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
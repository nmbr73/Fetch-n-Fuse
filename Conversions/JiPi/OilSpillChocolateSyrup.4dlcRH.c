
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Small' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution


const float brushSize = 25.0f;

__DEVICE__ float diff(float2 t, float2 b, __TEXTURE2D__ iChannel0){
   float3 t1 = swi3(_tex2DVecN(iChannel0,t.x,t.y,15),x,y,z);
   float3 t2 = swi3(_tex2DVecN(iChannel0,b.x,b.y,15),x,y,z);
   return dot(t1, to_float3_s(1.0f)) * dot(t2, to_float3_s(1.0f));
}

//__DEVICE__ bool reset() {
//    return texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
//}

__KERNEL__ void OilSpillChocolateSyrupFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;

    float2 res = iResolution;
    float2 uv = fragCoord / res;
    
    float2 offs = (0.4f / res);
        
    float2 top = uv + to_float2(0.0f,-offs.y);
    float2 bottom = uv + to_float2(0.0f,offs.y);
    float2 left = uv + to_float2(-offs.x, 0.0f);
    float2 right = uv + to_float2(offs.x, 0.0f);
    
    float gradient1 = diff(top, bottom, iChannel0);
    float gradient2 = diff(left, right, iChannel0);
    
    //float4 fc = to_float4_s( ((length(gradient1) + length(gradient2))*0.07f));
    float4 fc = to_float4_s( ((_fabs(gradient1) + _fabs(gradient2))*0.07f));
    
    if(iFrame < 15 || Reset ){
        fragColor = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    } else {
        float4 old = _tex2DVecN(iChannel0,uv.x,uv.y,15);
        //fc.w *= 0.38f;
        swi4S(fc,x,y,z,w, (swi4(fc,x,y,z,w) * fc.w) + (old*(1.0f - fc.w)));
        //fc += 0.05f;
      fragColor = clamp(fc, to_float4_s(0.0f), to_float4_s(1.0f));
        
    }
    
    if(iMouse.z >= 0.0f){        
        fragColor += to_float4_s(1.0f - clamp(length(swi2(iMouse,x,y) - fragCoord) - brushSize, 0.0f,1.0f));
    }
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


//blur 1
__KERNEL__ void OilSpillChocolateSyrupFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;
float BBBBBBBBBBBBBBBBBBBBBBBBBBB;
    float2 res = iResolution;
    float2 uv = fragCoord / res;
    float2 step = (1.750f / res);
    float4 u = texture(iChannel0, uv + to_float2(0.0f,-step.y));
    float4 d = texture(iChannel0, uv + to_float2(0.0f, step.y));
    float4 l = texture(iChannel0, uv + to_float2(-step.x,0.0f));
    float4 r = texture(iChannel0, uv + to_float2(step.x,0.0f));
    float4 c = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    float4 o = (u+d+l+r+c)*0.2f;
    fragColor = o;//
    fragColor.w = _tex2DVecN(iChannel0,uv.x,uv.y,15).w;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


//blur 2
__KERNEL__ void OilSpillChocolateSyrupFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float2 res = iResolution;
    float2 uv = fragCoord / res;
    float2 step = (1.00f / res);
    float4 u = texture(iChannel0, uv + to_float2(0.0f,-step.y));
    float4 d = texture(iChannel0, uv + to_float2(0.0f, step.y));
    float4 l = texture(iChannel0, uv + to_float2(-step.x,0.0f));
    float4 r = texture(iChannel0, uv + to_float2(step.x,0.0f));
    float4 o = (u+d+l+r)*0.25f;
    fragColor = o;//
    fragColor.w = _tex2DVecN(iChannel0,uv.x,uv.y,15).w;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1


//light from https://www.shadertoy.com/view/MsGSRd

__DEVICE__ float colormap_red(float x) {
    return 1.61361058036781E+00 * x - 1.55391688559828E+02;
}

__DEVICE__ float colormap_green(float x) {
    return 9.99817607003891E-01 * x + 1.01544260700389E+00;
}

__DEVICE__ float colormap_blue(float x) {
    return 3.44167852062589E+00 * x - 6.19885917496444E+02;
}

__DEVICE__ float4 colormap(float x) {
    float t = x * 255.0f;
    float r = clamp(colormap_red(t) / 255.0f, 0.0f, 1.0f);
    float g = clamp(colormap_green(t) / 255.0f, 0.0f, 1.0f);
    float b = clamp(colormap_blue(t) / 255.0f, 0.0f, 1.0f);
    return to_float4(r, g, b, 1.0f);
}

__DEVICE__ float getVal(float2 uv, __TEXTURE2D__ iChannel0)
{
    return length(swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z));
}
    
__DEVICE__ float2 getGrad(float2 uv,float delta, __TEXTURE2D__ iChannel0)
{
    float2 d=to_float2(delta,0);
    return to_float2(
        getVal(uv+swi2(d,x,y),iChannel0)-getVal(uv-swi2(d,x,y),iChannel0),
        getVal(uv+swi2(d,y,x),iChannel0)-getVal(uv-swi2(d,y,x),iChannel0)
    )/delta;
}

__KERNEL__ void OilSpillChocolateSyrupFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    fragCoord+=0.5f;    
    
    float2 res = iResolution;
    float2 uv = fragCoord / res;
    
    float3 n = to_float3_aw(getGrad(uv,1.0f/iResolution.y,iChannel0),350.0f);
    n=normalize(n);
    float3 light = normalize(to_float3(0.01f,0.75f,2.25f));
    
    //interactive light
    //vec3 light = normalize(to_float3(swi2(iMouse,x,y) / iResolution.y,1.25f) - to_float3_aw(fragCoord / iResolution.y,0.0f));

    float _diff=clamp(dot(n,light),0.5f,1.0f);
    float spec=clamp(dot(reflect(light,n),to_float3(0,0,-1)),0.0f,1.0f);
    spec=_powf(spec,64.0f)*1.5f;
    
    float osc = _sinf(iTime*0.25f)*0.5f + 0.5f;
    float4 fb = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float avg = dot(swi3(fb,x,y,z), to_float3_s(1.0f))*0.333333f;
    
    fb = colormap(fb.x);
    fb = swi4(fb,y,x,z,w);//.grba;
    swi3S(fb,x,y,z, swi3(fb,x,y,z) + to_float3(0.05f,-0.05f,0.0f));
    //fb*=0.95f;
  
    fragColor = fb * _diff + spec;

  SetFragmentShaderComputedColor(fragColor);
}
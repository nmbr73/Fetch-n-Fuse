
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define CL(x) clamp(x,0.0f,0.5f)
#define R iResolution
#define F iFrame
#define T mod_f((float)(iFrame)/60.0f,20.0f)
#define PI 2.0f*_asinf(1.0f)
#define E _expf(1.0f)
__DEVICE__ float4 hash44(float4 p4)
{
  p4 = fract_f4(p4  * to_float4(0.1031f, 0.1030f, 0.0973f, 0.1099f));
  p4 += dot(p4, swi4(p4,w,z,x,y)+19.19f);
  return fract_f4((swi4(p4,x,x,y,z)+swi4(p4,y,z,z,w))*swi4(p4,z,y,w,x));
}

__DEVICE__ float2 cis(float t){
    return cos_f2(t - to_float2(0,PI/2.0f));
}
__DEVICE__ float2 cexp(float2 z) {
    return _expf(z.x)*cis(z.y);
}
__DEVICE__ float2 clog(float2 z) {
    return to_float2(_logf(length(z)),_atan2f(z.y,z.x));
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


//Store all the particles here
__KERNEL__ void Cranks2DifficultJipiFuse__Buffer_A(float4 o, float2 i, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0)
{
    i+=0.5f;
    float2 uv = i/R;
    
    o = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    float v = _exp2f(7.0f-_floor(0.7f*T+T*T/20.0f));
float AAAAAAAAAAAAAAA;    
    if(T<0.3f){
        o = to_float4(_floor(uv.x/v*R.x)*v/R.x*0.8f+0.1f,0,0,5.1f);
    }
    
    swi2S(o,x,y, swi2(o,x,y)+swi2(o,z,w)/R*2.0f);
    float4 r = hash44(to_float4(_floor(i.x/v),_floor(i.y/v),F,iMouse.x));
    r.z = _sqrtf(-2.0f*_logf(r.z));
    r.w *= 6.28318f;
    swi2S(r,z,w, r.z*to_float2(_cosf(r.w),_sinf(r.w))*0.4f);
    swi2S(o,z,w, swi2(o,z,w)+swi2(r,z,w)*(0.1f+T/2000.0f));
    o.w -= 0.6f/(100.0f+T*30.0f);
    float l = length(swi2(o,z,w));
    swi2S(o,z,w, swi2(o,z,w)*_fmaxf(0.0f,(_powf(l,0.9f+r.x*0.1f)*(1.0f-T/30.0f))/l));
    float t = _atan2f(o.w,o.z);
    if(o.w>0.0f)
        t += 0.01f*sign_f(o.z);
    swi2S(o,z,w, length(swi2(o,z,w))*to_float2(_cosf(t),_sinf(t)));

  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


//This is the first pass of the rendering/search

__KERNEL__ void Cranks2DifficultJipiFuse__Buffer_B(float4 o, float2 i, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    i+=0.5f;

    int i1 = 100;
float BBBBBBBBBBBBBBBBBB;
    o = to_float4_s(0);
    for(int a = 0; a < i1; a++){
        float4 r = hash44(to_float4(i.x,i.y,F,a));//randomly read points from buffer A
        float4 p = _tex2DVecN(iChannel0,r.x,r.y,15);
        float l = length(swi2(p,x,y)*R-i);
        if(l < length(swi2(o,x,y)*R-i)){//save only the closest to this pixel
            o = p;
        }
    }

  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void Cranks2DifficultJipiFuse__Buffer_C(float4 o, float2 i, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    i+=0.5f;

    float s = 15.0f;  //search radius
    int i1 = 100;
float CCCCCCCCCCCCCCC;
    float2 uv = i/R;
    o = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    for(int a = 0; a < i1; a++){
        float4 r = hash44(to_float4(i.x,i.y,F,a));//Transform this uniform random into a normal distribution
        r.z = _sqrtf(-2.0f*_logf(r.z));
        r.w *= 6.28318f;
        swi2S(r,z,w, r.z*to_float2(_cosf(r.w),_sinf(r.w))*s);
        float4 p = _tex2DVecN(iChannel0,(i.x+r.x)/R.x,(i.y+r.y)/R.y,15);//sample random nearby points
        if((p.x!=0.0f || p.y!=0.0f) && length(swi2(p,z,w))>0.001f)
          o += to_float4(length(swi2(p,z,w)),0.5f+0.5f*_sinf(p.z),0.5f+0.5f*_cosf(4.0f*p.w),1)/(1.0f+_expf((2.0f+T/5.0f)*length(swi2(p,x,y)*R-i))); //add a gaussian to the accumulated image from the particle 
    }
    if(T<0.3f){
        o = to_float4_s(0);
    }

  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void Cranks2DifficultJipiFuse(float4 o, float2 i, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    i+=0.5f;
float IIIIIIIIIIIIIIIII;
    float2 uv = i/R;
    o = (_tex2DVecN(iChannel0,uv.x,uv.y,15)/1e2);
    
  SetFragmentShaderComputedColor(o);
}
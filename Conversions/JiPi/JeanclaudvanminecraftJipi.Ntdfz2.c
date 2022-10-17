
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0
// Connect Image 'Texture: Video' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



// Created by Andrew Wild - akohdr/2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

//#define ROTATE
//#define TECHN_IQ_COLOR
    
// Lower sampling res
//#define LOOP 199
//#define RES 1.0f

// Higher sampling res.
#define LOOP 899
#define RES 6.0f

__DEVICE__ bool isVoxel(out float4 *k, const in float4 P, float2 iResolution, __TEXTURE2D__ iChannel1 ) 
{
    float3 a = abs_f3(swi3(P,x,y,z));
    if(a.z<1.0f) {
        
        float2 uv = swi2(P,x,y)/iResolution;
        if(uv.x < -0.02f || uv.y>0.03f) return false;
        uv -= to_float2(0.05f,0.03f);
        uv *= 15.0f;
        
        *k = texture(iChannel1, 1.0f+uv);

#ifdef TECHN_IQ_COLOR
    float maxrb = _fmaxf( (*k).x, (*k).z );
    float dg = (*k).y; 
    (*k).y = _fminf( (*k).y, maxrb*0.8f ); 
    *k += dg - (*k).y;
#endif
float zzzzzzzzzzzzzzz;        
        return (((*k).y<0.3f) && (((*k).x>0.0f)||((*k).z>0.0f))) ||
               (((*k).y<0.6f) && (((*k).x>0.3f)||((*k).z>0.4f))) ||
               (((*k).y<0.9f) && (((*k).x>0.7f)||((*k).z>0.7f)));
    }
    return false;
}

__KERNEL__ void JeanclaudvanminecraftJipiFuse(float4 k, float2 P, float iTime, float2 iResolution, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1)
{

     float2 R = iResolution,
          h = to_float2(0,0.5f),
          u = (P - h*R.y)/R.x - swi2(h,y,x);
#ifdef ROTATE
    float T = 3.0f*_cosf(iTime);
#else
    float T = 2.3f;
#endif
float IIIIIIIIIIIIIIII;    
    float3 v = to_float3(_cosf(T), 1, _sinf(T)),
         r = mul_mat3_f3(to_mat3(u.x,    0,   0.8f,
                                   0,  u.y,    0,
                               -0.8f,    0,  u.x) , v),
         o = to_float3(50,2.0f,-50) * swi3(v,z,z,x),
         f = _floor(o),
         q = sign_f3(r),
         d = abs_f3(length(r)/r),
         s = d * ( q*(f-o + 0.5f) +0.5f), 
         m;

    for(int i=0; i<LOOP; i++) {
        float a = s.x, b = s.y, c = s.z;
        s += d*(m = to_float3(a<b&&a<=c, b<c&&b<=a, c<a&&c<=b));
        f += m/RES*q;
        
        if(isVoxel(&k, to_float4_aw(swi3(f,x,y,z), T),R,iChannel1) || isVoxel(&k, to_float4_aw(swi3(f,z,y,x), T),R,iChannel1)) {
            k += m.x>0.0f ? to_float4_s(0) : m.y>0.0f ? to_float4_s(0.6f) : to_float4_s(0.3f); 
            
            SetFragmentShaderComputedColor(k);
            return; 
        }//early exit
    }
    k = texture(iChannel0, P/R)/3.0f;


  SetFragmentShaderComputedColor(k);
}
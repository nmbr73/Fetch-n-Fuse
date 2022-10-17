
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 1' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// shader derived from Heartfelt - by Martijn Steinrucken aka BigWings - 2017
// https://www.shadertoy.com/view/ltffzl
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

#define S(a, b, t) smoothstep(a, b, t)
//#define DEBUG
#define size 0.2f
#define CAM // uncomment to switch from webcam input to iChannel1 texture


__DEVICE__ float3 N13(float p) {
   //  from DAVE HOSKINS
   float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f,0.11369f,0.13787f));
   p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
   return fract_f3(to_float3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}

__DEVICE__ float4 N14(float t) {
  return fract_f4(sin_f4(t*to_float4(123.0f, 1024.0f, 1456.0f, 264.0f))*to_float4(6547.0f, 345.0f, 8799.0f, 1564.0f));
}
__DEVICE__ float N(float t) {
    return fract(_sinf(t*12345.564f)*7658.76f);
}

__DEVICE__ float Saw(float b, float t) {
  return S(0.0f, b, t)*S(1.0f, b, t);
}



__DEVICE__ float2 Drops(float2 uv, float t) {
    
    float2 UV = uv;
    
    // DEFINE GRID
    uv.y += t*0.8f;
    float2 a = to_float2(6.0f, 1.0f);
    float2 grid = a*2.0f;
    float2 id = _floor(uv*grid);
    
    // RANDOM SHIFT Y
    float colShift = N(id.x); 
    uv.y += colShift;
    
    // DEFINE SPACES
    id = _floor(uv*grid);
    float3 n = N13(id.x*35.2f+id.y*2376.1f);
    float2 st = fract_f2(uv*grid)-to_float2(0.5f, 0);
    
    // POSITION DROPS
    //clamp(2*x,0,2)+clamp(1-x*0.5f, -1.5f, 0.5f)+1.5f-2
    float x = n.x-0.5f;
    
    float y = UV.y*20.0f;
    float distort = _sinf(y+_sinf(y));
    x += distort*(0.5f-_fabs(x))*(n.z-0.5f);
    x *= 0.7f;
    float ti = fract(t+n.z);
    y = (Saw(0.85f, ti)-0.5f)*0.9f+0.5f;
    float2 p = to_float2(x, y);
    
    // DROPS
    float d = length((st-p)*swi2(a,y,x));
    
    float dSize = size; 
    
    float Drop = S(dSize, 0.0f, d);
    
    
    float r = _sqrtf(S(1.0f, y, st.y));
    float cd = _fabs(st.x-x);
    
    // TRAILS
    float trail = S((dSize*0.5f+0.03f)*r, (dSize*0.5f-0.05f)*r, cd);
    float trailFront = S(-0.02f, 0.02f, st.y-y);
    trail *= trailFront;
    
    
    // DROPLETS
    y = UV.y;
    y += N(id.x);
    float trail2 = S(dSize*r, 0.0f, cd);
    float droplets = _fmaxf(0.0f, (_sinf(y*(1.0f-y)*120.0f)-st.y))*trail2*trailFront*n.z;
    y = fract(y*10.0f)+(st.y-0.5f);
    float dd = length(st-to_float2(x, y));
    droplets = S(dSize*N(id.x), 0.0f, dd);
    float m = Drop+droplets*r*trailFront;
    
    #ifdef DEBUG
    m += st.x>a.y*0.45f || st.y>a.x*0.165f ? 1.2f : 0.0f; //DEBUG SPACES
    #endif
    
    
    return to_float2(m, trail);
}

__DEVICE__ float StaticDrops(float2 uv, float t) {
    uv *= 30.0f;
    
    float2 id = _floor(uv);
    uv = fract_f2(uv)-0.5f;
    float3 n = N13(id.x*107.45f+id.y*3543.654f);
    float2 p = (swi2(n,x,y)-0.5f)*0.5f;
    float d = length(uv-p);
    
    float fade = Saw(0.025f, fract(t+n.z));
    float c = S(size, 0.0f, d)*fract(n.z*10.0f)*fade;

    return c;
}

__DEVICE__ float2 Rain(float2 uv, float t) {
    float s = StaticDrops(uv, t); 
    float2 r1 = Drops(uv, t);
    float2 r2 = Drops(uv*1.8f, t);
    
    #ifdef DEBUG
    float c = r1.x;
    #else
    float c = s+r1.x+r2.x;
    #endif
    
    c = S(0.3f, 1.0f, c);
    
    #ifdef DEBUG
    return to_float2(c, r1.y);
    #else
    return to_float2(c, _fmaxf(r1.y, r2.y));
    #endif
}


__KERNEL__ void SittingByTheWindowJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 uv = (fragCoord-0.5f*iResolution) / iResolution.y;
    float2 UV = fragCoord/iResolution;
    float T = iTime;
    
    
    float t = T*0.2f;
    
    float rainAmount = 0.8f;
    

    
    UV = (UV-0.5f)*(0.9f)+0.5f;
    
    float2 c = Rain(uv, t);

    float2 e = to_float2(0.001f, 0.0f); //pixel offset
    float cx = Rain(uv+e, t).x;
    float cy = Rain(uv+swi2(e,y,x), t).x;
    float2 n = to_float2(cx-c.x, cy-c.x); //normals
    
    float blur = 5.0f;    
    float focus = blur-c.y*blur*0.75f;
    
    #ifdef CAM

        // BLUR derived from existical https://www.shadertoy.com/view/Xltfzj
        float Pi = 6.28318530718f; // Pi*2
    
        // GAUSSIAN BLUR SETTINGS {{{
        float Directions = 16.0f; // BLUR DIRECTIONS (Default 16.0f - More is better but slower)
        float Quality = 3.0f; // BLUR QUALITY (Default 4.0f - More is better but slower)
        float Size = 32.0f; // BLUR SIZE (Radius)
        // GAUSSIAN BLUR SETTINGS }}}

        float2 Radius = Size/iResolution;

        float3 col = swi3(_tex2DVecN(iChannel0,UV.x,UV.y,15),x,y,z);//.rgb;
            // Blur calculations
        for( float d=0.0f; d<Pi; d+=Pi/Directions)
        {
            for(float i=1.0f/Quality; i<=1.0f; i+=1.0f/Quality)
            {
                #ifdef DEBUG
                float3 tex = swi3(texture( iChannel0, UV+c+to_float2(_cosf(d),_sinf(d))*Radius*i),x,y,z);//.rgb;
                #else
                float3 tex = swi3(texture( iChannel0, UV+n+to_float2(_cosf(d),_sinf(d))*Radius*i),x,y,z);//.rgb;
                #endif

                col += tex;            
            }
        }

        col /= Quality * Directions - 0.0f;

        float3 tex = swi3(texture( iChannel0, UV+n),x,y,z);//.rgb;
        c.y = clamp(c.y, 0.0f, 1.0f);

        col -= c.y;
        col += c.y*(tex+0.75f);

    #else
    float3 col = swi3(texture(iChannel1, UV+n),x,y,z);//.rgb;
    #endif
    
    fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
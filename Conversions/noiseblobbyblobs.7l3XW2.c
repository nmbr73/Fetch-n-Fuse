
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ float fract_f(float A){return A - _floor(A);}
__DEVICE__ float2 fract_f2(float2 A){return make_float2(A.x - _floor(A.x), A.y - _floor(A.y));}
__DEVICE__ float mod_f(float value, float divisor) {  return value - divisor * _floor(value / divisor);}
__DEVICE__ float3 cos_f3(float3 i) {float3 r; r.x = _cosf(i.x); r.y = _cosf(i.y); r.z = _cosf(i.z); return r;}
__DEVICE__ float3 tanh_f3(float3 i) {float3 r; r.x = _tanhf(i.x); r.y = _tanhf(i.y); r.z = _tanhf(i.z); return r;}

#define in

#define pi 3.14159

__DEVICE__ float thc(float a, float b) {
    return _tanhf(a * _cosf(b)) / _tanhf(a);
}

__DEVICE__ float ths(float a, float b) {
    return _tanhf(a * _sinf(b)) / _tanhf(a);
}

__DEVICE__ float3 thc(float a, float3 b) {
    return tanh_f3(a * cos_f3(b)) / _tanhf(a);
}

__DEVICE__ float h21 (float2 a) {
    return fract_f(_sinf(dot(swixy(a), to_float2(12.9898f, 78.233f))) * 43758.5453123f);
}

__DEVICE__ float h21 (float a, float b, float sc) {
    a = mod_f(a, sc); b = mod_f(b, sc);
    return fract_f(_sinf(dot(to_float2(a, b), to_float2(12.9898f, 78.233f)))*43758.5453123f);
}

__DEVICE__ float sdSegment( in float2 p, in float2 a, in float2 b )
{
    float2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h );
}

__DEVICE__ float2 pnt(float2 ipos, float sc, float iTime) {
    float h = h21(ipos.x, ipos.y, sc);
    float t = iTime + 10.0f * h;
    float k = 1.5f +  h;
    return 0.4f * to_float2(thc(4.0f * (1.0f-h), 100.0f + k * t),
                            ths(4.0f * h, 100.0f + (1.0f-k) * t));
}

__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b * cos_f3(6.28318f*(c*t+d) );
}

__DEVICE__ float fn(float2 p, float time) {
    float m = fract_f(h21(p) + time);
    return m * m * (3.0f - 2.0f * m);
}

__DEVICE__ float test(float2 p, float time, float a) {
    return h21(_floor(h21(p) + time) + a + 0.01f * p);
}



__KERNEL__ void noiseblobbyblobsKernel(
__CONSTANTREF__ Params*  params,
__TEXTURE2D__            iChannel0,
__TEXTURE2D_WRITE__      dst
 ){

 PROLOGUE;
 //float4 fragColor = fragColor;
 //float2 fragCoord  = fragCoord;

    float2 uv = (fragCoord - 0.5f * swixy(iResolution))/ iResolution.y;
    float2 ouv = uv;
   // ouv *= 2.3f;

    float a = _atan2f(uv.y, uv.x);
    float r = length(uv);
    uv.y += 0.02f * iTime; // 0.06

    float sc = 20.0f;
    float2 fpos = fract_f2(sc * uv) - 0.0f; // dont include -0.5f, so box lerp is easier
    float2 ipos = _floor(sc * uv) + 0.0f;

    // box corner points
    float2 lp = ipos + to_float2(1.0f,0.0f);
    float2 tp = ipos + to_float2(0.0f,1.0f);
    float2 tlp = ipos + to_float2_s(1.0f);
    float2 idp = ipos;

    float time = 0.5f * iTime;

    // corner value 1: _floor(time)
    float l = test(lp, time, 0.0f);
    float t = test(tp, time, 0.0f);
    float tl = test(tlp, time, 0.0f);
    float id = test(idp, time, 0.0f);

    // corner value 2: _floor(time) + 1.
    float l2 = test(lp, time, 1.0f);
    float t2 = test(tp, time, 1.0f);
    float tl2 = test(tlp, time, 1.0f);
    float id2 = test(idp, time, 1.0f);

    // lerp between corner values, present and future
    l = _mix(l, l2, fn(lp, time));
    t = _mix(t, t2, fn(tp, time));
    tl = _mix(tl, tl2, fn(tlp, time));
    id = _mix(id, id2, fn(idp, time));

    // smooth fpos so end points meet continuously
    float2 sfpos = fpos * fpos * (3.0f - 2.0f * fpos);

    // box lerp corner values
    float v = l  * sfpos.x      * (1.0f-sfpos.y)
             + t  * (1.0f-sfpos.x) * sfpos.y
             + tl * sfpos.x      * sfpos.y
              + id * (1.0f-sfpos.x) * (1.0f-sfpos.y);

    // remove me to see grid version
    v += 0.4f + 1.2f * (1.0f - length(ouv)) * _cosf(20.0f * length(ouv) + 0.0f * _atan2f(ouv.y, ouv.x) - 2.0f * iTime);

    float k = 0.1f;
    float s = smoothstep(-k, k, -v + 0.3f);
    s = _powf(4.0f * (1.0f-s) * s, 2.0f);

    float k2 = 0.1f;
    s = step(0.4f, s); //smoothstep(-k2, k2, s - 0.4f);

    float3 col = to_float3_s(s);
    float3 e = to_float3_s(1.0f);
    col = s * pal(h21((ipos)), e, e, e, to_float3(0.0f,0.33f,0.66f));


    fragColor = to_float4_aw(col, 1.0f); //to_float4(v);

	EPILOGUE(fragColor);
}

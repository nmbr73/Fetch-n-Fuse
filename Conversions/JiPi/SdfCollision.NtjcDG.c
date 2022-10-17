
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

// ------------------------------------------------------------

#define BALL_R 0.05f
#define BALL_INIT_POS to_float2(0.0f, 0.3f)
#define BALL_INIT_ANGLE 45.0f

#define MOVE_SPEED 2.2f

__DEVICE__ float box(float2 p, float2 s) {
    float2 bd = abs_f2(p) - s;
    return length(_fmaxf(bd, to_float2_s(0.0f))) + _fminf(_fmaxf(bd.x, bd.y), 0.0f);
}

__DEVICE__ float star(float2 p, float r, float rf) {
    const float2 k1 = to_float2(0.809016994375f, -0.587785252292f);
    const float2 k2 = to_float2(-k1.x, k1.y);
    p.x = _fabs(p.x);
    p -= 2.0f * _fmaxf(dot(k1,p),0.0f) * k1;
    p -= 2.0f * _fmaxf(dot(k2,p),0.0f) * k2;
    p.x = _fabs(p.x);
    p.y -= r;
    float2 ba = rf * to_float2(-k1.y, k1.x) - to_float2(0, 1);
    float h = clamp( dot(p, ba) / dot(ba, ba), 0.0f, r);
    return length(p - ba * h) * sign_f(p.y * ba.x - p.x * ba.y);
}

__DEVICE__ float moon(float2 p, float d, float ra, float rb) {
    p.y = _fabs(p.y);
    float a = (ra * ra - rb * rb + d * d) / (2.0f * d);
    float b = _sqrtf(_fmaxf(ra * ra - a * a, 0.0f));
    if (d * (p.x * b - p.y * a) > d * d * _fmaxf(b - p.y, 0.0f))
        return length(p - to_float2(a, b));
    return _fmaxf((length(p) - ra), -(length(p - to_float2(d, 0)) - rb));
}

__DEVICE__ float map(float2 p, float iTime) {
    float d = 0.0f;
    
    float4 r = to_float4(0.0f, 0.5f, 0.2f, 0.3f);
    swi2S(r,x,y, (p.x > 0.0f) ? swi2(r,x,y) : swi2(r,z,w));
    r.x  = (p.y > 0.0f) ? r.x : r.y;
    float2 q = abs_f2(p) - to_float2(1.6f, 0.9f) + r.x;
    d = _fminf(_fmaxf(q.x, q.y), 0.0f) + length(_fmaxf(q, to_float2_s(0.0f))) - r.x;
    
    float c = length(p) - 0.2f;
    
    float cs = _cosf(iTime), ss = _sinf(iTime);
    mat2 rot1 = to_mat2(cs, -ss, ss, cs);
    mat2 rot2 = to_mat2(cs, ss, -ss, cs);
    
    float2 cp = abs_f2(mul_mat2_f2(rot1 , (p - to_float2(-1.0f, 0.5f))));
    c = _fminf(c, length(cp - _fminf(cp.x + cp.y, 0.25f) * 0.5f) - 0.025f);
    
    c = _fminf(c, box(mul_mat2_f2(rot1 , (p - to_float2(-1.0f, -0.5f))), to_float2(0.1f, 0.2f)));
    
    c = _fminf(c, star(mul_mat2_f2(rot2 , (p - to_float2(1.0f, 0.5f))), 0.18f, 0.4f));
    
    c = _fminf(c, moon(mul_mat2_f2(rot2 , (p - to_float2(1.0f, -0.5f))), 0.05f, 0.18f, 0.14f));
    
    d = _fmaxf(-c, d);
    
    return d;
}

__DEVICE__ float2 nrm(float2 p, float iTime) {
    float2 eps = to_float2(0.001f, 0.0f);
  return normalize(to_float2(
    map(p + swi2(eps,x,y),iTime) - map(p-swi2(eps,x,y),iTime),
    map(p + swi2(eps,y,x),iTime) - map(p-swi2(eps,y,x),iTime)
  ));
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


#define _reflect(I,N) (I-2.0f*dot(N,I)*N)

__KERNEL__ void SdfCollisionFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, sampler2D iChannel0)
{

   fragCoord+=0.5f;

    const float ia = BALL_INIT_ANGLE * 3.1415926536f / 180.0f;
    const float2 ip = BALL_INIT_POS;

    float2 uv = fragCoord / iResolution;
float AAAAAAAAAAAAA;    
    float4 d = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    
    if (d.z == 0.0f) {
        //swi2(d,z,w) = to_float2(_cosf(ia), _sinf(ia)) * 0.5f + 0.5f;
        d.z = _cosf(ia) * 0.5f + 0.5f;
        d.w = _sinf(ia) * 0.5f + 0.5f;
        
        //swi2(d,x,y) = ip;
        d.x = ip.x;
        d.y = ip.y;
    } else {
        float2 v = swi2(d,z,w) * 2.0f - 1.0f;
        float2 p = swi2(d,x,y);
        p += v * 0.0166f * MOVE_SPEED;
        
        float dist = map(p,iTime);
        if (dist > -BALL_R) {
            p -= (dist + BALL_R) * v;
float AAAAAAAAAAAAA;            
            float2 n = nrm(p,iTime);
            float2 o = _reflect(v, n);
            
            //swi2(d,z,w) = o * 0.5f + 0.5f;
            d.z = o.x * 0.5f + 0.5f;
            d.w = o.y * 0.5f + 0.5f;
        }
        
        //swi2(d,x,y) = p;
        d.x = p.x;
        d.y = p.y;
    }
    
    fragColor = d;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0

__DEVICE__ float2 _fwidth(float2 inp, float2 iR, float Par){
    //simulate fwidth
    float uvx = inp.x + Par/iR.x;
    float ddx = uvx * uvx - inp.x * inp.x;

    float uvy = inp.y + Par/iR.y;
    float ddy = uvy * uvy - inp.y * inp.y;

    return to_float2(_fabs(ddx), _fabs(ddy));
}

__DEVICE__ float _fwidth(float inp, float2 iR, float Par,float Par2){
    //simulate fwidth
    float uvx = inp+Par2 + Par/iR.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp+Par2 + Par/iR.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}



__KERNEL__ void SdfCollisionFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, sampler2D iChannel0)
{
  CONNECT_SLIDER0(Par, -10.0f, 10.0f, 1.0f);
  CONNECT_SLIDER1(Par2, -10.0f, 10.0f, 0.0f);
  
    fragCoord+=0.5f;
 float IIIIIIIIIIIIIII;
    float2 uv = (2.0f * fragCoord - iResolution) / iResolution.y;
 

 
    float w = length(_fwidth(uv,iResolution,Par));
    //float w = length(to_float2(_fwidth(uv.x,iResolution,Par,Par2),_fwidth(uv.y,iResolution,Par,Par2)));
    
    float d = map(uv,iTime);
    float c = smoothstep(-w, w, _fabs(d));

    float3 col = to_float3_s(c);
    
    float4 buf = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float2 bp = swi2(buf,x,y);
    
    float ball = length(uv - bp) - BALL_R;
    ball = smoothstep(-w, w, _fabs(ball));
    
    col *= ball;
    
    fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define buf(p) texture(iChannel0,(p)/iResolution)

#define N  to_float2( 0, 1)
#define NE to_float2( 1, 1)
#define E  to_float2( 1, 0)
#define SE to_float2( 1,-1)
#define S  to_float2( 0,-1)
#define SW to_float2(-1,-1)
#define W  to_float2(-1, 0)
#define NW to_float2(-1, 1)

#define PI 3.14159265359f

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Lichen' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// 2018 David A Roberts https://davidar.io

__DEVICE__ float slope(float2 p, float2 q, float2 R, __TEXTURE2D__ iChannel0 ) {
  
    return ((buf(q)).x - (buf(p)).x) / distance_f2(p,q);
}

__DEVICE__ float2 rec(float2 p, float2 R, __TEXTURE2D__ iChannel0) { // direction of water flow at point
    float2 d = N;
    if (slope(p + NE, p, R, iChannel0) > slope(p + d, p, R, iChannel0)) d = NE;
    if (slope(p + E,  p, R, iChannel0) > slope(p + d, p, R, iChannel0)) d = E;
    if (slope(p + SE, p, R, iChannel0) > slope(p + d, p, R, iChannel0)) d = SE;
    if (slope(p + S,  p, R, iChannel0) > slope(p + d, p, R, iChannel0)) d = S;
    if (slope(p + SW, p, R, iChannel0) > slope(p + d, p, R, iChannel0)) d = SW;
    if (slope(p + W,  p, R, iChannel0) > slope(p + d, p, R, iChannel0)) d = W;
    if (slope(p + NW, p, R, iChannel0) > slope(p + d, p, R, iChannel0)) d = NW;
    return d;
}

__KERNEL__ void RiverbasinJipi552Fuse__Buffer_A(float4 r, float2 p, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    p+=0.5f;


    if (iFrame < 10 || iMouse.z > 0.0f || Reset) {
        r = to_float4_s(0);
        r.x = texture(iChannel1, p / iResolution).x + 9.0f * p.x/iResolution.x;
        SetFragmentShaderComputedColor(r);
        return;
    }
    r = buf(p);
    
    // flow accumulation
    r.y = 1.0f;
    
    
    
    //if (rec(p + N, R, iChannel0)  == -1.0f*N)  r.y += buf(p + N).y;
    if (rec(p + N, R, iChannel0).x  == (-1.0f*N).x && rec(p + N, R, iChannel0).y  == (-1.0f*N).y)  r.y += buf(p + N).y;
        
    //if (rec(p + NE, R, iChannel0) == -1.0f*NE) r.y += buf(p + NE).y;
    if (rec(p + NE, R, iChannel0).x == (-1.0f*NE).x && rec(p + NE, R, iChannel0).y == (-1.0f*NE).y) r.y += buf(p + NE).y;
    
    //if (rec(p + E, R, iChannel0)  == -1.0f*E)  r.y += buf(p + E).y;
    if (rec(p + E, R, iChannel0).x  == (-1.0f*E).x && rec(p + E, R, iChannel0).y  == (-1.0f*E).y)  r.y += buf(p + E).y;
    
    //if (rec(p + SE, R, iChannel0) == -1.0f*SE) r.y += buf(p + SE).y;
    if (rec(p + SE, R, iChannel0).x == (-1.0f*SE).x && rec(p + SE, R, iChannel0).y == (-1.0f*SE).y) r.y += buf(p + SE).y;
    //if (rec(p + S, R, iChannel0)  == -1.0f*S)  r.y += buf(p + S).y;
    if (rec(p + S, R, iChannel0).x  == (-1.0f*S).x && rec(p + S, R, iChannel0).y  == (-1.0f*S).y)  r.y += buf(p + S).y;
    //if (rec(p + SW, R, iChannel0) == -1.0f*SW) r.y += buf(p + SW).y;
    if (rec(p + SW, R, iChannel0).x == (-1.0f*SW).x && rec(p + SW, R, iChannel0).y == (-1.0f*SW).y) r.y += buf(p + SW).y;
    //if (rec(p + W, R, iChannel0)  == -1.0f*W)  r.y += buf(p + W).y;
    if (rec(p + W, R, iChannel0).x  == (-1.0f*W).x && rec(p + W, R, iChannel0).y  == (-1.0f*W).y)  r.y += buf(p + W).y;
    //if (rec(p + NW, R, iChannel0) == -1.0f*NW) r.y += buf(p + NW).y;
    if (rec(p + NW, R, iChannel0).x == (-1.0f*NW).x && rec(p + NW, R, iChannel0).y == (-1.0f*NW).y) r.y += buf(p + NW).y;
    if (p.y < 2.0f || p.y > iResolution.y - 2.0f) r.y = 0.0f;
    
    // stream power
    float4 receiver = buf(p + rec(p, R, iChannel0));
    float pslope = (r.x - receiver.x) / length(rec(p, R, iChannel0));
    r.x = _fmaxf(r.x - 0.5f * _powf(r.y, 0.8f) * _powf(pslope, 2.0f), receiver.x);
    
    // tectonic uplift
    r.x += 0.005f * p.x/iResolution.x;
    r.x += 0.005f * (0.5f + (_fabs(mod_f(iTime/5.0f, 4.0f) - 2.0f) - 1.0f) * (p.y/iResolution.y - 0.5f));
    
    // basin colouring
    r.z = (p.x < 10.0f) ? p.y/iResolution.y : receiver.z;

  SetFragmentShaderComputedColor(r);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// uncomment next line for hillshading
//#define TERRAIN

__KERNEL__ void RiverbasinJipi552Fuse(float4 r, float2 p, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX1(Terrain, 0);
    CONNECT_COLOR0(Color, 0.0f, 23.0f, 21.0f, 1.0f);

    r = to_float4(0,0,0,1);
    float4 c = buf(p);
    if (Terrain)
    {
      float2 grad = to_float2(buf(p+E).x - buf(p+W).x, buf(p+N).x - buf(p+S).x);
      r = to_float4_aw(0.1f * to_float3_s(1.0f + _cosf(_atan2f(grad.y, grad.x) + 0.25f*PI)), Color.w);
    }
    if (p.x > 2.0f && c.z > 0.0f && c.y > 1.0f)
        r = to_float4_aw(swi3(r,x,y,z) + 0.15f * _logf(c.y) * (0.6f + 0.6f * cos_f3(6.3f * c.z + swi3(Color,x,y,z))), Color.w);

  SetFragmentShaderComputedColor(r);
}
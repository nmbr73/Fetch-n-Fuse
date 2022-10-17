
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Blending' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution

// Based on "spilled" by florian berger (flockaroo) https://www.shadertoy.com/view/MsGSRd
// Click and drag to inject color
// Press I to pick a different start seed

// RotNum has to be an odd integer
#define RotNum 5

//#define keyTex iChannel1
//#define KEY_I texture(keyTex,to_float2((105.5f-32.0f)/256.0f,(0.5f+0.0f)/3.0f)).x

#define third  (1.0f / 3.0f)
#define sixth  (1.0f / 6.0f)


__DEVICE__ float4 permute(float4 x) {
    float4 xm = mod_f(x, 289.0f);
    return mod_f4(((xm * 34.0f) + 10.0f) * xm, 289.0f);
}

// Stefan Gustavson's and Ian McEwan's implementation of simplex noise (patent is now expired)
// https://github.com/stegu/psrdnoise
__DEVICE__ float3 psrdnoise(float3 x) {
    float3 uvw = x + dot(x, to_float3_s(third));
    float3 i0 = _floor(uvw);
    float3 f0 = fract_f3(uvw);
    float3 g_ = step(swi3(f0,x,y,x), swi3(f0,y,z,z));
    float3 l_ = 1.0f - g_;
    float3 g = to_float3(l_.z, g_.x, g_.y);
    float3 l = to_float3_aw(swi2(l_,x,y), g_.z);
    float3 o1 = _fminf(g, l);
    float3 o2 = _fmaxf(g, l);
    float3 i1 = i0 + o1;
    float3 i2 = i0 + o2;
    float3 i3 = i0 + 1.0f;
    float3 v0 = i0 - dot(i0, to_float3_s(sixth));
    float3 v1 = i1 - dot(i1, to_float3_s(sixth));
    float3 v2 = i2 - dot(i2, to_float3_s(sixth));
    float3 v3 = i3 - dot(i3, to_float3_s(sixth));
    float3 x0 = x - v0;
    float3 x1 = x - v1;
    float3 x2 = x - v2;
    float3 x3 = x - v3;
    float4 hash = permute(permute(permute(
                                          to_float4(i0.z, i1.z, i2.z, i3.z))
                                        + to_float4(i0.y, i1.y, i2.y, i3.y))
                                        + to_float4(i0.x, i1.x, i2.x, i3.x));
    float4 theta = hash * 3.883222077f;
    float4 sz = hash * -0.006920415f + 0.996539792f;
    float4 psi = hash * 0.108705628f;
    float4 Ct = cos_f4(theta);
    float4 St = sin_f4(theta);
    float4 sz_prime = sqrt_f4(to_float4_s(1.0f) - sz * sz);
    float4 gx = Ct * sz_prime;
    float4 gy = St * sz_prime;
    float3 g0 = to_float3(gx.x, gy.x, sz.x);
    float3 g1 = to_float3(gx.y, gy.y, sz.y);
    float3 g2 = to_float3(gx.z, gy.z, sz.z);
    float3 g3 = to_float3(gx.w, gy.w, sz.w);
    float4 w = to_float4_s(0.5f) - to_float4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3));
    w = _fmaxf(w, to_float4_s(0.0f));
    float4 w2 = w * w;
    float4 w3 = w2 * w;
    float4 gdotx = to_float4(dot(g0, x0), dot(g1, x1), dot(g2, x2), dot(g3, x3));
    float4 dw = -6.0f * w2 * gdotx;
    float3 dn0 = w3.x * g0 + dw.x * x0;
    float3 dn1 = w3.y * g1 + dw.y * x1;
    float3 dn2 = w3.z * g2 + dw.z * x2;
    float3 dn3 = w3.w * g3 + dw.w * x3;
    return 39.5f * (dn0 + dn1 + dn2 + dn3);
}

// Chris Wellons' and TheIronBorn's best 32-bit two-round integer hash
// https://github.com/skeeto/hash-prospector
// flockaroo used 2D noise, but it was only sampled along one dimension
// and only used one output value, so I replaced it with 1D noise
__DEVICE__ float hash32(int x) {
    const float imf = 1.0f / (float)(0xFFFFFFFFU);
   
    // since the frame number is an int seed, convert to uint for bitwise ops
    uint p = (uint)(x);
    p ^= p >> 16;
    p *= 0x21F0AAADU;
    p ^= p >> 15;
    p *= 0xD35A2D97U;
    p ^= p >> 15;
    // normalize float and shift range to -0.5f, 0.5f to cover whole period with ang
    return (float)(p) * imf - 0.5f;
}

__DEVICE__ float circleSDF(float2 p, float2 c, float r) {
    return length(p + c) - r;
}

// basically tried to eliminate as many dot products and divisions as possible
__DEVICE__ float getRot(float2 invRes, float2 pos, float2 b, float idb, mat2 m, __TEXTURE2D__ iChannel0) {
    
    const float rotFloat = (float)(RotNum);
    const float iRotFloat = 1.0f / rotFloat;
    
    float2 p = b;
    float rot = 0.0f;
    for(int i = 0; i < RotNum; i++) {
        rot += dot(swi2(texture(iChannel0, fract_f2((pos + p) * invRes)),x,y) - 0.5f, to_float2(p.y, -p.x));
        p = mul_mat2_f2(m , p);
    }
    return rot * iRotFloat * idb;
}

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R, float alphathres)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > alphathres)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}



__KERNEL__ void SinglepassfluidsimJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, float4 iDate, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_CHECKBOX0(KEY_I, 0);
    
    CONNECT_COLOR0(ColorMouse, 0.0f, 0.0f, 0.0f, 0.0f);
    CONNECT_SLIDER0(MouseSize, -0.1f, 1.0f, 0.0f);
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);

    CONNECT_SLIDER5(AlphaThres, 0.0f, 1.0f, 0.0f);

    fragCoord+=0.5f;

    const float ang = 2.0f * 3.1415926535f / (float)(RotNum);
    mat2 m = to_mat2( _cosf(ang), _sinf(ang),-_sinf(ang), _cosf(ang));

    // use screen resolution in all calculations
    // since buffer and image resolution are the same
    
    // eliminate division where possible
    float2 invRes = 1.0f / iResolution;
    
    // proportionally squared and zero-centered uvs
    // to keep simplex seed color grid coords from stretching
    float2 uv = (fragCoord - 0.5f * iResolution) * invRes.y;
    float rnd = hash32(iFrame);
    
    // do ang * rnd once instead of twice
    float angrnd = ang * rnd;
    float2 b = to_float2(_cosf(angrnd), _sinf(angrnd));
    
    // calculate dot product and its inverse only once
    float db = dot(b, b);
    float idb = 1.0f / db;
    float2 v = to_float2_s(0.0f);
    
    // abort loop later for bigger vortices (for long-term stability)
    // this makes it less dependent on certain resolutions for stability
    float bbMax = iResolution.y;
    bbMax *= bbMax;
    
    // reduced the number of rounds the velocity is summed over
    for(int l = 0; l < 9; l++) {
        if(db > bbMax) break;
        float2 p = b;
        for(int i = 0; i < RotNum; i++) {
            v += swi2(p,y,x) * getRot(invRes, fragCoord + p, b, idb,m,iChannel0);
            p = mul_mat2_f2(m , p);
        }
        b *= 2.0f;
        // do multiplications for dot product optimizations
        db *= 4.0f;
        idb *= 0.25f;
    }
    
    fragColor=texture(iChannel0,fract_f2((fragCoord + v * to_float2(-1, 1) * 2.0f) / iResolution));

    float2 scr = fragCoord * invRes - 0.5f;
    // added an extra blue color "fountain"
    // since velocity is driven by red and green channels, blue is initially stationary (like black)
    // but starts moving when red and green accumulate in the blue pixels
    // slowed down color accumulation a bit to accomodate for an extra channel being used
    swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + (0.009f * to_float3_aw(swi2(scr,x,y), -0.5f * (scr.x + scr.y)) / (dot(scr, scr) * 10.0f + 0.3f)));
    
    
    
    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/R, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R, AlphaThres);
    
    
    if(iFrame < 1 || KEY_I) {
        float srv = iTime;//iDate.x + iDate.y + iDate.z + iDate.w; 
        float3 n = psrdnoise(to_float3_aw(uv * 4.0f, srv));
        n = normalize(n) * 0.5f + 0.5f;
        fragColor = to_float4_aw(n, 1.0f);
    }
    
    if(iMouse.z > 0.0f) {
        float t = iTime;
        float2 ppos = -1.0f*(swi2(iMouse,x,y) - 0.5f * iResolution) * invRes.y;
        float3 pcol = 0.5f * (sin_f3(to_float3(t, t + 2.1f, t + 4.2f)) + 1.0f);
        pcol = _mix(pcol,swi3(ColorMouse,x,y,z),ColorMouse.w);
        float dist = circleSDF(uv, ppos, 0.1f+MouseSize);
        if(dist < 0.0f) fragColor = to_float4_aw(pcol, 1.0f);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Based on "spilled" by florian berger (flockaroo) https://www.shadertoy.com/view/MsGSRd
// Click and drag to inject color
// Press I to pick a different start seed

__KERNEL__ void SinglepassfluidsimJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float4 iDate, int iFrame, float4 iMouse, sampler2D iChannel0)
{
  float2 uv = fragCoord / iResolution;
  fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);

  SetFragmentShaderComputedColor(fragColor);
}
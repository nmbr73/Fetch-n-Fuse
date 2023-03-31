
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel0
// Connect Image 'Texture: Blue Noise' to iChannel1


#define AA 1

#define pi  3.14159265358979323f

__DEVICE__ float pieceDist(float3 p, float th, int n, float r, float rs, float iTime)
{
    float2 delta = to_float2(_sinf(th), _cosf(th));
    float y = 0.0f;
    float d = dot(to_float2(delta.y, -delta.x), to_float2(length(swi2(p,x,z)) - r, p.y - y));
    float r0 = 0.0f;
    float l = length(swi2(p,x,z));
    for(int i = 0; i < n; ++i)
    {
        r0 = (0.07f + _cosf((float)(i + n) + iTime / 2.0f) * 0.04f) * rs;
        y += delta.y * r0;
        r += delta.x * r0;
        float td = length(to_float2(l - r, p.y - y)) - r0;
        if((i & 1) == 0)
          d = _fminf(d, td);
        else
          d = _fmaxf(d, -td);
        y += delta.y * r0;
        r += delta.x * r0;
    }
    return _fmaxf(d, p.y - y);
}

__DEVICE__ float4 piece(float3 p, float2 org, float th, int n, float r, float rs, float iTime)
{
    return to_float4(org.x, org.y, pieceDist(p - to_float3(org.x, 0, org.y), th, n, r, rs,iTime), r);
}

__DEVICE__ float4 u(float4 a, float4 b)
{
    return a.z < b.z ? a : b;
}

__DEVICE__ float4 scene(float3 p, float iTime)
{
    float4 res = to_float4(0, 0, 1e4, 0);
    res = u(res, piece(p, to_float2_s(0), -0.2f, 13, 0.5f, 1.0f,iTime));
    res = u(res, piece(p, to_float2(1.5f, 0), -0.0f,9, 0.2f, 1.0f,iTime));
    res = u(res, piece(p, to_float2(-0.7f, -0.9f), -0.0f, 8, 0.3f, 1.3f,iTime));
    res = u(res, piece(p, to_float2(-1.5f, 0.1f), -0.5f, 5, 0.8f, 2.0f,iTime));
    res = u(res, piece(p, to_float2(0.5f, 0.7f), -0.05f, 12, 0.2f, 1.0f,iTime));
    res.z = _fminf(res.z, p.y);
    return res;
}

__DEVICE__ float map(float3 p, float iTime)
{
    return scene(p,iTime).z;
}

// Soft shadow for SDF, from IQ and Sebastian Aaltonen:
// https://www.shadertoy.com/view/lsKcDD
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float mint, in float tmax, int technique, float s, float iTime )
{
    float res = 1.0f;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration

    for( int i=0; i<55; i++ )
    {
        float h = map( ro + rd*t, iTime );

        // traditional technique
        if( technique==0 )
        {
            res = _fminf( res, s*h/t );
        }
        // improved technique
        else
        {
            // use this if you are getting artifact on the first iteration, or unroll the
            // first iteration out of the loop
            //float y = (i==0) ? 0.0f : h*h/(2.0f*ph); 

            float y = h*h/(2.0f*ph);
            float d = _sqrtf(h*h-y*y);
            res = _fminf( res, s*d/_fmaxf(0.0f,t-y) );
            ph = h;
        }

        t += h;

        if( res<0.0001f || t>tmax ) break;

    }
    return clamp( res, 0.0f, 1.0f );
}

// Forward-difference SDF gradients.
__DEVICE__ float3 distG(float3 p, float iTime)
{
    float2 e = to_float2(1e-4, 0);
    return to_float3(map(p + swi3(e,x,y,y),iTime), map(p + swi3(e,y,x,y),iTime), map(p + swi3(e,y,y,x),iTime)) -
        to_float3(map(p - swi3(e,x,y,y),iTime), map(p - swi3(e,y,x,y),iTime), map(p - swi3(e,y,y,x),iTime));
}

__DEVICE__ void render( out float4 *fragColor, in float2 fragCoord, float2 iResolution, float iTime, __TEXTURE2D__ iChannel0, float4 ColColor, float3 Look )
{
  float2 uv = fragCoord / iResolution * 2.0f - 1.0f;
  uv.x *= iResolution.x / iResolution.y;
    
  float3 ro = to_float3(-0.3f, 0.8f, 4.2f), rd = normalize(to_float3_aw(uv, -3.0f)+Look);
    
    float t = 2.5f;
    for(int i = 0; i < 110; ++i)
    {
        float d = map(ro + rd * t,iTime);
        if(_fabs(d) < 1e-4)
            break;
        if(t > 10.0f)
            break;
        t += d;
    }
    
    float3 rp = ro + rd * t;
    
    float3 n = normalize(distG(ro + rd * t,iTime));
    float3 r = reflect(rd, n);
    float3 ld = normalize(to_float3(-1, 1, 1));
    float sh = calcSoftshadow(ro + rd * t, ld, 1e-2, 1e3, 0, 2.0f,iTime);
    float sh2 = calcSoftshadow(ro + rd * t, r, 1e-2, 1e3, 0, 10.0f,iTime);
    
    //float3 diff = 0.5f + 0.5f * cos_f3(rp.y * to_float3(3, 2, 5) * 0.5f + to_float3(0.6f, 0, 0.6f));
    float3 diff = 0.5f + 0.5f * cos_f3(rp.y * swi3(ColColor,x,y,z) + to_float3(0.6f, 0, 0.6f));
    
    float4 sp = scene(rp,iTime);
    diff = _mix(to_float3_s(1), diff, smoothstep(0.1f, 0.12f,_fabs(fract(0.1f + _atan2f(rp.z - sp.y, rp.x - sp.x) / pi * 5.0f) - 0.5f)));
    
    if(_fabs(rp.y) < 1e-2 || t > 9.0f)
        diff = to_float3(0.5f, 0.75f, 1.0f) * smoothstep(-0.1f, 0.15f, distance_f2(swi2(rp,x,z), swi2(sp,x,y)) - sp.w);
    
    //swi3(fragColor,x,y,z) = diff;
//    (*fragColor).x=diff.x;
//    (*fragColor).y=diff.y;
//    (*fragColor).z=diff.z;
    
    //swi3S(*fragColor,x,y,z, swi3(*fragColor,x,y,z) * _mix(0.5f, 1.0f, sh) * to_float3_s(_fmaxf(0.0f, 0.6f + 0.4f * dot(n, ld))));
    diff *= _mix(0.5f, 1.0f, sh) * to_float3_s(_fmaxf(0.0f, 0.6f + 0.4f * dot(n, ld)));
    
    float fr = _powf(clamp(1.0f - dot(n, -rd), 0.0f, 1.0f), 2.0f);
    
//    swi3(*fragColor,x,y,z) += swi3(decube_f3(iChannel0, r),x,x,x) * fr * sh2;
    diff += swi3(decube_f3(iChannel0, r),x,x,x) * fr * sh2;

//    swi3(*fragColor,x,y,z) += smoothstep(0.4f, 0.5f, dot(ld, r)) * fr * sh2 * 1.6f;
    diff += smoothstep(0.4f, 0.5f, dot(ld, r)) * fr * sh2 * 1.6f;
    
    //swi3(*fragColor,x,y,z) *= 0.85f;

    *fragColor = to_float4_aw(diff * 0.85f, (*fragColor).w);
}

__KERNEL__ void UndulatingFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

  CONNECT_COLOR0(ColColor, 1.5f, 1.0f, 2.5f, 1.0f);
  CONNECT_POINT0(Look, 0.0f, 0.0f);
  CONNECT_SLIDER0(LookZ, -10.0f, 10.0f, 0.0f);

    //fragColor.w = 1.0f;
    //swi3(fragColor,x,y,z) = to_float3_s(0);
    //fragColor = to_float4(0.0f,0.0f,0.0f,1.0f);
    
    float3 Color = to_float3_s(0);
    
    // Anti-aliasing loop
    for(int y = 0; y < AA; ++y)
        for(int x = 0; x < AA; ++x)
        {
            float4 rc;
            render(&rc, fragCoord + to_float2(x, y) / (float)(AA), iResolution,iTime,iChannel0, ColColor, to_float3_aw(Look,LookZ) );
            //swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + clamp(swi3(rc,x,y,z), 0.0f, 1.0f));
            Color += clamp(swi3(rc,x,y,z), 0.0f, 1.0f);
        }

    //swi3(fragColor,x,y,z) /= float(AA * AA);
    Color /= (float)(AA * AA);
    //swi3(fragColor,x,y,z) /= (swi3(fragColor,x,y,z) + 1.5f)*0.43f;
    Color /= (swi3(fragColor,x,y,z) + 1.5f)*0.43f;
    //swi3(fragColor,x,y,z) = _powf(clamp(swi3(fragColor,x,y,z), 0.0f, 1.0f), to_float3_aw(1.0f / 2.2f)) + swi3(texelFetch(iChannel1, to_int2(fragCoord) & 1023, 0),x,y,z) / 200.0f;
    Color = pow_f3(clamp(Color, 0.0f, 1.0f), to_float3_s(1.0f / 2.2f)) + swi3(_tex2DVecN(iChannel1, ((float)((int)(fragCoord.x) & 1023)+0.5f)/iResolution.x ,((float)((int)(fragCoord.y) & 1023)+0.5f)/iResolution.y, 15),x,y,z) / 200.0f;

  fragColor = to_float4_aw(Color,1.0f); 

  SetFragmentShaderComputedColor(fragColor);
}
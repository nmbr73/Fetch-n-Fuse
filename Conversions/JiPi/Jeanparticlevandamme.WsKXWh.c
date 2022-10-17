
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// https://www.shadertoy.com/view/XsXGDM

#define BIAS  4.0f
#define LUMIN to_float3(0.2126f, 0.7152f, 0.0722f)

__DEVICE__ float mask(in float4 fg)
{
  float sf = _fmaxf(fg.x, fg.z);
  float k = clamp((fg.y - sf) * BIAS, 0.0f, 1.0f);
  
  if (fg.y > sf) fg = to_float4_s(dot(LUMIN, swi3(fg,x,y,z)));
  
  return smoothstep(0.3f, 0.0f, (fg * k).x);    
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel1
// Connect Buffer A 'Texture: Video' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void JeanparticlevandammeFuse__Buffer_A(float4 fragColor, float2 P, float2 iResolution, float iTime, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_SLIDER0(TimeDelta, -1.0f, 1.0f, 0.0f);

    iTimeDelta += TimeDelta;
    
    P+=0.5f; 
    
    fragColor = to_float4_s(0.0f);
    
    if(iFrame < 5) 
    {
      SetFragmentShaderComputedColor(fragColor);
      return;
    }
    
    float2 uv = P/iResolution;
    float4 rgba  = _tex2DVecN(iChannel2,uv.x,uv.y,15);
    
    float ba= smoothstep(0.49f, 0.5f, _fabs(uv.x - 0.5f));
    float bb= smoothstep(0.49f, 0.5f, _fabs(uv.y - 0.5f));

    if(mask(rgba) > 0.0f)
    {
        fragColor = to_float4_s(0.0f);
        swi2S(fragColor,x,y, swi2(texture(iChannel1, uv + iTime),x,y));//.rg;
        fragColor.x -= 0.5f;
        fragColor.y *= 0.25f;
        swi2S(fragColor,z,w, sign_f2(swi2(fragColor,x,y)) * 1.1f);
    }
    else
    {
           
        float4 ct = texture(iChannel0, (make_float2(to_int2_cfloat(P) + to_int2( 0.0f,  1.0f))+0.5f)/R);
        float4 cr = texture(iChannel0, (make_float2(to_int2_cfloat(P) + to_int2( 1.0f,  0.0f))+0.5f)/R);
        float4 cl = texture(iChannel0, (make_float2(to_int2_cfloat(P) + to_int2(-1.0f,  0.0f))+0.5f)/R);
        float4 cb = texture(iChannel0, (make_float2(to_int2_cfloat(P) + to_int2( 0.0f, -1.0f))+0.5f)/R);

        float4 tr = texture(iChannel0, (make_float2(to_int2_cfloat(P) + to_int2(  1.0f,  1.0f))+0.5f)/R);
        float4 tl = texture(iChannel0, (make_float2(to_int2_cfloat(P) + to_int2(  1.0f, -1.0f))+0.5f)/R);
        
        float4 br = texture(iChannel0, (make_float2(to_int2_cfloat(P) + to_int2(  1.0f, -1.0f))+0.5f)/R);
        float4 bl = texture(iChannel0, (make_float2(to_int2_cfloat(P) + to_int2( -1.0f, -1.0f))+0.5f)/R);
        
        float4 c = texture(iChannel0, (make_float2(to_int2_cfloat(P))+0.5f)/R);
        float4 incb = to_float4_s(0.0f);

        // incoming from topright
        if(tr.z <= -1.0f && tr.w <= -1.0f)
        {
            //swi2(tr,z,w) -= to_float2(-1.0f, -1.0f);
            tr.z -=  -1.0f;
            tr.w -=  -1.0f;
            incb = tr;
        }

        // incoming from topleft
        else if(tl.z >= 1.0f && tl.w <= -1.0f)
        {
            //swi2(tl,z,w) -= to_float2(1.0f, -1.0f);
            tr.z -=  -1.0f;
            tr.w -=  -1.0f;
            incb = tl;
        }
        
        // incoming from bottomright
        if(br.z <= -1.0f && br.w >= 1.0f)
        {
            //swi2(br,z,w) -= to_float2(-1.0f, 1.0f);
            tr.z -=  -1.0f;
            tr.w -=  1.0f;
            incb = br;
        }

        // incoming from bottomleft
        else if(bl.z >= 1.0f && bl.w >= 1.0f)
        {
            //swi2(bl,z,w) -= to_float2(1.0f, 1.0f);
            tr.z -=  1.0f;
            tr.w -=  1.0f;
            incb = bl;
        }
        
        
        // incoming from bottom
        else if(cb.w >= 1.0f && _fabs(cb.z) < 1.0f)
        {
            cb.w -= 1.0f;
            incb = cb;
        }

        // incoming from top
        else if(ct.w <= -1.0f && _fabs(ct.z) < 1.0f)
        {
            ct.w += 1.0f;
            incb = ct;
        }
        
        // incoming from left
        else if(_fabs(cl.w) < 1.0f && cl.z >= 1.0f)
        {
            cl.z -= 1.0f;
            incb = cl;
        }

        // incoming from right
        else if(_fabs(cr.w) < 1.0f && cr.z <= -1.0f)
        {
            cr.z += 1.0f;
            incb = cr;
        }
        
        
        // delete from this cell if moved out
        if(_fabs(c.z) > 1.0f || _fabs(c.w) > 1.0f) c = to_float4_s(0.0f);
        
                
        // add incoming one
        c += incb;
        
        // apply velocity
        swi2S(c,z,w, swi2(c,z,w) + swi2(c,x,y) * iTimeDelta* 50.0f);
        if(dot(c, c) > 0.0f && bb <= 0.0f && ba <= 0.0f)
        {
            c.y -= iTimeDelta * iTimeDelta * 25.0f;
        }
        
        //swi2(c,x,y) *= 0.98f;
        c.x *= 0.98f;
        c.y *= 0.98f;
        fragColor = c;
    }

    // vignette
    float2 edgeDir = (-1.0f*(uv - 0.5f));
    edgeDir.y += _sinf(uv.x*220.0f + iTime*2.1f);
    edgeDir.x += _cosf(uv.y*220.0f + iTime*2.1f);
    swi2S(fragColor,x,y, swi2(fragColor,x,y) + _fminf(1.0f, ba + bb)*edgeDir*0.005f);

  SetFragmentShaderComputedColor(fragColor);
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel1
// Connect Image 'Texture: Video' to iChannel2
// Connect Image 'Previsualization: Buffer A' to iChannel0



__KERNEL__ void JeanparticlevandammeFuse(float4 fragColor, float2 P, float2 iResolution, sampler2D iChannel0, sampler2D iChannel2)
{
    P+=0.5f;

    float2 uv = P/iResolution;
    
    float Mask = mask(_tex2DVecN(iChannel2,uv.x,uv.y,15));
    float4 c = texture(iChannel0, (make_float2(to_int2_cfloat(P))+0.5f)/R);
    float4 col = (_tex2DVecN(iChannel2,uv.x,uv.y,15))*Mask;
    col *= 1.0f-Mask;
    float4 col1 = to_float4(1.0f, 0.0f, 0.0f, 0.0f);
    float4 col2 = to_float4(1.0f, 1.0f, 1.0f, 0.0f);
    float4 col3 = to_float4(0.2f, 0.8f, 1.0f, 0.0f);
    col1 = _mix(col1, col3, clamp(c.y*1.5f, 0.0f, 1.0f));
    float ln = _fminf(length(swi2(c,x,y))*0.6f, 1.0f);
    fragColor = col + ln * (1.0f-Mask) * _mix(col1, col2, ln);
    float edge = 0.55f-_fmaxf(_fabs(uv.x - 0.5f), _fabs(uv.y - 0.5f));
    fragColor *=  1.0f+smoothstep(0.41f, 0.5f, 0.49f-edge)*155.0f*to_float4(0.2f, 0.9f, 0.6f, 0.0f);
    fragColor = _fminf(fragColor, to_float4_s(1.0f));
    float4 bg = to_float4(0.02f, 0.0f, uv.y * 0.07f, 0.0f)*0.1f;
    fragColor += bg;
    fragColor = sqrt_f4(fragColor);

  SetFragmentShaderComputedColor(fragColor);
}
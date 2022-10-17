
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define PI _acosf(-1.0f)


__DEVICE__ uint decodeAscii(float b_z) {
    return uint(fract(b_z)*256.0f);
}
__DEVICE__ float decodeColor(float b_z) {
    return fract(b_z*256.0f);
}
__DEVICE__ float decodeSize(float b_z) {
    return _floor(b_z);
}
__DEVICE__ float ln (float2 p, float2 a, float2 b) {return length(p-a-(b-a)*clamp(dot(p-a,b-a)/dot(b-a,b-a),0.0f,1.0f));}


//---------------------------------------------------------------------------------

__DEVICE__ float linear_srgb(float _x) {
    return _mix(1.055f*_powf(_x, 1.0f/2.4f) - 0.055f, 12.92f*_x, step(_x,0.0031308f));
}
__DEVICE__ float3 linear_srgb(float3 _x) {
    return mix_f3(1.055f*pow_f3(_x, to_float3_s(1.0f/2.4f)) - 0.055f, 12.92f*_x, step(_x,to_float3_s(0.0031308f)));
}

__DEVICE__ float srgb_linear(float _x) {
    return _mix(_powf((_x + 0.055f)/1.055f,2.4f), _x / 12.92f, step(_x,0.04045f));
}
__DEVICE__ float3 srgb_linear(float3 _x) {
    return mix_f3(pow_f3((_x + 0.055f)/1.055f,to_float3_s(2.4f)), _x / 12.92f, step(_x,to_float3_s(0.04045f)));
}

//---------------------------------------------------------------------------------


// https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * 443.8975f);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float2 pModPolar(float2 p, float repetitions) {
  float angle = 2.0f*PI/repetitions;
  float a = _atan2f(p.y, p.x) + angle/2.0f;
  float r = length(p);
  a = mod_f(a,angle) - angle/2.0f;
  p = to_float2(_cosf(a), _sinf(a))*r;
  return p;
}
__DEVICE__ float vmax(float2 v) {
  return _fmaxf(v.x, v.y);
}
__DEVICE__ float vmin(float2 v) {
  return _fminf(v.x, v.y);
}
__DEVICE__ float fBox(float2 p, float2 b) {
  float2 d = abs_f2(p) - b;
  return length(_fmaxf(d, to_float2_s(0))) + vmax(_fminf(d, to_float2_s(0)));
}

__DEVICE__ float StarPolygon(float2 p, float repetitions, float radius, float inner) {
    float angle = PI/repetitions;
    float2 p2 = pModPolar(swi2(p,y,x), repetitions);
    float _x = _fabs(p2.y);
    float _y = p2.x - radius;
    float offset = (PI*0.5f - angle)*inner;
    float uvRotation = angle + offset;
    float2 uv = _cosf(uvRotation)*to_float2(_x, _y) + _sinf(uvRotation)*to_float2(-_y, _x);
    
    float corner = radius*_sinf(angle)/_cosf(offset);
    float li = length(to_float2(_fmaxf(uv.x - corner, 0.0f), uv.y));
    float lo = length(to_float2(_fminf(uv.x, 0.0f), uv.y));
    return _mix(-li, lo, step(0.0f, uv.y));
}
__DEVICE__ float something(float2 u, float z) {
    float a = 36.4f*z;
    u = abs_f2(u)-0.25f*z;
    u = mul_f2_mat2(u, to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)));
    u = abs_f2(u)-z*to_float2(0.3f,1);
    return _fmaxf(u.x,u.y);
}

__DEVICE__ float starPolys(float2 u, float4 b) {
    float df = StarPolygon(u,3.0f+_floor(4.0f*fract(b.z*1.601f)),b.z,fract(b.z*2.601f));
    return df;
}
__DEVICE__ float almostIdentityLo(float _x, float m) {
    return (_x >= m) ? _x : (-(1.0f / 3.0f) * (1.0f / m) * (1.0f / m) * _x + (1.0f / m)) * _x * _x + (1.0f / 3.0f) * m;
}
__DEVICE__ float4 sampleCharacter(uint ch, float2 chUV, float2 R, __TEXTURE2D__ sampler) {
    
    uint2 chPos = make_uint2(ch % 16u, ch / 16u);
    float2 cchUV = clamp(chUV, to_float2_s(0.0078125f), to_float2_s(0.9921875f));
    cchUV = 0.5f+(0.5f-0.0078125f)*(chUV-0.5f)/_fmaxf(0.5f-0.0078125f,_fmaxf(_fabs(chUV.x-0.5f),_fabs(chUV.y-0.5f)));
    float2 uv = (make_float2(chPos) + cchUV) / 16.0f;

    float l = distance_f2(cchUV, chUV);
    l = fBox(chUV-0.5f,to_float2_s(0.5f-0.0078125f))-0.0f-0.0078125f*0.0f;
    float4 s = texture(sampler,uv);
    //s.gb = s.gb*2.0f- 1.0f;
    s.y = s.y*2.0f- 1.0f;
    s.z = s.z*2.0f- 1.0f;
    
    s.z = -s.z; // texture sampler is VFlipped
    s.w = s.w- 0.5f+ s.x/ 256.0f;
    //s.w = s.w+ (l >= 0.0f ? 1.0f : _fmaxf(0.0f,s.w*1.0f-0.5f)) * l;
    s.w = s.w + l * _fmaxf(step(0.0f,l),s.w);
    return s;
}
__DEVICE__ float2 rot(float2 v, float a) {
    return mul_f2_mat2(v , to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a)));
}
__DEVICE__ mat3 rotAA(float3 u, float a) {
    float c = _cosf(a), s = _sinf(a), o = 1.0f-c;
    return to_mat3(
        u.x*u.x*o+1.0f*c, u.x*u.y*o-u.z*s, u.x*u.z*o+u.y*s,
        u.y*u.x*o+u.z*s, u.y*u.y*o+1.0f*c, u.y*u.z*o-u.x*s,
        u.z*u.x*o-u.y*s, u.z*u.y*o+u.x*s, u.z*u.z*o+1.0f*c);
}
__DEVICE__ float4 alphabet(float2 u, float4 b, float2 R, __TEXTURE2D__ sampler) {
    uint id = decodeAscii(b.z);
    if(id == 208u || id == 80u || id == 93u || id == 143u) id = 252u;
    //else if(id == 0u) id = 80u;
    //else if(id >= 143u) id = 0u;
    float sizev = 16.0f+ 2.5f* b.z;
    float4 df = sampleCharacter(id,u/sizev+ 0.5f,R, sampler)*sizev;
    //df.x -= _fmaxf(0.125f-df.w,0.0f);
    //df.w *= sizev;
    swi2S(df,y,z, rot(swi2(df,y,z), -b.w));
    return swi4(df,w,y,z,x);
}
__DEVICE__ float2 distRot(float2 U, float4 b) {
    return rot(U-swi2(b,x,y),b.w);
}
//#define distN(U,b) to_float3_aw(something(distRot(U,b), b.z),0,0);
//#define distN(U,b) to_float3_aw(starPolys(distRot(U,b), b),0,0);
#define distN(U,b) alphabet(distRot(U,b),b,R,iChannel3)
#define dist(U,b) distN(U,b).x

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel2
// Connect Buffer A 'Texture: Font 1' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


__DEVICE__ float4 A(float2 U, float2 R, __TEXTURE2D__ iChannel0) {return texture(iChannel0,U/R);}
__DEVICE__ float4 B(float2 U, float2 R, __TEXTURE2D__ iChannel1) {return texture(iChannel1,U/R);}
__DEVICE__ float4 K(float2 U, __TEXTURE2D__ iChannel2) {return texture(iChannel2,(U+0.5f)/to_float2(256,3));}
__DEVICE__ float4 F(float2 U, __TEXTURE2D__ iChannel3) {return texture(iChannel3,U);}

__DEVICE__ float4 T (float2 U, float2 R, __TEXTURE2D__ iChannel0) {
  return A(U-swi2(A(U,R,iChannel0),x,y),R,iChannel0);
}
__KERNEL__ void StopPollutingFuse__Buffer_A(float4 Q, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    U+=0.5f;

    float4 mo = A(to_float2_s(0),R,iChannel0);
    if(U.x < 1.0f && U.y < 1.0f) {
        if (iMouse.z > 0.0f) {
            if (mo.z > 0.0f) {
                Q = to_float4(iMouse.x/R.x,iMouse.y/R.y, mo.x,mo.y);
            } else {
                Q =  to_float4(iMouse.x/R.x,iMouse.y/R.y,iMouse.x/R.x, iMouse.y/R.y);
            }
        } else {
            Q = to_float4_s(0.0f);
        }
        SetFragmentShaderComputedColor(Q);
        return;
    }
    Q = T(U,R,iChannel0);
    float4 b = B(U,R,iChannel1);
    float4 q = T(swi2(b,x,y),R,iChannel0);
    
        
    float p = smoothstep(2.0f,0.0f,dist(U,b));
    float2 r = normalize(U-swi2(b,x,y));
    //vec2 k = to_float2(-r.y,r.x);
    float o = 1.0f+2.0f*p;
    float4 
        n = T(U+to_float2(0,o),R,iChannel0),
        e = T(U+to_float2(o,0),R,iChannel0),
        s = T(U-to_float2(0,o),R,iChannel0),
        w = T(U-to_float2(o,0),R,iChannel0),
        m = n+e+s+w;
    Q.x -= 0.25f*(e.z-w.z+Q.w*(n.w-s.w));
    Q.y -= 0.25f*(n.z-s.z+Q.w*(e.w-w.w));
    Q.z  = 0.25f*((s.y-n.y+w.x-e.x)+m.z);
    Q.w  = 0.25f*((n.x-s.x+w.y-e.y)-Q.w);
    
    swi2S(Q,x,y, swi2(Q,x,y) + p*(0.25f*swi2(m,x,y)-swi2(Q,x,y)));
    Q.z += 0.05f*p;
    Q.z*=0.975f;
    //if (mo.z > 0.0f && swi2(mo,x,y) != swi2(mo,z,w)) {
    if (mo.z > 0.0f && (mo.x != mo.z || mo.y != mo.w)) {
        float l = ln(U, swi2(mo,x,y)*R, swi2(mo,z,w)*R);
        swi2S(Q,x,y, swi2(Q,x,y) + 0.006f*(swi2(mo,x,y)*R - swi2(mo,z,w)*R)*smoothstep(40.0f,0.0f,l));
    }
    
    if (R.x-U.x < 5.0f) Q.x = -_fabs(Q.x)*0.999f;
    if (R.y-U.y < 5.0f) Q.y = -_fabs(Q.y)*0.999f;
    if (U.x < 5.0f)     Q.x = _fabs(Q.x)*0.999f;
    if (U.y < 5.0f)     Q.y = _fabs(Q.y)*0.999f;
    if(mod_f((float)(iFrame), 60.0f) == 1.0f && (R.x-U.x < 1.0f && R.y < U.y+U.y || U.x < 1.0f && R.y >= U.y+U.y)) {
        swi2S(Q,x,y, to_float2(0.6f*(1.0f-2.0f*smoothstep(0.45f,0.55f,U.y/R.y)),0.0f));
    }
    
    //if (R.x-U.x<1.0f) Q.xy=to_float2(-0.3f,0.0f);
    
    //if (R.x-U.x < 5.||U.y < 5.||R.y-U.y<5.0f) swi2(Q,x,y) *= 0.5f;
    
    if (iFrame < 1 || K(to_float2(32,1),iChannel2).x > 0.1f) {
        Q = to_float4(0.3f-0.6f*smoothstep(0.45f,0.55f,U.y/R.y),0,0,0)*0.0f;
    }

    if (iFrame < 240) {
        swi2S(Q,x,y, swi2(Q,x,y) * smoothstep(200.0f,300.0f,clamp((float)(240 - iFrame)/150.0f-1.0f,0.0f,1.0f)*100.0f + distance_f2(R*0.5f,U)));
    }

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Preset: Keyboard' to iChannel2
// Connect Buffer B 'Texture: Font 1' to iChannel3
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1



__DEVICE__ void swap (float2 U, inout float4 *A, float4 B, float2 R, __TEXTURE2D__ iChannel3) {
    
    float tempA = dist(U,B);
    float tempB = dist(U,*A);
    //if (A.z == 0.0f || !(dist(U,B) >= dist(U,*A))) *A = B;
    if ((*A).z == 0.0f || !(tempA >= tempB)) *A = B;
    }
    
__DEVICE__ float ascii2id(float Q_z) {return
// ((15.0f-_floor(Q_z/16.0f))*16.0f+(15.0f-mod_f(Q_z,16.0f))*1.0f )
// ((    _floor(Q_z/16.0f))*16.0f+(15.0f-mod_f(Q_z,16.0f))*1.0f )
// ((15.0f-_floor(Q_z/16.0f))*1.0f +(15.0f-mod_f(Q_z,16.0f))*16.0f)
// ((    _floor(Q_z/16.0f))*1.0f +(15.0f-mod_f(Q_z,16.0f))*16.0f)
   ((15.0f-_floor(Q_z/16.0f))*16.0f+(    mod_f(Q_z,16.0f))*1.0f )
// ((    _floor(Q_z/16.0f))*16.0f+(    mod_f(Q_z,16.0f))*1.0f )
// ((15.0f-_floor(Q_z/16.0f))*1.0f +(    mod_f(Q_z,16.0f))*16.0f)
// ((    _floor(Q_z/16.0f))*1.0f +(    mod_f(Q_z,16.0f))*16.0f)
;
}
__DEVICE__ float uv2color(float2 rnd, int iFrame, float iTime) {
    return mod_f(rnd.x*0.5f+rnd.y*2.0f+(float)(iFrame % 1024)+iTime,32.0f)/32.0f;
}
__DEVICE__ float encodeSizeAsciiColor(float size, float ascii, float color) {
    return size+(ascii2id(ascii)+color)/256.0f;
}
    
__KERNEL__ void StopPollutingFuse__Buffer_B(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, int iFrame, float4 iDate, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    U+=0.5f;
     
    Q = B(U,R,iChannel1);

    swap(U,&Q,B(U+to_float2(8,8),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U+to_float2(8,-8),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U-to_float2(8,8),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U-to_float2(8,-8),R,iChannel1),R,iChannel3);

    swap(U,&Q,B(U+to_float2(4,0),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U+to_float2(0,-4),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U-to_float2(4,0),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U-to_float2(0,-4),R,iChannel1),R,iChannel3);

    swap(U,&Q,B(U+to_float2(2,2),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U+to_float2(2,-2),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U-to_float2(2,2),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U-to_float2(2,-2),R,iChannel1),R,iChannel3);

    swap(U,&Q,B(U+to_float2(1,0),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U+to_float2(0,-1),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U-to_float2(1,0),R,iChannel1),R,iChannel3);
    swap(U,&Q,B(U-to_float2(0,-1),R,iChannel1),R,iChannel3);

    swi3S(Q,x,y,w, swi3(Q,x,y,w) + swi3(A(swi2(Q,x,y),R,iChannel0),x,y,w));
    /*
    if((R.x-U.x < 1.0f && R.y < U.y+U.y || U.x < 1.0f && R.y >= U.y+U.y)) {
        float key = -1.0f;
        for(float i = 0.0f; i < 256.0f; i++) {
            if(K(to_float2(i,0)).x > 0.1f) key = i;
        }
        if(key >= 0.0f || mod_f(float(iFrame), 60.0f) == 1.0f) {
            float y = _floor(U.y/32.0f)*32.0f+16.0f;
            float prog = 1.0f+_floor(R.y/(40.0f+iTime*2.0f));
            if(mod_f(_floor(U.y/32.0f),prog) == mod_f(-_floor(float(iFrame)/60.0f),prog)) {
                Q.x = U.x < 1.0f ? -1.0f : R.x;
                Q.y = y;
                Q.z = -2.5f*_logf(1e-5+(0.5f+0.5f*_sinf(y+(y+0.45f)*mod_f(float(iFrame),1e3)))*(0.0625f+0.9375f*smoothstep(120.0f,10.0f,_fminf(100.0f,iTime))));
                Q.w = 0.0f;
                if(key>=0.0f) {
                    Q.z = encodeSizeAsciiColor(_floor(Q.z),key,fract(fract(float(iFrame)/256.0f)+0.0f*Q.z*256.0f));
                }
            }
        }
    }
    */
    if(mod_f((float)(iFrame), 60.0f) == 1.0f && (R.x-U.x < 1.0f && R.y < U.y+U.y || U.x < 1.0f && R.y >= U.y+U.y)) {
        //float y = round((U.y+5.0f)/20.0f)*20.0f-5.0f;
        float _y = _floor(U.y/32.0f)*32.0f+16.0f;
        float prog = 1.0f+_floor(R.y/(40.0f+iTime*2.0f));
        if(mod_f(_floor(U.y/32.0f),prog) == mod_f(-_floor((float)(iFrame)/60.0f),prog)) {
            float key = -1.0f;
            float rnd = hash13(to_float3(U.x-501.61f,_y+101.61f,(float)(iFrame)/60.0f));
            float rnd2 = fract(PI*(PI+rnd));
            float beg = 0.0f;
            float expo = 10.0f-mod_f(_floor((float)(iFrame)/240.0f),10.0f);
            float expoKey = -1.0f;

            float2 seg = to_float2_s(0);
            seg = to_float2(  0,  3);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(49,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2(  3,  2);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(50,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2(  5,  4);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(51,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2(  9,  7);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(52,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2( 16, 12);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(53,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2( 28,  1);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(54,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2( 29,  1);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(55,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2( 30,  2);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(56,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2( 32, 96);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(57,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            seg = to_float2(128,128);expo--;if(expo > 0.0f)expoKey = seg.x+rnd2*seg.y;if(K(to_float2(48,2),iChannel2).x < 0.1f && rnd >= beg/(beg+seg.y)) {key = seg.x+rnd2*seg.y;beg+=seg.y;}
            key = _floor(key);
            float anyKey = -1.0f;
            for(float i = 0.0f; i < 256.0f; i+=1.0f) {
                if(K(to_float2(i,0),iChannel2).x > 0.1f) anyKey = i;
            }
            if(key < 0.0f) {
                key = anyKey;
            }
            if(anyKey < 0.0f) {
              key = _floor(expoKey);
            }
            Q.x = U.x < 1.0f ? -1.0f : R.x;
            Q.y = _y;
            Q.z = -2.5f*_logf(1e-5+(0.5f+0.5f*_sinf(_y+(_y+0.45f)*mod_f((float)(iFrame),1e3)))*(0.0625f+0.9375f*smoothstep(100.0f,10.0f,_fminf(100.0f,iTime))));
                           Q.w = 0.0f;
            if(key>=0.0f) {
                Q.z = encodeSizeAsciiColor(_floor(Q.z),key,fract(fract((float)(iFrame)/256.0f)+0.0f*Q.z*256.0f));
            }
        }
    }
/*
    if (R.x-U.x < 1.0f && mod_f(float(iFrame) , 60.0f) == 1.0f) {
        float y = round((U.y+5.0f)/20.0f)*20.0f-5.0f;
        Q = to_float4(
            R.x,y,
        0.5f+0.5f*_sinf(y+(y+0.45f)*mod_f(float(iFrame),1e3)),0.
       );
       Q.z = -1.5f*_logf(1e-4+Q.z);
    }
    
*/    
    
    if(iFrame < 1 || K(to_float2(32,1),iChannel2).x > 0.1f) {
        float4 q = to_float4(_floor((U.x+2.0f-R.x*0.5f)/4.0f/5.0f),_floor((U.y+2.0f-R.y*0.5f)/4.0f/5.0f),0,0);
        if(q.y == 5.0f) {
            q.z = (q.x == -8.0f) ? 80.0f  :// P
            /* */ (q.x == -6.0f) ? 111.0f :// o
            /* */ (q.x == -4.0f) ? 108.0f :// l
            /* */ (q.x == -2.0f) ? 108.0f :// l
            /* */ (q.x ==  0.0f) ? 117.0f :// u
            /* */ (q.x ==  2.0f) ? 116.0f :// t
            /* */ (q.x ==  4.0f) ? 105.0f :// i
            /* */ (q.x ==  6.0f) ? 110.0f :// n
            /* */ (q.x ==  8.0f) ? 103.0f :// g
            0.0f;
        } else if(q.y == -1.0f) {
            q.z = (q.x == -9.0f) ? 73.0f  :// I
            /* */ (q.x == -7.0f) ? 115.0f :// s
            /* */ (q.x == -3.0f) ? 101.0f :// e
            /* */ (q.x == -1.0f) ? 97.0f  :// a
            /* */ (q.x ==  1.0f) ? 115.0f :// s
            /* */ (q.x ==  3.0f) ? 121.0f :// y
            /* */ (q.x ==  7.0f) ? 97.0f  :// a
            /* */ (q.x ==  9.0f) ? 115.0f :// s
            0.0f;
        } else if(q.y == -5.0f) {
            q.z = (q.x == -11.0f) ? 65.0f  :// A
            /* */ (q.x ==  -9.0f) ? 45.0f  :// -
            /* */ (q.x ==  -7.0f) ? 66.0f  :// B
            /* */ (q.x ==  -5.0f) ? 45.0f  :// -
            /* */ (q.x ==  -3.0f) ? 116.0f :// t
            /* */ (q.x ==  -2.0f) ? 111.0f :// o
            /* */ (q.x ==  -1.0f) ? 115.0f :// s
            /* */ (q.x ==   0.0f) ? 115.0f :// s
            /* */ (q.x ==   2.0f) ? 45.0f  :// -
            /* */ (q.x ==   4.0f) ? 105.0f :// i
            /* */ (q.x ==   5.0f) ? 110.0f :// n
            /* */ (q.x ==   7.0f) ? 45.0f  :// -
            /* */ (q.x ==   9.0f) ? 67.0f  :// C
            /* */ (q.x ==  11.0f) ? 33.0f  :// !
            0.0f;
        } else { q.z = 0.0f; }
        if(q.z > 0.0f) {
            Q = to_float4(q.x*5.0f*4.0f-2.0f+R.x*0.5f,q.y*5.0f*4.0f-2.0f+R.y*0.5f,encodeSizeAsciiColor(30.0f,q.z,uv2color(swi2(q,x,y),iFrame,iTime)),0);
        } else {
            Q = to_float4(-R.x,-R.y,0,0);
        }
    } else if(iMouse.z>0.0f) {
        float2 pa = swi2(iMouse,x,y);
        float2 pb = swi2(A(to_float2_s(0),R,iChannel0),z,w)*R;
        if(ln(U, pa, pb) < 40.0f + distance_f2(pa,pb)*0.5f) {
          Q = to_float4(-R.x,-R.y,0,0);
        }
    }


  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel2
// Connect Image 'Texture: Font 1' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


//////////////////////////////////////////////////
//
// As Easy As ABC! (Stop Polluting!)
//
// by Timo Kinnunen 2019
//
// Based on https://www.shadertoy.com/view/wsjXWh
//
// Use mouse to clear some stuff
// and press space bar to reinitialize
// the screen (useful after going fullscreen)!
//
///////////////////////////////////////////////////


//vec4 F(float2 U) {return _tex2DVecN(iChannel3,U.x,U.y,15);}

__DEVICE__ float ln (float3 p, float3 a, float3 b) {return length(p-a-(b-a)*dot(p-a,b-a)/dot(b-a,b-a));}

// linear white point
//const float W = 11.2f;
#define W  11.2f

// Filmic Reinhard, a simpler tonemapping
// operator with a single coefficient
// regulating the toe size.

// The operator ensures that f(0.5f) = 0.5

// T = 0: no toe, classic Reinhard
//const float T = 0.01f;
#define T  0.01f

__DEVICE__ float filmic_reinhard_curve (float _x) {
    float q = (T + 1.0f)*_x*_x;    
  return q / (q + _x + T);
}

__DEVICE__ float inverse_filmic_reinhard_curve (float _x) {
    float q = -2.0f * (T + 1.0f) * (_x - 1.0f);
    return (_x + _sqrtf(_x*(_x + 2.0f*T*q))) / q;
}

__DEVICE__ float3 filmic_reinhard(float3 _x) {
    float w = filmic_reinhard_curve(W);

    return to_float3(
        filmic_reinhard_curve(_x.x),
        filmic_reinhard_curve(_x.y),
        filmic_reinhard_curve(_x.z)) / w;
}

__DEVICE__ float3 inverse_filmic_reinhard(float3 _x) {
    _x *= filmic_reinhard_curve(W);
    return to_float3(
        inverse_filmic_reinhard_curve(_x.x),
        inverse_filmic_reinhard_curve(_x.y),
        inverse_filmic_reinhard_curve(_x.z));
}

///////////////////////////////////////////////

// ACES fitted
// from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl



__DEVICE__ float3 RRTAndODTFit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

__DEVICE__ float3 ACESFitted(float3 color)
{
    const mat3 ACESInputMat = to_mat3(
    0.59719f, 0.35458f, 0.04823f,
    0.07600f, 0.90834f, 0.01566f,
    0.02840f, 0.13383f, 0.83777
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = to_mat3(
     1.60475f, -0.53108f, -0.07367f,
    -0.10208f,  1.10813f, -0.00605f,
    -0.00327f, -0.07276f,  1.07602
);
   
    color = mul_f3_mat3(color , ACESInputMat);

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = mul_f3_mat3(color , ACESOutputMat);

    // Clamp to [0, 1]
    color = clamp(color, 0.0f, 1.0f);

    return color;
}

__DEVICE__ float impulse(float k, float x) {
    float h = k* x;
    return h* _expf(1.0f- h);
}
__DEVICE__ float cubicPulse(float c, float w, float _x) {
    _x = _fabs(_x - c);
    if(_x > w) return 0.0f;
    _x /= w;
    return 1.0f - _x*_x*(3.0f-2.0f*_x);
}
__DEVICE__ float3 hsv2rgb(float h, float s, float v) {
  return v* _mix(to_float3_s(1),clamp(abs_f3(mod_f3(h* 6.0f+ to_float3(0,4,2),6.0f)- 3.0f)- 1.0f,0.0f,1.0f),s);
}

__DEVICE__ float2 map(float3 U, float2 R, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3) {
    
    return to_float2(dist(swi2(U,x,z),B(swi2(U,x,z),R,iChannel1))-U.y*0.0625f*0.0625f,1);
}

//const float maxHei = 10.0f;
#define maxHei  10.0f
#define ZERO 0

__DEVICE__ float2 castRay( in float3 ro, in float3 rd, float2 R, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3) {
    float2 res = to_float2(-1.0f,-1.0f);
    
    float t = 0.0001f;
    for( int i=0; i<70; i++ ) {
        float2 h = map( ro+rd*t, R, iChannel1,iChannel3 );
        if( _fabs(h.x)<(0.00001f*t) ) {
            res = to_float2(t,h.y); 
            break;
        }
        t += h.x;
    }
    return res;
}

// http://iquilezles.org/www/articles/rmshadows/rmshadows.htm
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float mint, in float tmax, float2 R, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3 )
{
    // bounding volume
    float tp = (maxHei-ro.y)/rd.y; if( tp>0.0f ) tmax = _fminf( tmax, tp );

    float res = 1.0f;
    float t = mint;
    for( int i=ZERO; i<16; i++ )
    {
    float h = map( ro + rd*t, R,iChannel1,iChannel3 ).x;
        res = _fminf( res, 8.0f*h/t );
        t += clamp( h, 0.02f, 0.10f );
        if( res<0.005f || t>tmax ) break;
    }
    return clamp( res, 0.0f, 1.0f );
}


// http://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm
__DEVICE__ float3 calcNormal( in float3 pos, float2 R, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3 )
{
#if 1

    float2 e = to_float2(1.0f,-1.0f)*0.5773f*0.0005f;
    return normalize( swi3(e,x,y,y)*map( pos + swi3(e,x,y,y), R,iChannel1,iChannel3 ).x + 
                      swi3(e,y,y,x)*map( pos + swi3(e,y,y,x), R,iChannel1,iChannel3 ).x + 
                      swi3(e,y,x,y)*map( pos + swi3(e,y,x,y), R,iChannel1,iChannel3 ).x + 
                      swi3(e,x,x,x)*map( pos + swi3(e,x,x,x), R,iChannel1,iChannel3 ).x );
#else
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    float3 n = to_float3_s(0.0f);
    for( int i=ZERO; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*map(pos+0.0005f*e, R,iChannel1,iChannel3).x;
    }
    return normalize(n);
#endif    
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor, float2 R, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3 )
{
  float occ = 0.0f;
    float sca = 1.0f;
    for( int i=ZERO; i<5; i++ )
    {
        float hr = 0.01f + 0.12f*float(i)/4.0f;
        float3 aopos =  nor * hr + pos;
        float dd = map( aopos, R,iChannel1,iChannel3 ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95f;
    }
    return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f ) * (0.5f+0.5f*nor.y);
}



__DEVICE__ float3 render(float3 pos, float3 rd, float3 upv, float3 nor, float3 col, float occ, float3 lig, float3 blg, float bla, float3 ligc) {
    float3 ref = reflect(rd, nor);
    float3 hal = normalize(lig-rd);
    
    float NoH = dot(nor, hal);
    float HoV = dot(hal, rd);
    float NoV = dot(nor, rd);
    float RoY = dot(ref, upv);
    float NoY = dot(nor, upv);
    float NoL = dot(nor, lig);
    float NoB = dot(nor, blg);

    // lighting
    float amb = clamp(0.5f+0.5f*NoY, 0.0f, 1.0f);
    float dif = clamp(NoL, 0.0f, 1.0f);
    float bac = clamp(NoB, 0.0f, 1.0f)*bla;
    float dom = smoothstep(-0.2f, 0.2f, RoY);
    float fre = _powf(clamp(1.0f-_fabs(NoV),0.0f,1.0f), 8.0f);
    
    //dif *= calcSoftshadow(pos, lig, 0.02f, 2.5f);
    //dom *= calcSoftshadow(pos, ref, 0.02f, 2.5f);
    
    float speN = _powf(clamp(NoH,0.0f,1.0f),4.0f*4.0f);
    float speV = _powf(clamp(1.0f+HoV,0.0f,1.0f),4.0f*1.0f);
    //speV = 0.04f+0.96f*speV;
    float spe = speN*dif*speV;
    
    float3 bacc = to_float3(0.25f,0.25f,0.25f)+to_float3(0,0.125f,0.25f)*ligc;
    float3 lin = to_float3_s(0.0f);
    lin += 0.30f*dif*to_float3(1.00f,0.80f,0.55f);
    lin += 0.05f*amb*to_float3(0.40f,0.60f,1.00f)*occ;
    lin += 0.05f*dom*to_float3(0.40f,0.60f,1.00f)*occ;
    lin += 0.25f*bac*bacc*occ;
    lin += 0.05f*fre*to_float3(1.00f,1.00f,1.00f)*occ;
    col *= lin*1.0f;
    //col += to_float3(speV,dif,speN); 
    col += 90.0f*spe*ligc;
    //col += spe;
    return col;
}


__KERNEL__ void StopPollutingFuse(float4 Q, float2 U, float2 iResolution, float iTime, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    U+=0.5f;

    float4 a = A(U,R,iChannel0);
    float4 b = B(U,R,iChannel1);
    float4 df = distN(U,b);
    float s = df.x;
    float2 sGrad = s - to_float2(dist(U-to_float2(1,0),b),dist(U-to_float2(0,1),b));
    float2 azGrad = a.z - to_float2(A(U-to_float2(1,0),R,iChannel0).z, A(U-to_float2(0,1),R,iChannel0).z);
    //float m000 = smoothstep(-0.1f,1.1f,s-a.z*1.995f);
    float m000 = smoothstep(-0.1f,1.1f,s);
    float m001 = 1.0f-m000;
    float m010 = smoothstep(-0.1f-a.z*12.995f,1.1f-a.z*12.995f,s);
    float m011 = 1.0f-m010;
    float m100 = m010-m000;
    
    float m0 = smoothstep(-0.1f,1.1f,0.0f+s-a.z*1.995f);
    float m1 = 1.0f-m0;
    float m2 = smoothstep(0.25f,-0.95f,s);
    float m3 = 1.0f-m2;
    float pulse = dot(sGrad,swi2(a,x,y));
    float pulse1 = impulse(2.0f,_fmaxf(0.0f,pulse));
    float pulse2 = cubicPulse(-1.0f,0.95f,_fminf(0.0f,pulse));
    float am0 = _exp2f(-1.0f-0.125f*0.125f*s);
    float am1 = _exp2f(-1.0f-0.125f*s)*(1.0f+1.25f*pulse1-0.875f*pulse2);
    float am2 = 0.5f-0.5f*_cosf(1.53f*s);
    float am3 = m1*m2;
    float am4 = 0.5f-0.5f*_cosf(1.131f*s);

    float wave = _powf(_powf(_fabs(s),0.9f),0.9f);
    wave *= 1.0f+0.5f*pulse1;
    wave *= 1.0f-0.75f*pulse2;
    wave -= 2.0f*fract(2.0f*iTime-8.0f*length((swi2(iMouse,x,y)-U)/R));

    float hue = fract(0.41f+b.z*300.61f)*0.75f-0.0625f;
    hue = decodeColor(b.z)*0.75f-0.0625f;
    float hueM = 1.0f-2.0f*fract(0.41f+b.z*33.61f);
    hueM = 0.03f*hueM + 0.01f*sign_f(hueM);
    hueM = mod_f(hueM,0.0625f*0.5f);
    float sat = 1.0f-fract(0.31f+b.z*211.5f)*0.25f;
    float val = 1.0f;
    Q = to_float4_s(0);
    //Q = 0.5f+0.5f*_sinf(1.6f*dist(U,b)+A(U).z*to_float4(1,2,3,4));
    

    float4 col0 = to_float4(1,2,3,4);
    float4 col1 = to_float4(1,1,1,1)*(0.5f+0.5f*_sinf(PI*wave));
    float4 col2 = (0.5f+0.5f*sin_f4(PI*0.5f*s+a.z*to_float4(1,2,3,4)*3.5f));
    float4 col3 = (0.5f+0.5f*sin_f4(0.31f+b.z*to_float4(1,2,3,4)*41.5f));
    float4 col4 = (0.5f+0.5f*sin_f4(0.91f+b.z*to_float4(1,2,3,4)*41.5f));
    col3 = swi4(hsv2rgb(hue+hueM,0.5f+0.5f*sat,0.1f+0.6f*val),x,y,z,z);
    col4 = swi4(hsv2rgb(hue-hueM,0.1f+0.9f*sat,0.3f+0.7f*val),x,y,z,z);
    //Q += 0.25f*m000*to_float4(0,1,0,0);
    //Q += 0.25f*m010*to_float4(1,0,1,0);
    
    float outline = smoothstep(0.5f,2.0f,_fabs(s-0.75f));
    float4 colWater = to_float4_s(0);
    colWater += 0.5f*am0*col0;
    float4 test = clamp(to_float4_s(2.0f+_sinf(33.0f*a.z))-to_float4(1,2,0,0),0.0f,1.0f);
    float rippleWaves = 0.5f*_exp2f(-1.0f-0.125f*s)*(1.0f+1.25f*pulse1-0.875f*pulse2)*(0.5f+0.5f*_sinf(PI*wave))*smoothstep(-0.9f,0.1f,s);
    float rippleWavesMask = smoothstep(0.0f,0.0625f,a.z);
    colWater += rippleWaves* rippleWavesMask;
    colWater += 8.0f*length(azGrad)*_sinf(33.0f*a.z);
    colWater *= 0.5f+0.5f*outline;
    float4 colShape = to_float4_s(0);
    colShape += 0.5f*m1*am2*col2;
    colShape += _mix(col3,col4,am4);
    colShape *= outline;
    colShape += 0.5f*(1.0f-outline);
    
    float3 ro = to_float3_aw(R*0.5f,_fmaxf(R.x,R.y));
    float3 pos = to_float3_aw(U,0);
    float rz = distance_f3(pos,ro);
    float3 rd = normalize(pos-ro);
    float2 sun2D = rot(R, iTime*1.0025f);
    float3 sunO = to_float3_aw(swi2(iMouse,x,y),0.125f*ro.z); 
    float3 sunD = sunO-pos;
    float submerge = cubicPulse(2.1f,0.2f,a.z)*(0.0f+1.5f*_sinf(54.01f*a.z+iTime)*_sinf(54.01f*a.z+iTime));
    submerge = (a.z-1.9375f)*1.0f-2.0f*_fmaxf(-0.0625f*s-0.5f,0.0f);
    submerge = clamp(submerge,0.0f,1.0f);
    float3 res;
    {
        float3 axis = sunO;
        float3 nor = normalize(swi3(df,y,z,w));
        float4 alb = colShape;
        float3 lig = normalize(sunD);
        float3 blg = normalize(to_float3(-1,-1,0.5f)*lig);
        float bla = clamp(-10.0f+15.0f*nor.z,0.0f,1.0f);
        float3 upv = to_float3(0,0,1);
        float occ = clamp(1.0f-dot(to_float2_s(1),exp2_f2(32.0f*(abs_f2(2.0f*U/R-1.0f)-1.0f))),0.0f,1.0f);
        float4 colShape2 = to_float4_s(0);
        float3 ligc = to_float3_s(1);
        float3 col = res = render(pos, rd, upv, nor, swi3(colShape,x,y,z), occ, lig, blg, bla, ligc);
        for(float i = 0.0f; i < 1.0f; i += 1.0f/8.0f) {
            sunD = sunO- pos;
            sunD += to_float3_aw(450.0f*rot(normalize(swi2(sunD,x,y)+0.00001f),i*2.0f*PI),0.0625f*ro.z);
            lig = normalize(sunD);
            blg = normalize(to_float3(-1,-1,0.5f)*lig);
            ligc = hsv2rgb(i,0.875f,1.0f);
            float3 rend = render(pos, rd, upv, nor, swi3(colShape,x,y,z), occ, lig, blg, bla, ligc);
            //res *= _expf(-
            colShape2 += to_float4_aw(rend,1);//_fmaxf(to_float3_aw(0),res*8.0f-7.0f-col);
        }
        swi3(colShape,x,y,z) = 
            //col + 
            swi3(colShape2,x,y,z) / _fmaxf(1.0f,colShape2.w-50.0f);
            //col + (_sqrtf(1.0f+1.0f*colShape2)-1.0f)/1.0f;
    }
    float h001 = submerge;
    float h000 = smoothstep(-0.09f,2.9f,s)*(1.0f-h001);
    float h002 = (1.0f-h000)*(1.0f-h001);

    Q += 1.0f*h000*colWater;
    Q += 1.0f*h001*0.6f*colWater;
    Q += 1.0f*h001*0.3f*colShape;
    Q += 1.0f*h002*colShape;
    

#ifdef ORG            
    swi3(Q,x,y,z) = _powf(swi3(Q,x,y,z), to_float3(0.833f*2.0f));
    swi3(Q,x,y,z) *= 1.07f*0.99968f;
    swi3(Q,x,y,z) = ACESFitted(swi3(Q,x,y,z));
    swi3(Q,x,y,z) = clamp(linear_srgb(swi3(Q,x,y,z)), 0.0f, 1.0f);
#endif

    float3 _Q = pow_f3(swi3(Q,x,y,z), to_float3_s(0.833f*2.0f));
    _Q *= 1.07f*0.99968f;
    _Q = ACESFitted(_Q);
    _Q = clamp(linear_srgb(_Q), 0.0f, 1.0f);

    Q = to_float4_aw(_Q, Q.w);

  SetFragmentShaderComputedColor(Q);
}
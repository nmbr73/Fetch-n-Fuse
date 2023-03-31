
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 2' to iChannel1
// Connect Image 'Texture: Gray Noise Small' to iChannel2
// Connect Image 'Texture: Gray Noise Medium' to iChannel0
// Connect Image 'Texture: RGBA Noise Medium' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ mat3 rotx(float a) { mat3 rot; rot.r0 = to_float3(1.0f, 0.0f, 0.0f);          rot.r1 = to_float3(0.0f, _cosf(a), -_sinf(a)); rot.r2 = to_float3(0.0f, _sinf(a), _cosf(a));  return rot; }
__DEVICE__ mat3 roty(float a) { mat3 rot; rot.r0 = to_float3(_cosf(a), 0.0f, _sinf(a));  rot.r1 = to_float3(0.0f, 1.0f, 0.0f);          rot.r2 = to_float3(-_sinf(a), 0.0f, _cosf(a)); return rot; }
__DEVICE__ mat3 rotz(float a) { mat3 rot; rot.r0 = to_float3(_cosf(a), -_sinf(a), 0.0f); rot.r1 = to_float3(_sinf(a), _cosf(a), 0.0f);  rot.r2 = to_float3(0.0f, 0.0f, 1.0f);          return rot; }


//https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash(float2 p)
{
  #define HASHSCALE1 0.1031f
  float3 p3  = fract_f3((to_float3(p.x,p.y,p.x) * HASHSCALE1));
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract_f((p3.x + p3.y) * p3.z);
}

// https://www.shadertoy.com/view/lsf3WH
__DEVICE__ float noise( in float2 p )
{
  float2 i = _floor( p );
  float2 f = fract_f2( p );
  
  float2 u = f*f*(3.0f-2.0f*f);
  return _mix( _mix( hash( i + to_float2(0.0f,0.0f) ), 
                     hash( i + to_float2(1.0f,0.0f) ), u.x),
               _mix( hash( i + to_float2(0.0f,1.0f) ), 
                     hash( i + to_float2(1.0f,1.0f) ), u.x), u.y);
}


// https://www.shadertoy.com/view/Ml2XDw
__DEVICE__ float smax(float a, float b, float k)
{
    return _logf(exp(k*a)+_expf(k*b))/k;
}

// https://iquilezles.org/www/articles/distfunctions/distfunctions.htm
__DEVICE__ float smin( float a, float b, float k )
{
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 d = abs_f3(p) - b;
  return length(_fmaxf(d, to_float3_s(0.0f)))
              + _fminf(max(d.x,_fmaxf(d.y,d.z)),0.0f); // remove this line for an only partially signed sdf 
}

__DEVICE__ float opBentBox(in float3 p, in float3 v , float bend)
{
    float c = _cosf(bend*p.y);
    float s = _sinf(bend*p.y);
    mat2  m = to_mat2(c,-s,s,c);
    float3  q = to_float3_aw(mul_mat2_f2(m,swi2(p,x,y)),p.z);
    return sdBox(q, v);
}

__DEVICE__ float sdRoundBox( float3 p, float3 b, float r )
{
  float3 d = abs_f3(p) - b;
  return length(_fmaxf(d,to_float3_s(0.0f))) - r
              + _fminf(max(d.x,_fmaxf(d.y,d.z)),0.0f); // remove this line for an only partially signed sdf 
}

__DEVICE__ float3 traceSphere(in float3 ro, in float3 rd, float r, out float *t1, out float *t2)
{
    *t1=*t2=-1.0f;
    float3 X = ro + rd * (dot(normalize(-ro), rd)) * length(ro);
    float disc = r*r-_powf(length(X), 2.0f);
    if (disc < 0.0f) return to_float3_s(1000000.0f);
    disc=_sqrtf(disc);
    float3 p=X-disc*rd;
    *t1=length(p-ro);*t2=*t1+disc*2.0f;
    return p;
}

#define NOTHING 0
#define EYES    1

struct HitInfo
{
    int id;
    float3 pos;
    float d;
};

__DEVICE__ struct HitInfo map(in float3 rp)
{
    struct HitInfo hi;
    rp.x = _fabs(rp.x);
    hi.id = NOTHING;
    
    // head
    float head = sdRoundBox(rp*1.7f, to_float3(0.04f, 0.05f, 0.2f)*1.0f, 0.1f);
    head += length(rp + to_float3(0.0f, 0.0f, 0.1f)) - 0.15f;
    head = smin(head, length(rp * to_float3(1.1f, 1.3f, 1.0f)+ to_float3(0.0f, -0.1f, -0.07f)) -0.08f, 0.05f);
    
    // nostrils
    float nostril = length(rp * to_float3(6.0f, 10.0f, 1.0f) + to_float3(-0.27f, -0.5f, 0.25f)) - 0.1f;
    head = _fmaxf(head, -nostril );
    
    // ears
    float ear = length(rp * to_float3(1.0f, 1.0f, 5.0f) + to_float3(-0.05f, -0.14f, -0.5f)) - 0.02f;
    head = smin(head, ear, 0.02f);
    // eyes
    float3 eyePos = to_float3(-0.02f, -0.11f, -0.02f);
    float eye = length(rp + eyePos) - 0.03f;
    
    if(eye < 0.0f) 
    {
      hi.id = EYES;
        hi.pos = rp-eyePos;
        hi.d = eye;
        return hi;
    }

    head = _fminf(head, eye);
  
    // mouth
    float mouth = sdBox(mul_mat3_f3(rotx(-0.2f) , rp) + to_float3(0.0f, 0.02f, 0.215f), to_float3(0.15f, 0.001f * _fmaxf( -((rp.z))*25.0f, 0.0f), 0.15f));
    head = _fmaxf(head, -mouth);
    
    // torso    
    float torso = length(rp * to_float3(1.0f, 1.0f, 1.0f) + to_float3(0.0f, 0.12f, -0.04f)) - 0.13f;
    torso = smin(torso, head, _fmaxf(0.0f, rp.z*1.0f));
    
    // legs
    float leg = sdRoundBox(rp + to_float3(-0.075f, 0.2f, -0.1f), to_float3(0.04f, 0.3f, 0.04f)*0.25f, 0.04f);
    float feet = sdBox(rp + to_float3(-0.075f, 0.35f, -0.07f), to_float3(0.07f, 0.01f, 0.06f)*0.25f)-0.025f;
    leg = smin(leg, feet, 0.14f);
    torso = smin(torso,leg, 0.04f);
    
    // arms
    float arm = opBentBox(mul_mat3_f3(rotz(0.8f),(rp + to_float3(-0.15f, 0.09f, -0.08f))), to_float3(0.01f, 0.3f, 0.07f*_fmaxf(1.0f, -rp.y*0.0f))*0.25f, 5.0f) - 0.02f;
    const float fingerWidth = 0.03f;
    const float fingerBend = 40.0f;
    const float fingerX = -0.185f;
    const float roundness = 0.004f;
    const float spacing = 0.025f;
    const float smoothen = 0.02f;
    const float fingerY = 0.184f;
    
    float finger1 = opBentBox(rp + to_float3(fingerX, fingerY, -0.08f-spacing), to_float3(fingerWidth, 0.02f, 0.01f)*0.2f, fingerBend)-roundness;
    arm = smin(finger1, arm, smoothen);

    float finger2 = opBentBox(rp + to_float3(fingerX, fingerY, -0.08f), to_float3(fingerWidth, 0.02f, 0.01f)*0.2f, fingerBend)-roundness;
    arm = smin(finger2, arm, smoothen);

    float finger3 = opBentBox(rp + to_float3(fingerX, fingerY, -0.08f+spacing), to_float3(fingerWidth, 0.02f, 0.01f)*0.2f, fingerBend)-roundness;
    arm = smin(finger3, arm, smoothen);
    
    head = smin(arm, torso, 0.05f);
    
    
    float body = _fminf(head, torso);
    hi.d = body;
    return hi;
}

__DEVICE__ float3 grad(in float3 rp)
{
    float2 off = to_float2(0.002f, 0.0f);
    float3 g = to_float3(map(rp + swi3(off,x,y,y)).d - map(rp - swi3(off,x,y,y)).d,
                         map(rp + swi3(off,y,x,y)).d - map(rp - swi3(off,y,x,y)).d,
                         map(rp + swi3(off,y,y,x)).d - map(rp - swi3(off,y,y,x)).d);
    return normalize(g);
}

__DEVICE__ float ao(in float3 n, in float3 rp)
{
    float dist = 0.1f;
    rp += n*dist;
    float occ = 0.0f;
    const int steps = 4;
    
    for (int i = 0; i < steps; ++i)
    {
        float d = map(rp).d;
        float o= clamp(d/(dist*(float)(i + 1)), 0.0f, 1.0f);
        
        occ += o;
        rp += n * dist;
    }

    occ /= (float)(steps);
    return occ;
}

__DEVICE__ float fbm(in float3 rp)
{
    rp += to_float3(5.0f, 0.0f, 0.0f);
    float2 p = swi2(rp,x,z)*0.2f;
    float f = noise(p) * 0.5f;
    f += noise(p * 2.0f) * 0.5f * 0.5f;
    f += noise(p * 4.0f) * 0.5f * 0.5f * 0.5f;
    return f;
}

__DEVICE__ float sampleGround(in float3 rp)
{
    rp *= 3.0f;
    float texCol = 0.0f;
    float f = fbm(rp);
    texCol=1.0f-f;
    return texCol;
}

__DEVICE__ float3 groundNormal(in float3 rp, __TEXTURE2D__ iChannel3)
{
    float h0 = sampleGround(rp);
    float2 off = to_float2(0.1f, 0.0f);
    float h1 = h0 - sampleGround(rp + swi3(off,x,y,y));
    float h2 = h0 - sampleGround(rp + swi3(off,y,y,x));
    float h =0.5f;
    float3 f=(to_float3(off.x, h1*h, 0.0f));
    float3 u=(to_float3(0.0f, h2*h, off.x));
    float3 n = normalize(cross(u, f));
    n += (1.0f - 2.0f * swi3(texture(iChannel3, swi2(rp,x,z)*2.0f),x,y,z))*0.15f;
    n = normalize(n);
    return n*to_float3(-1.0f, 1.0f, -1.0f);
}

__DEVICE__ bool trace(in float3 rp, in float3 rd, inout float4 *color, int iFrame,float3 *g_hitp, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel3, float Par1, float Par2)
{
  
    float3 lightDir = normalize(to_float3(1.5f, 1.2f, -1.0f));
    const float groundH = 0.05f + Par1;
 
    bool hit = false;
    float3 ro = rp;
    float dist = 0.0f;
    struct HitInfo hi;
    
    // trace to character bounding sphere
    float t1, t2 = 0.0f;
    traceSphere(ro, rd, 0.38f, &t1, &t2);
    
    // character
    if(t1 > 0.0f)
    {
        rp = ro + t1 * rd;
        for (int i = 0; i < 140 + min(0, iFrame); ++i)
        {
            hi = map(rp);
            dist = hi.d;
            if(dist < 0.0f)
            {
                hit = true;
                break;
            }
            rp += rd * _fmaxf(dist*0.2f, 0.001f);

            if(length(ro - rp) > t2) break;

        }
        rp += rd * dist*0.5f;
        hi = map(rp);
    }
  
    // character color
    float3 albedo = to_float3(180.0f, 190.0f, 200.0f)/255.0f;
    if(hi.id == EYES)
    {
        float off = 0.155f;
        albedo = to_float3_s(1.0f-smoothstep(off, off+0.001f, dot(hi.pos, normalize(to_float3(0.0f, 1.0f, -1.0f)))));
    }
           
    if(hit)
    {
        float3 _color = to_float3_s(0.0f);
        float3 g = grad(rp);
        *g_hitp = rp;
        
        //diff
        float d = dot(g, lightDir);
        float wrap = 0.8f;
        d = d+wrap/(1.0f+wrap);
        d = clamp(d, 0.1f, 1.0f);
        _color += d*albedo*0.5f;
        
        //ao
        _color += ao(g, rp)*to_float3(239.0f, 219.0f, 159.0f)/255.0f*0.15f;
        
        // rim/fresn
        float3 source = normalize(to_float3(1.0f, 2.0f, 5.0f));
        float rim = _fmaxf(0.0f, (dot(reflect(source, g), rd)));
        rim = _powf(rim, 4.0f)*0.5f;
        _color += rim*to_float3(0.2f, 0.2f, 0.3f);
        
        // some grounding for character + shadow
        _color *= _mix(to_float3_s(1.0f), to_float3(0.4f, 0.6f, 0.8f), 1.0f-smoothstep(-0.7f, 0.3f, g.y));
        _color *= 0.4f + 0.6f * smoothstep(-0.5f, 0.0f, rp.y);
        
        *color = to_float4_aw(_color,0.0f);
    }
    
    float travel = length(ro - rp);
    float3 hitp = ro;
    float3 n = to_float3(0.0f, 1.0f, 0.0f);
    float t = (-dot(n, ro)+groundH)/dot(rd, n);
    
    // ground
    if(t > 0.0f)
    {
        hitp = ro + rd*t;
        float vdist = 0.0f;
        
        // rougher tracing
        for (int i = 0; i < 40 + min(0, iFrame); ++i)
        {
            float texCol = sampleGround(hitp);
            vdist = hitp.y - (groundH - texCol);
            if(vdist < 0.0f)
            {
                break;
            }

            hitp += rd*0.05f*_logf(2.0f+dot(ro-hitp, ro-hitp));
        }
        
        // hone into the surface
        for (int i = 0; i < 40 + min(0, iFrame); ++i)
        {
            hitp += rd * vdist;
            float texCol = sampleGround(hitp);
            vdist = hitp.y - (groundH - texCol);
        }
    
        if(!hit || (travel > length(ro - hitp)))
        {
            // dif
            float3 n = groundNormal(hitp,iChannel3);
            float d = dot(n, normalize(to_float3(0.0f, 1.0f, 0)));
            d = clamp(d, 0.1f, 0.99f);
            float3 groundCol = to_float3(0.7f, 1.0f, 1.0f) *_powf(d, 4.0f)*0.5f;
            
            // rim            
            float _rimd = 1.0f-(n.y * -rd.y);
            float rimd = _powf(_rimd, 8.0f) * 4.0f;
            rimd = clamp(rimd, 0.0f, 1.0f);
            
            groundCol += to_float3(0.4f, 0.6f, 0.8f) * rimd;
            groundCol += swi3(texture(iChannel1, swi2(hitp,x,z)*0.02f),x,x,x) * swi3(texture(iChannel3, swi2(hitp,x,z)),x,x,x);
            
            float specd = dot(reflect(normalize(to_float3(0.0f, -1.0f, 0.0f)), n), -rd); 
            specd = _powf((clamp(specd, 0.0f, 1.0f)), 4.0f) * 0.3f ;
            groundCol += to_float3(0.8f, 0.9f, 1.0f)*specd * Par2;
            
            //swi3(color,x,y,z) = groundCol;
            (*color).x = groundCol.x;
            (*color).y = groundCol.y;
            (*color).z = groundCol.z;

            *g_hitp = hitp;

            swi3S(*color,x,y,z, swi3(*color,x,y,z) * 0.5f + 0.5f * smoothstep(0.0f, 0.5f, length(swi2(hitp,x,z))));
        }
    }
    
    return hit;
}
    
__DEVICE__ mat3 lookat(float3 from, float3 to)
{
    float3 f = normalize(to - from);
    float3 _tmpr = normalize(cross(f, to_float3(0.0f, 1.0f, 0.0f)));
    float3 u = normalize(cross(_tmpr, f));
    float3 r = normalize(cross(u, f));
    return to_mat3_f3(r, u, f);
}


__KERNEL__ void StargazinghippoFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
  
    CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
    CONNECT_SLIDER1(Par1, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Par2, -10.0f, 10.0f, 1.0f);


    float3 g_hitp = to_float3_s(0.0f);

    float2 uv = fragCoord / iResolution;
    uv -= to_float2_s(0.5f);
    uv.y /= iResolution.x / iResolution.y;

    float2 im = 2.0f * ((swi2(iMouse,x,y) / iResolution) - to_float2_s(0.5f));
    im.y *= 0.7f;
    float3 rd = normalize(to_float3_aw(uv, 0.4f));
    float3 rp = to_float3(0.0f, 0.7f, -0.7f);
    float3 _rp = rp;
    rp = mul_mat3_f3(roty(im.x) , rp);
    rp.y = (mul_mat3_f3(rotx(im.y) ,_rp)).y;
    
    if(iMouse.z <= 0.0f)
    {
        float T = iTime * 0.2f;
        rp.x = _sinf(T+0.4f);
        rp.y = _sinf(T) * 0.25f + 0.3f;
        rp.z = -0.6f;
    }
    
    rd = mul_mat3_f3(lookat(rp, to_float3_s(0.0f)) , rd);
    float4 bgCol = to_float4(0.0f, 0.1f+rd.y*0.2f, 0.2f, 0.15f)*0.15f;
    bool hit = trace(rp, rd, &fragColor, iFrame, &g_hitp, iChannel1, iChannel3, Par1, Par2);
    
    float light = smoothstep(5.0f, 1.0f, length(g_hitp));
    swi3S(fragColor,x,y,z, _mix(swi3(fragColor,x,y,z), to_float3(0.0f, 0.0f, 0.02f), 0.99f-light));
    if(!hit)
    {
        swi3S(fragColor,x,y,z, _mix(swi3(fragColor,x,y,z), swi3(bgCol,x,y,z), smoothstep(-0.15f, 0.0f, rd.y)));
        
        float2 starCoord = to_float2( _atan2f(rd.x, rd.z), rd.y);
        float3 stars = swi3(_tex2DVecN(iChannel0,starCoord.x,starCoord.y,15),x,x,x)*smoothstep(-1.0f, 0.1f, rd.y);
        stars = smoothstep(to_float3_s(0.5f), to_float3_s(1.0f), stars-0.3f);
        stars *= swi3(texture(iChannel0, starCoord+to_float2_s(iTime*0.02f)),x,x,x);
        //swi3(fragColor,x,y,z) += stars;
        fragColor.x += stars.x;
        fragColor.y += stars.y;
        fragColor.z += stars.z;
    }
    fragColor = smoothstep(to_float4_s(0.0f), to_float4_s(1.0f), fragColor);
    fragColor = to_float4_aw(pow_f3(swi3(fragColor,x,y,z), to_float3_s(1.0f / 2.2f)), Alpha);
    
    
  SetFragmentShaderComputedColor(fragColor);
}
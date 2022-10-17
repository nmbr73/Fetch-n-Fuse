
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ mat3 transpose(mat3 m)
{
    return(to_mat3(m.r0.x,m.r1.x,m.r2.x, m.r0.y,m.r1.y,m.r2.y, m.r0.z,m.r1.z,m.r2.z)); 	
}


#define R iResolution
#define IR to_int2_cfloat(iResolution)
#define C0(uv) texture(iChannel0, (uv))
#define C1(uv) texture(iChannel1, (uv))
#define IC0(p) texture(iChannel0, (make_float2((p))+0.5f)/R)
#define IC1(p) texture(iChannel1, (make_float2((p))+0.5f)/R)
#define IC2(p) texture(iChannel2, (make_float2((p))+0.5f)/R)


// Version 1: First try, encoded each particle position at that time
// Version 2: encode prev+next site as integer, as well as phase.
// Version 3: same, but use particle tracking/search instead of exhaustive loop each frame.

// In version 2: Looping over >100 particles resulted in under 30fps on my laptop,
// so for version 3, use particle tracking.

//#define VERSION 1
#define VERSION 2
#define SIZE 8
#define SIZEf 8.0f
// How many particles to search for each frame. Higher = faster convergence but slower.
#define NEW_SAMPLES 8

__DEVICE__ float rand11(float x) { return fract(337.1f*_sinf(x*22.13f+11.2f)); }
__DEVICE__ float2 rand22(float2 p) { return fract_f2(to_float2(
    337.1f*_sinf(22.3f+27.31f*p.y+p.x*22.13f),
    437.1f*_sinf(1.7f+22.31f*p.y+p.x*29.13f) )); }
__DEVICE__ float3 rand23(float2 p) { return fract_f3(to_float3(
    337.1f*_sinf(22.3f+27.31f*p.y+p.x*22.13f),
    437.1f*_sinf(1.7f+22.31f*p.y+p.x*29.13f),
    407.1f*_sinf(111.7f+12.31f*p.y+p.x*19.13f) )); }
__DEVICE__ float3 rand33(float3 p) { return fract_f3(to_float3(
    337.1f*_sinf(22.3f+27.31f*p.y+p.x*22.13f+p.z*19.2f),
    437.1f*_sinf(1.7f+22.31f*p.y+p.x*29.13f+p.z*10.3f),
    407.1f*_sinf(111.7f+12.31f*p.y+p.x*19.13f+p.z*8.2f) )); }

__DEVICE__ float distanceToLine(float3 p1, float3 p2, float3 q) {
    return length(cross(q-p1, q-p2)) / length(p1-p2);
}

//const float CAM_DIST = 4.0f;
//const float FOCAL_COEFF = 1.60f;


__DEVICE__ float xsect_sphere(float3 ro, float3 rd, float radius) {
    float a = dot(rd,rd);
    float b = 2.0f * dot(rd,ro);
    float c = dot(ro,ro) - radius;
    float discrim = b*b - 4.0f*a*c;
    if (discrim < 0.0f) return 0.0f;
    float t1 = -b + _sqrtf(b*b - 4.0f*a*c);
    float t2 = -b - _sqrtf(b*b - 4.0f*a*c);
    return _fminf(t1,t2) / 2.0f*a;
}

__DEVICE__ float2 randVor22(float2 g, float iTime) {
    //g += iTime * 0.00001f;
    //float a = _sinf(iTime)*0.5f+0.5f;
    const float a = 1.0f;
    return rand22(g) * a + 0.5f - 0.5f * a;
}

__DEVICE__ float3 site(float2 pp, float t) {
    float2 g = _floor(pp);
    float2 a = (g+randVor22(g, t)) * 3.141f / SIZEf * 0.5f;
    return swi3(normalize(to_float3(_sinf(a.y)*_cosf(a.x), _sinf(a.y)*_sinf(a.x), _cosf(a.y))),x,z,y);
}
__DEVICE__ float3 isite(int i, float t) {
    float2 p = make_float2(i % SIZE, i / SIZE);
    float2 pp = to_float2(p.x,p.y) * 2.0f - SIZEf * 0.5f;
    return site(pp, t);
}
__DEVICE__ float3 partAtPhase(float4 samp, float t) {
    float3 a = isite((int)(samp.x+0.1f), t);
    float3 b = isite((int)(samp.y+0.1f), t);
    float phase = samp.z;
    float amp = fract(samp.w*9999.77f) * 0.2f + 0.1f;
    float3 part = normalize((1.0f-phase)*a + phase*b) * (1.0f + amp*(1.0f-_powf(_cosf(phase*3.141f),2.0f)));
    return part;
}

__DEVICE__ float2 proj(float3 p, float2 focal, float tt) {
    return swi2(p,x,y) * focal / (p.z + tt);
}



__DEVICE__ float hashXY(int2 xy, int2 ir) {
    return (float)(xy.y * ir.x + xy.x);
}
__DEVICE__ int2 unhashXY(int h, int2 ir) {
    float zzzzzzzzzzzz;
    return to_int2(h%ir.x, h/ir.x);
}



    
//float smoo(float v) { return v*v*(3.0f-2.0f*v); }
__DEVICE__ float smoo(float v) { return v; }
__DEVICE__ float3 noise33(float3 x) {
    float3 o = to_float3_s(0.0f);
    float3 fx = _floor(x);
    float3 lx = fract_f3(x);
    for (int i=0; i<8; i++) {
        float dx = (i  ) % 2 == 0 ? 1.0f : 0.0f;
        float dy = (i/2) % 2 == 0 ? 1.0f : 0.0f;
        float dz = (i/4) % 2 == 0 ? 1.0f : 0.0f;
        float3 v  = rand33(fx + to_float3(dx,dy,dz));
        float3 d = to_float3(dx,dy,dz) * 2.0f * lx + 1.0f - lx - to_float3(dx,dy,dz);
        o += smoo(d.x)*smoo(d.y)*smoo(d.z)*v;
    }
    return o;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Particle Sim


__KERNEL__ void FlightcontrolcenterFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame)
{
    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;

    if (iFrame == 0 || Reset) { fragColor = to_float4_s(0.0f); return; }
    
    float4 old = IC0(to_int2_cfloat(fragCoord));
    float4 _new = old;
    
    float TS = (fragCoord.x * 2.0f / R.x + 2.0f) / 24.0f;
    float AMP = rand11(fragCoord.x * 0.97f) * 0.2f + 0.1f;
float AAAAAAAAAAAAAAAAAAAAAAA;    
    /*
    if (_new.w == 0.0f || _new.w >= 1.0f) {
        // Random init
        _new = to_float4_aw(rand33(to_float3(fragCoord+iTime*0.3336f, iTime+float(iFrame)))*2.0f-1.0f, 0.00001f);
        swi3(_new,x,y,z) = normalize(swi3(_new,x,y,z));
    } else {
        // Move particle along path
        _new.w += (fragCoord.x * 2.0f / R.x + 1.0f) / 60.0f;
        
        float3 a = normalize(rand33(to_float3_aw(fragCoord+iTime*0.3336f, iTime+float(iFrame)))*2.0f-1.0f);
        float3 b = normalize(rand33(to_float3_aw(fragCoord+iTime*0.3336f, iTime+float(iFrame)))*2.0f-1.0f);
    }
    */
    
    #if VERSION == 1
    float period = _floor(iTime * TS);
    float phase  = fract(iTime * TS);
    
    float3 a = normalize(rand33(to_float3_aw(fragCoord,0.0f)+to_float3_s(period))*2.0f-1.0f);
    float3 b = normalize(rand33(to_float3_aw(fragCoord,0.0f)+to_float3_s(period+1.0f))*2.0f-1.0f);
    swi3S(_new,x,y,z, normalize((1.0f-phase)*a + phase*b) * (1.0f + AMP*(1.0f-_powf(_cosf(phase*3.141f),2.0f))));
    
    #elif VERSION == 2 || VERSION == 3
    
    // row 0: i | j | phase | timeScale
    // i,j encode xy integer location.
    // Which is then further transformed by the site function.
    
    float S = 2.0f * _powf(SIZEf,2.0f) + 0.99f;
    if (iFrame <= 1) {
        _new.z = fragCoord.x/R.x;
        _new.y = _floor(rand11(fragCoord.x + iTime*9.333f + 11.9f) * S );
        _new.x = _floor(rand11(fragCoord.x + iTime*9.333f) * S );
        _new.w = (((float)(fragCoord.x) / R.x) + 1.0f) / 320.0f;
    } else if (_new.z > 1.0f) {
        _new.z = 0.0f;
        _new.x = _new.y;
        int i = (int)(old.y + 0.1f);
        float2 p = make_float2(i % SIZE, i / SIZE) * 2.0f - 1.0f;
        float2 pp = p + (rand22(to_float2(fragCoord.x,fragCoord.x*2.3f)+iTime) * 2.0f - 1.0f) * 2.0f;
        _new.y = _floor(rand11(fragCoord.x + iTime*7.333f) * S);
        //_new.y = _new.x + 1.0f;
        _new.w = (((float)(fragCoord.x) / R.x) + 1.0f) / 320.0f;
    } else _new.z += (((float)(fragCoord.x) / R.x) + 1.0f) / 320.0f;
    
    #endif
    
    //swi2(_new,x,y) = to_float2(_floor(fragCoord.x/200.0f));
    
    fragColor = _new;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// Particle Tracking.
// Each pixel tracks the four nearest particles to it, when particles projected.

union A2F
 {
   float4  F; //32bit float
   float  A[4];  //32bit unsigend integer
 };


__KERNEL__ void FlightcontrolcenterFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime)
{
    fragCoord+=0.5f;    
    
    const float CAM_DIST = 4.0f;
    const float FOCAL_COEFF = 1.60f;

    A2F ds; ds.F = to_float4_s(9999.0f);
    
    float tt = iTime / 33.0f;
    //mat3 RR = to_mat3_f(1.0f);
    //RR[0] = to_float3(_cosf(tt), 0.0f, _sinf(tt));
    //RR[2] = to_float3(_sinf(tt), 0.0f, -_cosf(tt));
    
    mat3 RR = to_mat3_f3(to_float3(_cosf(tt), 0.0f, _sinf(tt)), to_float3(0.0f,1.0f,0.0f), to_float3(_sinf(tt), 0.0f, -_cosf(tt)));
    
    const float f = FOCAL_COEFF;
    float2 fuv = (fragCoord-swi2(R,x,y)*0.5f)/R.y;
    
    A2F old; old.F = IC0(to_int2_cfloat(fragCoord));
    A2F _new; _new.F = old.F;
    int2 meXY = to_int2_cfloat(fragCoord);
    const bool oneRow = true;
    
    for (int j=0; j<4; j++) {
        ds.A[j] = length(proj(mul_mat3_f3(RR,swi3(partAtPhase(IC1(unhashXY((int)(old.A[j]),IR)),iTime),x,y,z)),to_float2_s(f),CAM_DIST) - fuv);
    }
    
    // Sample neighbours
float BBBBBBBBBBBBBBBBBBBBBBBBB;    
    for (int j=0; j<4; j++) {
        int2 nnXY = (meXY + make_int2((j==0)?-1:(j==1)?1:0, (j==2)?-1:(j==3)?1:0));
        int2 nXY = unhashXY((int)(IC0(nnXY).x),IR);
        
        if (oneRow) nXY.y = 0;
        float3 part = mul_mat3_f3(RR,partAtPhase(IC1(nXY),iTime));
        if (part.z > 0.0f) continue; // Discard if behind
        float d = length(proj(part,to_float2_s(f),CAM_DIST) - fuv);
        
        
        if (d <= ds.A[0]) {
            if (d<ds.A[0]) {
            ds.A[3] = ds.A[2]; _new.A[3] = _new.A[2];
            ds.A[2] = ds.A[1]; _new.A[2] = _new.A[1];
            ds.A[1] = ds.A[0]; _new.A[1] = _new.A[0];
            }
            ds.A[0] = d;
            _new.A[0] = hashXY(nXY, IR);
        } else if (d <= ds.A[1]) {
            if (d<ds.A[1]) {
            ds.A[3] = ds.A[2]; _new.A[3] = _new.A[2];
            ds.A[2] = ds.A[1]; _new.A[2] = _new.A[1];
            }
            ds.A[1] = d;
            _new.A[1] = hashXY(nXY, IR);
        } else if (d <= ds.A[2]) {
            if (d<ds.A[2]) {
            ds.A[3] = ds.A[2]; _new.A[3] = _new.A[2];
            }
            ds.A[2] = d;
            _new.A[2] = hashXY(nXY, IR);
        } else if (d <= ds.A[3]) {
            ds.A[3] = d;
            _new.A[3] = hashXY(nXY, IR);
        }
        /*
        for (int k=0; k<4; k++) {
            if (d < ds.A[k]) {
                for (int l=3; l>k; l--) ds.A[l] = ds.A[l-1], _new.A[l] = _new.A[l-1];
                ds.A[k] = d;
                _new.A[k] = hashXY(nXY,IR);
                break;
            }
        }*/
    }
    
    // Sample random

    float2 size = swi2(R,x,y);
    size.y = 1.0f;
    //size.x = 12.0f;
    size.x = _fminf(R.x,320.0f); // This controls max particles.
    for (int i=0; i<NEW_SAMPLES; i++) {
        int2 nXY = to_int2_cfloat(rand22(iTime*0.0111117f+fragCoord) * size);
        
        if (oneRow) nXY.y = 0;
        float3 part = mul_mat3_f3(RR,partAtPhase(IC1(nXY),iTime));
        if (part.z > 0.0f) continue; // Discard if behind
        float d = length(proj(part,to_float2_s(f),CAM_DIST) - fuv);
        
        if (d <= ds.A[0]) {
            if (d<ds.A[0]) {
            ds.A[3] = ds.A[2]; _new.A[3] = _new.A[2];
            ds.A[2] = ds.A[1]; _new.A[2] = _new.A[1];
            ds.A[1] = ds.A[0]; _new.A[1] = _new.A[0];
            }
            ds.A[0] = d;
            _new.A[0] = hashXY(nXY, IR);
        } else if (d <= ds.A[1]) {
            if (d<ds.A[1]) {
            ds.A[3] = ds.A[2]; _new.A[3] = _new.A[2];
            ds.A[2] = ds.A[1]; _new.A[2] = _new.A[1];
            ds.A[1] = d;
            }
            _new.A[1] = hashXY(nXY, IR);
        } else if (d <= ds.A[2]) {
            if (d<ds.A[2]) {
            ds.A[3] = ds.A[2]; _new.A[3] = _new.A[2];
            }
            ds.A[2] = d;
            _new.A[2] = hashXY(nXY, IR);
        } else if (d <= ds.A[3]) {
            ds.A[3] = d;
            _new.A[3] = hashXY(nXY, IR);
        }
        /*
        for (int k=0; k<4; k++) {
            if (d < ds.A[k]) {
                for (int l=3; l>k; l--) ds.A[l] = ds.A[l-1], _new.A[l] = _new.A[l-1];
                ds.A[k] = d;
                _new.A[k] = hashXY(nXY,IR);
                break;
            }
        }*/
    }
    
    fragColor = _new.F;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel2
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


// Main scene + time averaging



__KERNEL__ void FlightcontrolcenterFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime)
{
    fragCoord+=0.5f;
    
    const float CAM_DIST = 4.0f;
    const float FOCAL_COEFF = 1.60f;
    
float CCCCCCCCCCCCCCCCCCCCCCCCCCC;
    float3 col = to_float3(0.003f,0.001f,0.02f);
    float tt = iTime / 33.0f;
    //tt = 0.0f;
    
    float3 scol = to_float3_s(0.0f);
    
    //mat3 RR = to_mat3_f(1.0f);
    //RR[0] = to_float3(_cosf(tt), 0.0f, _sinf(tt));
    //RR[2] = to_float3(_sinf(tt), 0.0f, -_cosf(tt));
    
    mat3 RR = to_mat3_f3(to_float3(_cosf(tt), 0.0f, _sinf(tt)), to_float3(0.0f,1.0f,0.0f), to_float3(_sinf(tt), 0.0f, -_cosf(tt)));
    
    float2 fuv = (fragCoord-swi2(R,x,y)*0.5f)/R.y;
    /*
    float3 ro = CAM_DIST * to_float3(_sinf(tt), 0.0f, -_cosf(tt));
    float3 rd_ = normalize(to_float3((fragCoord-swi2(R,x,y)*0.5f)/R.y, FOCAL_COEFF));
    float3 rd = to_float3(rd_.x*_cosf(tt)-_sinf(tt)*rd_.z, rd_.y,
                   rd_.x*_sinf(tt)+rd_.z*_cosf(tt));
                   */
    float3 ro = CAM_DIST * mul_mat3_f3(RR , to_float3(0.0f,0.0f,-1.0f));
    float3 rd = mul_mat3_f3(transpose(RR) , normalize(to_float3_aw((fragCoord-swi2(R,x,y)*0.5f)/R.y, FOCAL_COEFF)));
                   
    // Intersect sphere
    float t = xsect_sphere(ro,rd, 1.0f);
    float3 p = (ro + t * rd);//.xyz;
    if (t != 0.0f) {
        scol = to_float3(0.04f,0.11f,0.21f) * (1.0f+length(fuv)) * 4.0f;
        scol.y *= 0.8f + 0.3f * _sinf(p.y*3.0f+iTime);
        scol.x *= 0.5f + 1.5f * _sinf(p.x*5.0f+iTime);
        
        float lat = _acosf(p.y/length(p));
        float lng = _atan2f(p.z,p.x);
        
        // Do quad pattern
        float y = lat * SIZEf / 3.141f * 2.0f;
        float x = lng * SIZEf / 3.141f * 2.0f;
        float2 pp = to_float2(x,y);
        float2 g = _floor(pp);
        float2 a = g+randVor22(g,iTime);
        float acc = 0.0f;
        for (int i=0; i<4; i++) {
            float2 dd = to_float2((i==0)?-1.0f:(i==1)?1.0f:0.0f,(i==2)?-1.0f:(i==3)?1.0f:0.0f);
            float2 aa = g+dd + randVor22(g+dd,iTime);
            float d = distanceToLine(to_float3_aw(a,1.0f), to_float3_aw(aa,1.0f), to_float3_aw(pp,1.0f));
            if (dot(a-pp,aa-pp) < 0.0f)
                acc = _fmaxf(acc, 0.09f / (0.1f+d));
        }
        //if (acc < 1.0f) scol *= 0.0f;
        scol *= _expf(acc*5.0f-5.0f);
        
        //cc *= (1.0f - _fabs(_sinf(lng*30.0f)) * _fabs(_sinf(lat*30.0f))) * 2.1f;
        col += scol;
    }
    
    
    // Render particles
    #if VERSION == 1
    float3 cc = to_float3_s(0.0f);
    float dampen = 0.0f;
    int N = IR.x / 4;
    for (int i=0; i<N; i++) {
        float3 c = to_float3_s(0.0f);
        float4 part4 = IC0(to_int2(i,0));
        float3 part = mul_mat3_f3(RR , swi3(part4,x,y,z));
        float2 ppart = FOCAL_COEFF * swi2(part,x,y) / (part.z + CAM_DIST);
        float d = length(ppart - fuv);
        
        dampen += 0.005f / (0.0001f+d);
        if (d > 0.01f) d = 999.0f;
        c.y += 0.002f/(d+0.0002f);
        if (d > 0.01f) d = 999.0f;
        c.z += 0.004f/(d+0.0002f);
        float AMP = (float)(i) / (float)(N);
        cc += c * AMP;
        
        if (t != 0.0f) {
            float sd = _sqrtf(_powf(clamp(length(p-swi3(part4,x,y,z)) - 0.03f,0.0f,999.0f),2.0f));
            float3 c = (scol) * 15.0f;
            col += _expf(-sd*100.0f) * c;
        }
    }
    //col *= 1.0f/dampen;
    col += cc;


    float4 old = IC1(to_int2_cfloat(fragCoord));
    float ALPHA = 0.2f;
    col = col * ALPHA + (1.0f-ALPHA) * swi3(old,x,y,z);
    fragColor = to_float4_aw(col,1.0f);
    
    #elif VERSION == 2
    
    float3 cc = to_float3_s(0.0f);
    float dampen = 0.0f;
    /*
    int N = IR.x / 10;
    //N = 32;
    //N=64;
    for (int i=0; i<N; i++) {
        float AMP = 0.5f * float(i) / float(N);
        float4 samp = IC0(to_int2(i,0));
    */
    //int4 pidxs = to_int4_aw(IC2(to_int2_cfloat(fragCoord)));
    int pidxs[4] = {IC2(to_int2_cfloat(fragCoord)).x,IC2(to_int2_cfloat(fragCoord)).y,IC2(to_int2_cfloat(fragCoord)).z,IC2(to_int2_cfloat(fragCoord)).w};
    for (int i=0; i<4; i++) {
        float AMP = 0.5f;
        
        //ivec2 pidx = to_int2(IC2(unhashXY(int(pidxs[i]),IR)));
        int2 pidx = unhashXY(pidxs[i],IR);
        //pidx = to_int2(1,0);
        //pidx.x = 0;
        float4 samp = IC0(pidx);

        
        float3 c = to_float3_s(0.0f);
        
        float3 part_ = partAtPhase(samp, iTime);
        float3 part = mul_mat3_f3(RR , part_);
        //vec2 ppart = FOCAL_COEFF * swi2(part,x,y) / (part.z + CAM_DIST);
        float2 ppart = proj(part,to_float2_s(FOCAL_COEFF),CAM_DIST);
        
        
        // version a: use 2d projected distance to shade particle
        if (false) {
            float d = length(ppart - fuv);
            float far_mult = 1.0f-smoothstep(CAM_DIST-1.0f, CAM_DIST+0.5f, part.z + CAM_DIST);
            //c.z += 0.001f/(d+0.00001f);
            c.z += _expf(-clamp(d,0.001f,999.0f)*200.0f) * far_mult;
            c.y += _expf(-clamp(d,0.002f,999.0f)*300.0f) * far_mult;        
            cc += c;
        }
        
        // version b: use 2d distance to line to shade particle.
        //            line direction comes from instantaneous velocity, after projecting
        else {
            float phase = samp.z;
            float phasePrev = phase - samp.w * 1.0f;
            samp.z = phasePrev;
            float3 prevPart = mul_mat3_f3(RR , partAtPhase(samp, iTime));

            //vec2 prevPpart = FOCAL_COEFF * swi2(prevPart,x,y) / (prevPart.z + CAM_DIST);
            float2 prevPpart = proj(prevPart,to_float2_s(FOCAL_COEFF),CAM_DIST);
            //vec3 line = cross(to_float3_aw(ppart,1.0f), to_float3_aw(prevPpart,1.0f));
            float ld = distanceToLine(to_float3_aw(ppart,1.0f), to_float3_aw(prevPpart,1.0f), to_float3_aw(fuv,1.0f));
            float d = length(ppart-fuv);
            float dd = ld * (1.0f / (0.001f+d));

            float lineLength = 0.5f - _fabs(phase-0.5f) + 0.1f; // goes 0.1f -> 0.6f -> .1
            // There's two factors: one for line distance and one for radial distance.
            float cs = 2.0f* _expf(-ld*600.0f) * _expf(-d*25.0f / lineLength); 
            c = to_float3(0.4f - fract(samp.w*777.773f)*0.4f, 0.2f, 0.99f);
            //c.x += _expf(-d*50.0f);
            float far_mult = 1.0f-smoothstep(CAM_DIST-1.0f, CAM_DIST+0.5f, part.z + CAM_DIST);
            //far_mult = 10.0f;
            cc += c * cs * far_mult;
        }
        
        
        // Boost at projected point on sphere
        if (t != 0.0f) {
            float sd = _sqrtf(_powf(clamp(length(p-swi3(part_,x,y,z)) - 0.03f,0.0f,999.0f),2.0f));
            float3 c = (scol) * 5.0f;
            col += _expf(-sd*19.0f) * c;
        }
    }
    col += cc;
    
    float4 old = IC1(to_int2_cfloat(fragCoord));
    col = swi3(old,x,y,z) * 0.95f + 0.05f * col;
    col += length(swi2(fuv,x,y)) * to_float3(0.1f,0.0f,0.1f) * 0.06f;
    col += length(swi2(fuv,x,y) + to_float2(0.5f,0.2f)) * to_float3(0.001f,0.3f,0.52f) * _fabs(fract(iTime*0.1f)-0.5f) * 0.03f;
    col -= col * rand23(swi2(fuv,x,y)) * 0.02f;
    fragColor = to_float4_aw(col,1.0f);
        
    #endif

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void FlightcontrolcenterFuse(float4 fragColor, float2 fragCoord, float2 iResolution)
{

    fragCoord+=0.5f;

    fragColor = IC0(to_int2_cfloat(fragCoord));

// Show tracked particles / voro cells
#if 0
    fragColor *= 0.5f;
    
    int2 top0 = unhashXY((int)(IC2(to_int2(fragCoord)).x),IR);
    int2 top1 = unhashXY((int)(IC2(to_int2(fragCoord)).y),IR);
    int2 top2 = unhashXY((int)(IC2(to_int2(fragCoord)).z),IR);
    int2 top3 = unhashXY((int)(IC2(to_int2(fragCoord)).w),IR);
    //swi2(fragColor,x,y) = to_float2(top) / R;
    float d = 320.0f;
    fragColor.x += (float)(top0.x) / d;
    fragColor.y += (float)(top1.x) / d;
    fragColor.z += (float)(top2.x) / d;
    fragColor.w = 1.0f;
#endif

  SetFragmentShaderComputedColor(fragColor);
}
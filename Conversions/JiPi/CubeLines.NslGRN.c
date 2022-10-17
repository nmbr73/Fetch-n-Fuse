
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0


#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Created by Danil (2021+) https://twitter.com/AruGL
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// self https://www.shadertoy.com/view/NslGRN


// --defines for "DESKTOP WALLPAPERS" that use this shader--
// comment or uncomment every define to make it work (add or remove "//" before #define)


// this shadertoy use ALPHA, NO_ALPHA set alpha to 1, BG_ALPHA set background as alpha
// iChannel0 used as background if alpha ignored by wallpaper-app
//#define NO_ALPHA
//#define BG_ALPHA
//#define SHADOW_ALPHA
//#define ONLY_BOX


// save PERFORMANCE by disabling shadow (about 2x less GPU usage)
//#define NO_SHADOW


// static CAMERA position, 0.49f on top, 0.001f horizontal
//#define CAMERA_POS 0.049


// speed of ROTATION
#define ROTATION_SPEED 0.8999f


// static SHAPE form, default 0.5f
//#define STATIC_SHAPE 0.15f


// static SCALE far/close to camera, 2.0f is default, exampe 0.5f or 10.0
//#define CAMERA_FAR 0.1


// ANIMATION shape change
//#define ANIM_SHAPE


// ANIMATION color change
//#define ANIM_COLOR





// use 4xMSAA for cube only (set 2-4-etc level os MSAA)
//#define AA_CUBE 4



// --shader code--

// Layers sorted and support transparency and self-intersection-transparency
// Antialiasing is only dFd. (with some dFd fixes around edges)

// using iq's intersectors: http://iquilezles.org/www/articles/intersectors/intersectors.htm
// using https://www.shadertoy.com/view/ltKBzG
// using https://www.shadertoy.com/view/tsVXzh
// using https://www.shadertoy.com/view/WlffDn
// using https://www.shadertoy.com/view/WslGz4

#define tshift 53.0f

// reflect back side
//#define backside_refl

// Camera with mouse
#define MOUSE_control

// _fminf(iFrame,0) does not speedup compilation in ANGLE
#define ANGLE_loops 0


// this shader discover Nvidia bug with arrays https://www.shadertoy.com/view/NslGR4
// use DEBUG with BUG, BUG trigger that bug and one layer will be white on Nvidia in OpenGL
//#define DEBUG
//#define BUG

#define FDIST 0.7f
#define PI 3.1415926f
#define GROUNDSPACING 0.5f
#define GROUNDGRID 0.05f
#define BOXDIMS to_float3(0.75f, 0.75f, 1.25f)

#define IOR 1.33f

__DEVICE__ mat3 rotx(float a){float s = _sinf(a);float c = _cosf(a);return to_mat3_f3(to_float3(1.0f, 0.0f, 0.0f), to_float3(0.0f, c, s), to_float3(0.0f, -s, c));  }
__DEVICE__ mat3 roty(float a){float s = _sinf(a);float c = _cosf(a);return to_mat3_f3(to_float3(c, 0.0f, s), to_float3(0.0f, 1.0f, 0.0f), to_float3(-s, 0.0f, c));}
__DEVICE__ mat3 rotz(float a){float s = _sinf(a);float c = _cosf(a);return to_mat3_f3(to_float3(c, s, 0.0f), to_float3(-s, c, 0.0f), to_float3(0.0f, 0.0f, 1.0f ));}


__DEVICE__ float _fwidth(float inp, float2 R){
    //simulate fwidth
    float uvx = inp + 1.0f/R.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp + 1.0f/R.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}


__DEVICE__ float3 fcos(float3 x, float2 R) {
    float3 w = to_float3(_fwidth(x.x,R),_fwidth(x.y,R),_fwidth(x.z,R));
    //if((length(w)==0.0f))return to_float3_s(0.0f); // dFd fix2
    //w*=0.0f; //test
    float lw=length(w);
    if((lw==0.0f)||isnan(lw)||isinf(lw)){float3 tc=to_float3_s(0.0f); for(int i=0;i<8;i++)tc+=cos_f3(x+x*(float)(i-4)*(0.01f*400.0f/iResolution.y));return tc/8.0f;}
    
    return cos_f3(x) * smoothstep(to_float3_s(3.14f * 2.0f), to_float3_s(0.0f), w);
}

// rename to fcos
__DEVICE__ float3 fcos2( float3 x){return cos_f3(x);}

__DEVICE__ float3 getColor(float3 p, float iTime, float2 R)
{
    // dFd fix, dFd broken on borders, but it fix only top level dFd, self intersection has border
    //if (length(p) > 0.99f)return to_float3_s(0.0f);
    p = abs_f3(p);

    p *= 01.25f;
    p = 0.5f * p / dot(p, p);
#ifdef ANIM_COLOR
    p+=0.072f*iTime;
#endif

    float t = (0.13f) * length(p);
    float3 col = to_float3(0.3f, 0.4f, 0.5f);
    col += 0.12f * fcos(6.28318f * t * 1.0f + to_float3(0.0f, 0.8f, 1.1f),R);
    col += 0.11f * fcos(6.28318f * t * 3.1f + to_float3(0.3f, 0.4f, 0.1f),R);
    col += 0.10f * fcos(6.28318f * t * 5.1f + to_float3(0.1f, 0.7f, 1.1f),R);
    col += 0.10f * fcos(6.28318f * t * 17.1f + to_float3(0.2f, 0.6f, 0.7f),R);
    col += 0.10f * fcos(6.28318f * t * 31.1f + to_float3(0.1f, 0.6f, 0.7f),R);
    col += 0.10f * fcos(6.28318f * t * 65.1f + to_float3(0.0f, 0.5f, 0.8f),R);
    col += 0.10f * fcos(6.28318f * t * 115.1f + to_float3(0.1f, 0.4f, 0.7f),R);
    col += 0.10f * fcos(6.28318f * t * 265.1f + to_float3(1.1f, 1.4f, 2.7f),R);
    col = clamp(col, 0.0f, 1.0f);
 
    return col;
}

__DEVICE__ void calcColor(float3 ro, float3 rd, float3 nor, float d, float len, int idx, bool si, float td, out float4 *colx,
                          out float4 *colsi, float iTime, float2 R)
{

    float3 pos = (ro + rd * d);
#ifdef DEBUG
    float a = 1.0f - smoothstep(len - 0.15f, len + 0.00001f, length(pos));
    if (idx == 0) *colx = to_float4(1.0f, 0.0f, 0.0f, a);
    if (idx == 1) *colx = to_float4(0.0f, 1.0f, 0.0f, a);
    if (idx == 2) *colx = to_float4(0.0f, 0.0f, 1.0f, a);
    if (si)
    {
        pos = (ro + rd * td);
        float ta = 1.0f - smoothstep(len - 0.15f, len + 0.00001f, length(pos));
        if (idx == 0) *colsi = to_float4(1.0f, 0.0f, 0.0f, ta);
        if (idx == 1) * colsi = to_float4(0.0f, 1.0f, 0.0f, ta);
        if (idx == 2) *colsi = to_float4(0.0f, 0.0f, 1.0f, ta);
    }
#else
    float a = 1.0f - smoothstep(len - 0.15f*0.5f, len + 0.00001f, length(pos));
    //a=1.0f;
    float3 col = getColor(pos, iTime,R);
    *colx = to_float4_aw(col, a);
    if (si)
    {
        pos = (ro + rd * td);
        float ta = 1.0f - smoothstep(len - 0.15f*0.5f, len + 0.00001f, length(pos));
        //ta=1.0f;
        col = getColor(pos, iTime,R);
        *colsi = to_float4_aw(col, ta);
    }
#endif
}

// xSI is self intersect data, fade to fix dFd on edges
__DEVICE__ bool iBilinearPatch(in float3 ro, in float3 rd, in float4 ps, in float4 ph, in float sz, out float *t, out float3 *norm,
                               out bool *si, out float *tsi, out float3 *normsi, out float *fade, out float *fadesi)
{
    float3 va = to_float3(0.0f, 0.0f, ph.x + ph.w - ph.y - ph.z);
    float3 vb = to_float3(0.0f, ps.w - ps.y, ph.z - ph.x);
    float3 vc = to_float3(ps.z - ps.x, 0.0f, ph.y - ph.x);
    float3 vd = to_float3_aw(swi2(ps,x,y), ph.x);
    *t = -1.0f;
    *tsi = -1.0f;
    *si = false;
    *fade = 1.0f;
    *fadesi = 1.0f;
    *norm=to_float3(0.0f,1.0f,0.0f);*normsi=to_float3(0.0f,1.0f,0.0f);

    float tmp = 1.0f / (vb.y * vc.x);
    float a = 0.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = va.z * tmp;
    float e = 0.0f;
    float f = 0.0f;
    float g = (vc.z * vb.y - vd.y * va.z) * tmp;
    float h = (vb.z * vc.x - va.z * vd.x) * tmp;
    float i = -1.0f;
    float j = (vd.x * vd.y * va.z + vd.z * vb.y * vc.x) * tmp - (vd.y * vb.z * vc.x + vd.x * vc.z * vb.y) * tmp;

    float p = dot(to_float3(a, b, c), swi3(rd,x,z,y) * swi3(rd,x,z,y)) + dot(to_float3(d, e, f), swi3(rd,x,z,y) * swi3(rd,z,y,x));
    float q = dot(to_float3(2.0f, 2.0f, 2.0f) * swi3(ro,x,z,y) * swi3(rd,x,y,z), to_float3(a, b, c)) + dot(swi3(ro,x,z,z) * swi3(rd,z,x,y), to_float3(d, d, e)) +
              dot(swi3(ro,y,y,x) * swi3(rd,z,x,y), to_float3(e, f, f)) + dot(to_float3(g, h, i), swi3(rd,x,z,y));
    float r = dot(to_float3(a, b, c), swi3(ro,x,z,y) * swi3(ro,x,z,y)) + dot(to_float3(d, e, f), swi3(ro,x,z,y) * swi3(ro,z,y,x)) + dot(to_float3(g, h, i), swi3(ro,x,z,y)) + j;

    if (_fabs(p) < 0.000001f)
    {
        float tt = -r / q;
        if (tt <= 0.0f)
            return false;
        *t = tt;
        // normal

        float3 pos = ro + *t * rd;
        if(length(pos)>sz) return false;
        float3 grad = to_float3_s(2.0f) * swi3(pos,x,z,y) * to_float3(a, b, c) + swi3(pos,z,x,z) * to_float3(d, d, e) + swi3(pos,y,y,x) * to_float3(f, e, f) + to_float3(g, h, i);
        *norm = -1.0f*normalize(grad);
        return true;
    }
    else
    {
        float sq = q * q - 4.0f * p * r;
        if (sq < 0.0f)
        {
            return false;
        }
        else
        {
            float s = _sqrtf(sq);
            float t0 = (-q + s) / (2.0f * p);
            float t1 = (-q - s) / (2.0f * p);
            float tt1 = _fminf(t0 < 0.0f ? t1 : t0, t1 < 0.0f ? t0 : t1);
            float tt2 = _fmaxf(t0 > 0.0f ? t1 : t0, t1 > 0.0f ? t0 : t1);
            float tt0 = tt1;
            if (tt0 <= 0.0f)
                return false;
            float3 pos = ro + tt0 * rd;
            // black border on end of circle and self intersection with alpha come because dFd
            // uncomment this to see or rename fcos2 to fcos
            //sz+=0.3f; 
            bool ru = step(sz, length(pos)) > 0.5f;
            if (ru)
            {
                tt0 = tt2;
                pos = ro + tt0 * rd;
            }
            if (tt0 <= 0.0f)
                return false;
            bool ru2 = step(sz, length(pos)) > 0.5f;
            if (ru2)
                return false;

            // self intersect
            if ((tt2 > 0.0f) && ((!ru)) && !(step(sz, length(ro + tt2 * rd)) > 0.5f))
            {
                *si = true;
                *fadesi=s;
                *tsi = tt2;
                float3 tpos = ro + *tsi * rd;
                // normal
                float3 tgrad = to_float3_s(2.0f) * swi3(tpos,x,z,y) * to_float3(a, b, c) + swi3(tpos,z,x,z) * to_float3(d, d, e) +
                               swi3(tpos,y,y,x) * to_float3(f, e, f) + to_float3(g, h, i);
                *normsi = -1.0f*normalize(tgrad);
            }
            
            *fade=s;
            *t = tt0;
            // normal
            float3 grad = to_float3_s(2.0f) * swi3(pos,x,z,y) * to_float3(a, b, c) + swi3(pos,z,x,z) * to_float3(d, d, e) + swi3(pos,y,y,x) * to_float3(f, e, f) + to_float3(g, h, i);
            *norm = -1.0f*normalize(grad);

            return true;
        }
    }
}

__DEVICE__ float dot2( in float3 v ) { return dot(v,v); }

__DEVICE__ float segShadow( in float3 ro, in float3 rd, in float3 pa, float sh )
{
    float   dm = dot(swi2(rd,y,z),swi2(rd,y,z));
    float   k1 = (ro.x-pa.x)*dm;
    float   k2 = (ro.x+pa.x)*dm;
    float2  k5 = (swi2(ro,y,z)+swi2(pa,y,z))*dm;
    float   k3 = dot(swi2(ro,y,z)+swi2(pa,y,z),swi2(rd,y,z));
    float2  k4 = (swi2(pa,y,z)+swi2(pa,y,z))*swi2(rd,y,z);
    float2  k6 = (swi2(pa,y,z)+swi2(pa,y,z))*dm;
    
    for( int i=0; i<4 + ANGLE_loops; i++ )
    {
        float2  s = to_float2(i&1,i>>1);
        float t = dot(s,k4) - k3;
        
        if( t>0.0f )
        sh = _fminf(sh,dot2(to_float3(clamp(-rd.x*t,k1,k2),k5.x-k6.x*s.x,k5.y-k6.y*s.y)+rd*t)/(t*t));
    }
    return sh;
}

__DEVICE__ float boxSoftShadow( in float3 ro, in float3 rd, in float3 rad, in float sk ) 
{
    rd += 0.0001f * (1.0f - abs_f3(sign_f3(rd)));
    float3 rdd = rd;
    float3 roo = ro;

    float3 m = 1.0f/rdd;
    float3 n = m*roo;
    float3 k = abs_f3(m)*rad;
  
    float3 t1 = -n - k;
    float3 t2 = -n + k;

    float tN = _fmaxf( _fmaxf( t1.x, t1.y ), t1.z );
    float tF = _fminf( _fminf( t2.x, t2.y ), t2.z );
  
    if( tN<tF && tF>0.0f) return 0.0f;
    
    float sh = 1.0f;
    sh = segShadow( swi3(roo,x,y,z), swi3(rdd,x,y,z), swi3(rad,x,y,z), sh );
    sh = segShadow( swi3(roo,y,z,x), swi3(rdd,y,z,x), swi3(rad,y,z,x), sh );
    sh = segShadow( swi3(roo,z,x,y), swi3(rdd,z,x,y), swi3(rad,z,x,y), sh );
    sh = clamp(sk*_sqrtf(sh),0.0f,1.0f);
    return sh*sh*(3.0f-2.0f*sh);
}

__DEVICE__ float box(in float3 ro, in float3 rd, in float3 r, out float3 nn, bool entering)
{
    rd += 0.0001f * (1.0f - abs_f3(sign_f3(rd)));
    float3 dr = 1.0f / rd;
    float3 n = ro * dr;
    float3 k = r * abs_f3(dr);

    float3 pin = -k - n;
    float3 pout = k - n;
    float tin = _fmaxf(pin.x, _fmaxf(pin.y, pin.z));
    float tout = _fminf(pout.x, _fminf(pout.y, pout.z));
    if (tin > tout)
        return -1.0f;
    if (entering)
    {
        nn = -1.0f*sign_f3(rd) * step(swi3(pin,z,x,y), swi3(pin,x,y,z)) * step(swi3(pin,y,z,x), swi3(pin,x,y,z));
    }
    else
    {
        nn = sign_f3(rd) * step(swi3(pout,x,y,z), swi3(pout,z,x,y)) * step(swi3(pout,x,y,z), swi3(pout,y,z,x));
    }
    return entering ? tin : tout;
}

__DEVICE__ float3 bgcol(in float3 rd)
{
    return _mix(to_float3_s(0.01f), to_float3(0.336f, 0.458f, 0.668f), 1.0f - _powf(_fabs(rd.z+0.25f), 1.3f));
}

__DEVICE__ float3 background(in float3 ro, in float3 rd , float3 l_dir, out float *alpha)
{
#ifdef ONLY_BOX
    *alpha=0.0f;
    return to_float3_s(0.01f);
#endif
    float t = (-BOXDIMS.z - ro.z) / rd.z;
    *alpha=0.0f;
    float3 bgc = bgcol(rd);
    if (t < 0.0f)
        return bgc;
    float2 uv = swi2(ro,x,y) + t * swi2(rd,x,y);
#ifdef NO_SHADOW
    float shad=1.0f;
#else
    float shad = boxSoftShadow((ro + t * rd), mul_f3_mat3(normalize(l_dir+to_float3(0.0f,0.0f,1.0f)) , rotz(PI*0.65f)) , BOXDIMS, 1.5f);
#endif
    float aofac = smoothstep(-0.95f, 0.75f, length(abs_f2(uv) - _fminf(abs_f2(uv), to_float2_s(0.45f))));
    aofac = _fminf(aofac,smoothstep(-0.65f, 1.0f, shad));
    float lght=_fmaxf(dot(normalize(ro + t * rd+to_float3(0.0f,-0.0f,-5.0f)), mul_f3_mat3(normalize(l_dir-to_float3(0.0f,0.0f,1.0f)) , rotz(PI*0.65f))), 0.0f);
    float3 col = _mix(to_float3_s(0.4f), to_float3(0.71f,0.772f,0.895f), lght*lght* aofac+ 0.05f) * aofac;
    *alpha=1.0f-smoothstep(7.0f,10.0f,length(uv));
#ifdef SHADOW_ALPHA
    //*alpha=clamp(*alpha*_fmaxf(lght*lght*0.95f,(1.0f-aofac)*1.25f),0.0f,1.0f);
    *alpha=clamp(*alpha*(1.0f-aofac)*1.25f,0.0f,1.0f);
#endif

    return _mix(col*length(col)*0.8f,bgc,smoothstep(7.0f,10.0f,length(uv)));
}

#define swap(a,b) tv=a;a=b;b=tv

__DEVICE__ float4 insides(float3 ro, float3 rd, float3 nor_c, float3 l_dir, out float *tout, float iTime, float2 R)
{
    // custom COLOR, and change those const values
    //#define USE_COLOR
    const float3 color_blue = to_float3(0.5f,0.65f,0.8f);
    const float3 color_red  = to_float3(0.99f,0.2f,0.1f);
  
    *tout = -1.0f;
    float3 trd=rd;

    float3 col = to_float3_s(0.0f);

    float pi = 3.1415926f;

    if (_fabs(nor_c.x) > 0.5f)
    {
        rd = swi3(rd,x,z,y) * nor_c.x;
        ro = swi3(ro,x,z,y) * nor_c.x;
    }
    else if (_fabs(nor_c.z) > 0.5f)
    {
        l_dir = mul_f3_mat3(l_dir,roty(pi));
        rd = swi3(rd,y,x,z) * nor_c.z;
        ro = swi3(ro,y,x,z) * nor_c.z;
    }
    else if (_fabs(nor_c.y) > 0.5f)
    {
        l_dir = mul_f3_mat3(l_dir,rotz(-pi * 0.5f));
        rd = rd * nor_c.y;
        ro = ro * nor_c.y;
    }

#ifdef ANIM_SHAPE
    float curvature = (0.001f+1.5f-1.5f*smoothstep(0.0f,8.5f,mod_f((iTime+tshift)*0.44f,20.0f))*(1.0f-smoothstep(10.0f,18.5f,mod_f((iTime+tshift)*0.44f,20.0f))));
    // curvature(to not const above) make compilation on Angle 15+ sec
#else
#ifdef STATIC_SHAPE
    const float curvature = STATIC_SHAPE;
#else
    const float curvature = 0.5f;
#endif
#endif
    float bil_size = 1.0f;
    float4 ps = to_float4(-bil_size, -bil_size, bil_size, bil_size) * curvature;
    float4 ph = to_float4(-bil_size, bil_size, bil_size, -bil_size) * curvature;
    
    float4 colx[3]   = {to_float4_s(0.0f),to_float4_s(0.0f),to_float4_s(0.0f)};
    float3 dx[3]     = {to_float3_s(-1.0f),to_float3_s(-1.0f),to_float3_s(-1.0f)};
    float4 colxsi[3] = {to_float4_s(0.0f),to_float4_s(0.0f),to_float4_s(0.0f)};
    int order[3]     = {0,1,2};

    for (int i = 0; i < 3 + ANGLE_loops; i++)
    {
        if (_fabs(nor_c.x) > 0.5f)
        {
            ro = mul_f3_mat3(ro,rotz(-pi * (1.0f / (float)(3))));
            rd = mul_f3_mat3(rd,rotz(-pi * (1.0f / (float)(3))));
        }
        else if (_fabs(nor_c.z) > 0.5f)
        {
            ro = mul_f3_mat3(ro,rotz(pi * (1.0f / (float)(3))));
            rd = mul_f3_mat3(rd,rotz(pi * (1.0f / (float)(3))));
        }
        else if (_fabs(nor_c.y) > 0.5f)
        {
            ro = mul_f3_mat3(ro,rotx(pi * (1.0f / (float)(3))));
            rd = mul_f3_mat3(rd,rotx(pi * (1.0f / (float)(3))));
        }
        float3 normnew;
        float tnew;
        bool si;
        float tsi;
        float3 normsi;
        float fade;
        float fadesi;

        if (iBilinearPatch(ro, rd, ps, ph, bil_size, &tnew, &normnew, &si, &tsi, &normsi, &fade, &fadesi))
        {
            if (tnew > 0.0f)
            {
                float4 tcol, tcolsi;
                calcColor(ro, rd, normnew, tnew, bil_size, i, si, tsi, &tcol, &tcolsi, iTime, R);
                if (tcol.w > 0.0f)
                {
                    {
                        float3 tvalx = to_float3(tnew, (float)(si), tsi);
                        dx[i]=tvalx;
                    }
#ifdef DEBUG
                    colx[i]=tcol;
                    if (si) colxsi[i]=tcolsi;
#else

                    float dif = clamp(dot(normnew, l_dir), 0.0f, 1.0f);
                    float amb = clamp(0.5f + 0.5f * dot(normnew, l_dir), 0.0f, 1.0f);

                    {
#ifdef USE_COLOR
                        float3 shad = 0.57f * color_blue * amb + 1.5f*swi3(color_blue,z,y,x) * dif;
                        const float3 tcr = color_red;
#else
                        float3 shad = to_float3(0.32f, 0.43f, 0.54f) * amb + to_float3(1.0f, 0.9f, 0.7f) * dif;
                        const float3 tcr = to_float3(1.0f,0.21f,0.11f);
#endif
                        float ta = clamp(length(swi3(tcol,x,y,z)),0.0f,1.0f);
                        tcol=clamp(tcol*tcol*2.0f,0.0f,1.0f);
                        float4 tvalx = to_float4_aw((swi3(tcol,x,y,z)*shad*1.4f + 3.0f*(tcr*swi3(tcol,x,y,z))*clamp(1.0f-(amb+dif),0.0f,1.0f)), _fminf(tcol.w,ta));
                        swi3S(tvalx,x,y,z, clamp(2.0f*swi3(tvalx,x,y,z)*swi3(tvalx,x,y,z),0.0f,1.0f));
                        tvalx *= (_fminf(fade*5.0f,1.0f));
                        colx[i]=tvalx;
                    }
                    if (si)
                    {
                        dif = clamp(dot(normsi, l_dir), 0.0f, 1.0f);
                        amb = clamp(0.5f + 0.5f * dot(normsi, l_dir), 0.0f, 1.0f);
                        {
#ifdef USE_COLOR
                            float3 shad = 0.57f * color_blue * amb + 1.5f*swi3(color_blue,z,y,x) * dif;
                            const float3 tcr = color_red;
#else
                            float3 shad = to_float3(0.32f, 0.43f, 0.54f) * amb + to_float3(1.0f, 0.9f, 0.7f) * dif;
                            const float3 tcr = to_float3(1.0f,0.21f,0.11f);
#endif
                            float ta = clamp(length(swi3(tcolsi,x,y,z)),0.0f,1.0f);
                            tcolsi=clamp(tcolsi*tcolsi*2.0f,0.0f,1.0f);
                            float4 tvalx = to_float4_aw(swi3(tcolsi,x,y,z) * shad + 3.0f*(tcr*swi3(tcolsi,x,y,z))*clamp(1.0f-(amb+dif),0.0f,1.0f), _fminf(tcolsi.w,ta));
                            swi3S(tvalx,x,y,z, clamp(2.0f*swi3(tvalx,x,y,z)*swi3(tvalx,x,y,z),0.0f,1.0f));
                            swi3S(tvalx,x,y,z, swi3(tvalx,x,y,z)*(_fminf(fadesi*5.0f,1.0f)));
                            colxsi[i]=tvalx;
                        }
                    }
#endif
                }
            }
        }
    }
    // transparency logic and layers sorting 
    float a = 1.0f;
    if (dx[0].x < dx[1].x){{float3 swap(dx[0], dx[1]);}{int swap(order[0], order[1]);}}
    if (dx[1].x < dx[2].x){{float3 swap(dx[1], dx[2]);}{int swap(order[1], order[2]);}}
    if (dx[0].x < dx[1].x){{float3 swap(dx[0], dx[1]);}{int swap(order[0], order[1]);}}

    *tout = _fmaxf(_fmaxf(dx[0].x, dx[1].x), dx[2].x);

    if (dx[0].y < 0.5f)
    {
        a=colx[order[0]].w;
    }

#if !(defined(DEBUG)&&defined(BUG))
    
    // self intersection
    bool rul[3] = {
                    ((dx[0].y > 0.5f) && (dx[1].x <= 0.0f)),
                    ((dx[1].y > 0.5f) && (dx[0].x > dx[1].z)),
                    ((dx[2].y > 0.5f) && (dx[1].x > dx[2].z))
                  };
    for(int k=0;k<3;k++){
        if(rul[k]){
            float4 tcolxsi = to_float4_s(0.0f);
            tcolxsi=colxsi[order[k]];
            float4 tcolx = to_float4_s(0.0f);
            tcolx=colx[order[k]];

            float4 tvalx = _mix(tcolxsi, tcolx, tcolx.w);
            colx[order[k]]=tvalx;

            float4 tvalx2 = _mix(to_float4_s(0.0f), tvalx, _fmaxf(tcolx.w, tcolxsi.w));
            colx[order[k]]=tvalx2;
        }
    }

#endif

    float a1 = (dx[1].y < 0.5f) ? colx[order[1]].w : ((dx[1].z > dx[0].x) ? colx[order[1]].w : 1.0f);
    float a2 = (dx[2].y < 0.5f) ? colx[order[2]].w : ((dx[2].z > dx[1].x) ? colx[order[2]].w : 1.0f);
    col = _mix(_mix(swi3(colx[order[0]],x,y,z), swi3(colx[order[1]],x,y,z), a1), swi3(colx[order[2]],x,y,z), a2);
    a = _fmaxf(_fmaxf(a, a1), a2);
    return to_float4_aw(col, a);
}
//**********************************************************************************************************************
__KERNEL__ void CubeLinesFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.0f, 23.0f, 21.0f, 1.0f); 

    float osc = 0.5f;
    float3 l_dir = normalize(to_float3(0.0f, 1.0f, 0.0f));
    l_dir = mul_f3_mat3(l_dir,rotz(0.5f));
    float mouseY = 1.0f * 0.5f * PI;
#ifdef MOUSE_control
    mouseY = (1.0f - 1.15f * iMouse.y / iResolution.y) * 0.5f * PI;
    if(iMouse.y < 1.0f)
#endif
#ifdef CAMERA_POS
    mouseY = PI*CAMERA_POS;
#else
    mouseY = PI*0.49f - smoothstep(0.0f,8.5f,mod_f((iTime+tshift)*0.33f,25.0f))*(1.0f-smoothstep(14.0f,24.0f,mod_f((iTime+tshift)*0.33f,25.0f))) * 0.55f * PI;
#endif
#ifdef ROTATION_SPEED
    float mouseX = -2.0f*PI-0.25f*(iTime*ROTATION_SPEED+tshift);
#else
    float mouseX = -2.0f*PI-0.25f*(iTime+tshift);
#endif
#ifdef MOUSE_control
    mouseX+=-(iMouse.x / iResolution.x) * 2.0f * PI;
#endif
    
#ifdef CAMERA_FAR
    float3 eye = (2.0f + CAMERA_FAR) * to_float3(_cosf(mouseX) * _cosf(mouseY), _sinf(mouseX) * _cosf(mouseY), _sinf(mouseY));
#else
    float3 eye = 4.0f * to_float3(_cosf(mouseX) * _cosf(mouseY), _sinf(mouseX) * _cosf(mouseY), _sinf(mouseY));
#endif
    float3 w = normalize(-eye);
    float3 up = to_float3(0.0f, 0.0f, 1.0f);
    float3 u = normalize(cross(w, up));
    float3 v = cross(u, w);

    float4 tot=to_float4_s(0.0f);
#ifdef AA_CUBE
    const int AA = AA_CUBE;
    float3 incol_once=to_float3_s(0.0f);
    bool in_once=false;
    float4 incolbg_once=to_float4_s(0.0f);
    bool bg_in_once=false;
    float4 outcolbg_once=to_float4_s(0.0f);
    bool bg_out_once=false;
    for( int mx=0; mx<AA; mx++ )
    for( int nx=0; nx<AA; nx++ )
    {
    float2 o = to_float2(mod_f((float)(mx+AA/2),(float)(AA)),mod_f((float)(nx+AA/2),(float)(AA))) / (float)(AA) - 0.5f;
    float2 uv = (fragCoord + o - 0.5f * iResolution) / iResolution.x;
#else
    float2 uv = (fragCoord - 0.5f * iResolution) / iResolution.x;
#endif
    float3 rd = normalize(w * FDIST + uv.x * u + uv.y * v);

    float3 ni;
    float t = box(eye, rd, BOXDIMS, ni, true);
    float3 ro = eye + t * rd;
    float2 coords = swi2(ro,x,y) * ni.z/swi2(BOXDIMS,x,y) + swi2(ro,y,z) * ni.x/swi2(BOXDIMS,y,z) + swi2(ro,z,x) * ni.y/swi2(BOXDIMS,z,x);
    float fadeborders = (1.0f-smoothstep(0.915f,1.05f,_fabs(coords.x)))*(1.0f-smoothstep(0.915f,1.05f,_fabs(coords.y)));

    if (t > 0.0f)
    {
        float ang = -iTime * 0.33f;
        float3 col = to_float3_s(0.0f);
#ifdef AA_CUBE
        if(in_once)col=incol_once;
        else{
        in_once=true;
#endif
        float R0 = (IOR - 1.0f) / (IOR + 1.0f);
        R0 *= R0;

        float2 theta = to_float2_s(0.0f);
        float3 n = to_float3(_cosf(theta.x) * _sinf(theta.y), _sinf(theta.x) * _sinf(theta.y), _cosf(theta.y));

        float3 nr = swi3(n,z,x,y) * ni.x + swi3(n,y,z,x) * ni.y + swi3(n,x,y,z) * ni.z;
        float3 rdr = reflect(rd, nr);
        float talpha;
        float3 reflcol = background(ro, rdr, l_dir, &talpha);

        float3 rd2 = refract_f3(rd, nr, 1.0f / IOR);

        float accum = 1.0f;
        float3 no2 = ni;
        float3 ro_refr = ro;

        float4 colo[2] = {to_float4_s(0.0f),to_float4_s(0.0f)};

        for (int j = 0; j < 2 + ANGLE_loops; j++)
        {
            float tb;
            float2 coords2 = swi2(ro_refr,x,y) * no2.z + swi2(ro_refr,y,z) * no2.x + swi2(ro_refr,z,x) * no2.y;
            float3 eye2 = to_float3_aw(coords2, -1.0f);
            float3 rd2trans = swi3(rd2,y,z,x) * no2.x + swi3(rd2,z,x,y) * no2.y + swi3(rd2,x,y,z) * no2.z;

            rd2trans.z = -rd2trans.z;
            float4 internalcol = insides(eye2, rd2trans, no2, l_dir, &tb, iTime, R);
            if (tb > 0.0f)
            {
                swi3S(internalcol,x,y,z, swi3(internalcol,x,y,z) * accum);
                colo[j]=internalcol;
            }

            if ((tb <= 0.0f) || (internalcol.w < 1.0f))
            {
                float tout = box(ro_refr, rd2, BOXDIMS, no2, false);
                no2 = swi3(n,z,y,x) * no2.x + swi3(n,x,z,y) * no2.y + swi3(n,y,x,z) * no2.z;
                float3 rout = ro_refr + tout * rd2;
                float3 rdout = refract_f3(rd2, -no2, IOR);
                float fresnel2 = R0 + (1.0f - R0) * _powf(1.0f - dot(rdout, no2), 1.3f);
                rd2 = reflect(rd2, -no2);

#ifdef backside_refl
                if((dot(rdout, no2))>0.5f){fresnel2=1.0f;}
#endif
                ro_refr = rout;
                ro_refr.z = _fmaxf(ro_refr.z, -0.999f);

                accum *= fresnel2;
            }
        }
        float fresnel = R0 + (1.0f - R0) * _powf(1.0f - dot(-rd, nr), 5.0f);
        col = _mix(_mix(swi3(colo[1],x,y,z) * colo[1].w, swi3(colo[0],x,y,z), colo[0].w)*fadeborders, reflcol, _powf(fresnel, 1.5f));
        col=clamp(col,0.0f,1.0f);
#ifdef AA_CUBE
        }
        incol_once=col;
        if(!bg_in_once){
        bg_in_once=true;
        float alpha;
        incolbg_once = to_float4_aw(background(eye, rd, l_dir, alpha), 0.15f);
#if defined(BG_ALPHA)||defined(ONLY_BOX)||defined(SHADOW_ALPHA)
        incolbg_once.w = alpha;
#endif
        }
#endif
        
        float cineshader_alpha = 0.0f;
        cineshader_alpha = clamp(0.15f*dot(eye,ro),0.0f,1.0f);
        float4 tcolx = to_float4_aw(col, cineshader_alpha);
#if defined(BG_ALPHA)||defined(ONLY_BOX)||defined(SHADOW_ALPHA)
        tcolx.w = 1.0f;
#endif
        tot += tcolx;
    }
    else
    {
        float4 tcolx = to_float4_s(0.0f);
#ifdef AA_CUBE
        if(!bg_out_once){
        bg_out_once=true;
#endif
        float alpha;
        tcolx = to_float4_aw(background(eye, rd, l_dir, &alpha), 0.15f);
#if defined(BG_ALPHA)||defined(ONLY_BOX)||defined(SHADOW_ALPHA)
        tcolx.w = alpha;
#endif
#ifdef AA_CUBE
        outcolbg_once=tcolx;
        }else tcolx=_fmaxf(outcolbg_once,incolbg_once);
#endif
        tot += tcolx;
    }
#ifdef AA_CUBE
    }
    tot /= (float)(AA*AA);
#endif
    fragColor = tot;
#ifdef NO_ALPHA
    fragColor.w = 1.0f;
#endif
    //fragColor.xyz=clamp(swi3(fragColor,x,y,z),0.0f,1.0f);
    fragColor = to_float4_aw(clamp(swi3(fragColor,x,y,z),0.0f,1.0f),Color.w);
#if defined(BG_ALPHA)||defined(ONLY_BOX)||defined(SHADOW_ALPHA)
    //fragColor.xyz=swi3(fragColor,x,y,z)*fragColor.w+texture(iChannel0, fragCoord/iResolution).rgb*(1.0f-fragColor.w);
    fragColor = to_float4_aw(swi3(fragColor,x,y,z)*fragColor.w+swi3(texture(iChannel0, fragCoord/iResolution),x,y,z)*(1.0f-fragColor.w), Color.w);
#endif
    //fragColor=to_float4(fragColor.w);

  SetFragmentShaderComputedColor(fragColor);
}
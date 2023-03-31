
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


/*
=============================================

Limit set of rank 4 hyperbolic Coxeter groups

                                by Zhao Liang
=============================================

This program shows the limit set of rank 4 hyperbolic Coxeter groups.

Some math stuff:

Let G be a hyperbolic Coxeter group and x a point inside the hyperbolic
unit ball, the orbit S_x = { gx, g \in G } has accumulation points
(under Euclidean metric) only on the boundary of the space. We call the
accumulation points of S_x the limit set of the group, it can be proved that
this set is independent of the way x is chosen, and it's the smallest
closed subset of the boundary that is invariant under the action of the group.

The Coxeter-Dynkin diagram of a rank 4 Coxeter group of string type has the form

   A --- B --- C --- D
      p     q     r

Here A, B, D can be chosen as ususal Euclidean planes, C is a sphere orthongonal
to the unit ball. This is taken from mla's notation, and as far as I know this
has long been used by users on fractalforums. (fragmentarium)

In this animation these points are colored in "brass metal".

==========
!important
==========

The limit set is a closed set with no interior points, to show them we have
to use an approximate procedure: we simply try to reflect a point p on the
boundary to the fundamental domain up to a maximum steps, once failed then we
think p belongs to the limit set.

**So the number MAX_REFLECTIONS is an important param**, if' its set to a high
threshold then little limit set will be shown, or if it's not high enough then
the boundary of the set will look too coarse, so beware of this.

As always, you can do whatever you want to this work.

Update: thanks @mla for helping fix some bugs!
*/
// ------------------------------------------

// --------------------------
// You can try more patterns like
// (3, 7, 3), (4, 6, 3), (4, 4, 5), (5, 4, 4), (7, 3, 4), ..., etc. (5, 4, 4) is now
// my favorite! set PQR below to see the result.
// For large PQRs the limit set will become too small to be visible, you need to adjust
// MAX_REFLECTIONS and tweak with the function chooseColor to get appealling results.

#define inf        -1.0f



// --------------------------
// some global settings

#define MAX_TRACE_STEPS  100
#define MIN_TRACE_DIST   0.1f
#define MAX_TRACE_DIST   100.0f
#define PRECISION        0.0001f
#define AA               2
#define MAX_REFLECTIONS  500
#define PI               3.141592653f

// another pattern
//#define CHECKER1  to_float3(0.196078f, 0.33f, 0.82f)
//#define CHECKER2  to_float3(0.75f, 0.35f, 0.196078f)

/*
#define CHECKER1  to_float3(0.82f, 0.196078f, 0.33f)
#define CHECKER2  to_float3(0.196078f, 0.35f, 0.92f)
#define MATERIAL  to_float3(0.71f, 0.65f, 0.26f)
#define FUNDCOL   to_float3(0.0f, 0.82f, 0.33f)
*/

// Shane's color scheme
//#define CHECKER1  to_float3(0.0f, 0.0f, 0.05f)
//#define CHECKER2  to_float3_s(0.2f)
//#define MATERIAL  to_float3(10, 0.3f, 0.2f)
//#define FUNDCOL   to_float3(0.3f, 1, 8)


// used to highlight the limit set
//#define LighteningFactor 8.0f
// --------------------------

//float3 A, B, D;
//float4 C;
//float orb;

__DEVICE__ float dihedral(float x) { return x == inf ? 1.0f : _cosf(PI / x); }

// minimal distance to the four mirrors
__DEVICE__ float distABCD(float3 p, float3 A, float3 B, float4 C, float3 D)
{
    float dA = _fabs(dot(p, A));
    float dB = _fabs(dot(p, B));
    float dD = _fabs(dot(p, D));
    float dC = _fabs(length(p - swi3(C,x,y,z)) - C.w);
    return _fminf(dA, _fminf(dB, _fminf(dC, dD)));
}

// try to reflect across a plane with normal n and update the counter
__DEVICE__ bool try_reflect(inout float3 *p, float3 n, inout int *count)
{
    float k = dot(*p, n);
    // if we are already inside, do nothing and return true
    if (k >= 0.0f)
      return true;

    *p -= 2.0f * k * n;
    *count += 1;
    return false;
}

// similar with above, instead this is a sphere inversion
__DEVICE__ bool try_reflect_f4(inout float3 *p, float4 sphere, inout int *count, inout float *orb)
{
    float3 cen = swi3(sphere,x,y,z);
    float r = sphere.w;
    float3 q = *p - cen;
    float d2 = dot(q, q);
    if (d2 == 0.0f)
      return true;
    float k = (r * r) / d2;
    if (k < 1.0f)
      return true;
    *p = k * q + cen;
    *count += 1;
    *orb *= k;
    return false;
}

// sdf of the unit sphere at origin
__DEVICE__ float sdSphere(float3 p, float radius) { return length(p) - 1.0f; }

// sdf of the plane y=-1
__DEVICE__ float sdPlane(float3 p, float offset) { return p.y + 1.0f; }

// inverse stereo-graphic projection, from a point on plane y=-1 to
// the unit ball centered at the origin
__DEVICE__ float3 planeToSphere(float2 p)
{
    float pp = dot(p, p);
    return swi3(to_float3_aw(2.0f * p, pp - 1.0f),x,z,y) / (1.0f + pp);
}

// iteratively reflect a point on the unit sphere into the fundamental cell
// and update the counter along the way
__DEVICE__ bool iterateSpherePoint(inout float3 *p, inout int *count, float3 A, float3 B, float4 C, float3 D, inout float *orb)
{
    bool inA, inB, inC, inD;
    for(int iter=0; iter<MAX_REFLECTIONS; iter++)
    {
        inA = try_reflect(p, A, count);
        inB = try_reflect(p, B, count);
        inC = try_reflect_f4(p, C, count, orb);
        inD = try_reflect(p, D, count);
        *p =  normalize(*p);  // avoid floating error accumulation
        if (inA && inB && inC && inD)
            return true;
    }
    return false;
}

// colors for fundamental domain, checker pattern and limit set.
__DEVICE__ float3 chooseColor(bool found, int count, float LighteningFactor, float orb, float3 CHECKER1, float3 CHECKER2, float3 MATERIAL, float3 FUNDCOL)
{
    float3 col;
    if (found)
    {
        if (count == 0) return FUNDCOL;
        else if (count >= 300) col = MATERIAL;
        else
            col = (count % 2 == 0) ? CHECKER1 : CHECKER2;

    }
    else
        col = MATERIAL;

    float t =  (float)(count) / (float)(MAX_REFLECTIONS);
    col = _mix(MATERIAL*LighteningFactor, col, 1.0f - t * smoothstep(0.0f, 1.0f, _logf(orb) / 32.0f));
    return col;
}

// 2d rotation
__DEVICE__ float2 rot2d(float2 p, float a) { return p * _cosf(a) + to_float2(-p.y, p.x) * _sinf(a); }

__DEVICE__ float2 map(float3 p)
{
    float d1 = sdSphere(p, 1.0f);
    float d2 = sdPlane(p, -1.0f);
    float id = (d1 < d2) ? 0.0f: 1.0f;
    return to_float2(_fminf(d1, d2), id);
}

// standard scene normal
__DEVICE__ float3 getNormal(float3 p)
{
    const float2 e = to_float2(0.001f, 0.0f);
    return normalize(
        to_float3(
                  map(p + swi3(e,x,y,y)).x - map(p  - swi3(e,x,y,y)).x,
                  map(p + swi3(e,y,x,y)).x - map(p  - swi3(e,y,x,y)).x,
                  map(p + swi3(e,y,y,x)).x - map(p  - swi3(e,y,y,x)).x
                 )
        );
}

// get the signed distance to an object and object id
__DEVICE__ float2 raymarch(in float3 ro, in float3 rd)
{
    float t = MIN_TRACE_DIST;
    float2 h = to_float2_s(0.0f);
    for(int i=0; i<MAX_TRACE_STEPS; i++)
    {
        h = map(ro + t * rd);
        if (h.x < PRECISION * t)
            return to_float2(t, h.y);

        if (t > MAX_TRACE_DIST)
            break;

        t += h.x;
    }
    //return to_float2_s(-1.0f);
    return to_float2(t, h.y);
}

__DEVICE__ float calcOcclusion(float3 p, float3 n) {
    float occ = 0.0f;
    float sca = 1.0f;
    for (int i = 0; i < 5; i++) {
        float h = 0.01f + 0.15f * (float)(i) / 4.0f;
        float d = map(p + h * n).x;
        occ += (h - d) * sca;
        sca *= 0.75f;
    }
    return clamp(1.0f - occ, 0.0f, 1.0f);
}


__DEVICE__ float softShadow(float3 ro, float3 rd, float tmin, float tmax, float k) {
    float res = 1.0f;
    float t = tmin;
    for (int i = 0; i < 12; i++) {
        float h = map(ro + rd * t).x;
        res = _fminf(res, k * h / t);
        t += clamp(h, 0.01f, 0.2f);
        if (h < 0.0001f || t > tmax)
            break;
    }
    return clamp(res, 0.0f, 1.0f);
}


__DEVICE__ float3 getColor(float3 ro, float3 rd, float3 pos, float3 nor, float3 lp, float3 basecol, float3 BaseColor)
{
    float3 col = to_float3_s(0.0f);
    float3 ld = lp - pos;
    float lDist = _fmaxf(length(ld), 0.001f);
    ld /= lDist;
    float ao = calcOcclusion(pos, nor);
    float sh = softShadow(pos+0.001f*nor, ld, 0.02f, lDist, 32.0f);
    float diff = clamp(dot(nor, ld), 0.0f, 1.0f);
    float atten = 2.0f / (1.0f + lDist * lDist * 0.01f);

    float spec = _powf(_fmaxf( dot( reflect(-ld, nor), -rd ), 0.0f ), 32.0f);
    float fres = clamp(1.0f + dot(rd, nor), 0.0f, 1.0f);

    col += basecol * diff;
    col += basecol * BaseColor * spec * 4.0f;// to_float3(1.0f, 0.8f, 0.3f) * spec * 4.0f;
    col += basecol * to_float3_s(0.8f) * fres * fres * 2.0f;
    col *= ao * atten * sh;
    col += basecol * clamp(0.8f + 0.2f * nor.y, 0.0f, 1.0f) * 0.5f;
    return col;
}

__DEVICE__ mat3 sphMat(float theta, float phi)
{
    float cx = _cosf(theta);
    float cy = _cosf(phi);
    float sx = _sinf(theta);
    float sy = _sinf(phi);
    return to_mat3(cy, -sy * -sx, -sy * cx,
                    0,   cx,  sx,
                   sy,  cy * -sx, cy * cx);
}


__KERNEL__ void HyperbolicGroupLimitSetFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    CONNECT_COLOR0(CHECKER1, 0.0f, 0.0f, 0.05f, 1.0f);
    CONNECT_COLOR1(CHECKER2, 0.2f, 0.2f, 0.2f, 1.0f);
    CONNECT_COLOR2(MATERIAL, 10.0f, 0.3f, 0.2f, 1.0f);
    CONNECT_COLOR3(FUNDCOL, 0.3f, 1.0f, 8.0f, 1.0f);
    CONNECT_COLOR4(BaseColor, 1.0f, 0.8f, 0.3f, 1.0f);
    
    CONNECT_SLIDER0(LighteningFactor, -1.0f, 20.0f, 8.0f);
    
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
    CONNECT_SLIDER1(ViewZ, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER2(ColorMix, -1.0f, 1.0f, 0.0f);
    
    //float LighteningFactor = 8.0f;

    float3 finalcol = to_float3_s(0.0f);
    int count = 0;
    float2 m = to_float2(0.0f, 1.0f) + swi2(iMouse,x,y) / iResolution;
    float rx = m.y * PI;
    float ry = -m.x * 2.0f * PI;
    mat3 mouRot = sphMat(rx, ry);

// ---------------------------------
// initialize the mirrors
    const float3 PQR = to_float3(3, 3, 7);

    float P = PQR.x, Q = PQR.y, R = PQR.z;
    float cp = dihedral(P), sp = _sqrtf(1.0f - cp*cp);
    float cq = dihedral(Q);
    float cr = dihedral(R);
    float3 A = to_float3(0,  0,   1);
    float3 B = to_float3(0, sp, -cp);
    float3 D = to_float3(1,  0,   0);

    float r = 1.0f / cr;
    float k = r * cq / sp;
    float3 cen = to_float3(1, k, 0);
    float4 C = to_float4_aw(cen, r) / _sqrtf(dot(cen, cen) - r * r);

// -------------------------------------
// view setttings

    float3 camera = to_float3(3.0f, 3.2f, -3.0f) + to_float3_aw(ViewXY,ViewZ);
    float3 lp = to_float3(0.5f, 3.0f, -0.8f); //light position
    swi2S(camera,x,z, rot2d(swi2(camera,x,z), iTime*0.3f));
    float3 lookat  = to_float3(0.0f, -0.5f, 0.0f);
    float3 up = to_float3(0.0f, 1.0f, 0.0f);
    float3 forward = normalize(lookat - camera);
    float3 right = normalize(cross(forward, up));
    up = normalize(cross(right, forward));

// -------------------------------------
// antialiasing loop

    for(int ii=0; ii<AA; ii++)
    {
        for(int jj=0; jj<AA; jj++)
        {
            float2 o = to_float2((float)(ii), (float)(jj)) / (float)(AA);
            float2 uv = (2.0f * fragCoord + o - iResolution) / iResolution.y;
            float3 rd = normalize(uv.x * right + uv.y * up + 3.0f * forward);
            float orb = 1.0f;
            // ---------------------------------
            // hit the scene and get distance, object id

            float2 res = raymarch(camera, rd);
            float t = res.x;
            float id = res.y;
            float3 pos = camera + t * rd;

            bool found;
            float edist;
            float3 col;
            // the sphere is hit
            if (id == 0.0f)
            {
                float3 nor = pos;
                float3 q = mul_f3_mat3(pos , mouRot);
                found = iterateSpherePoint(&q, &count, A, B, C, D, &orb);
                edist = distABCD(q, A, B, C, D);
                float3 basecol = chooseColor(found, count, LighteningFactor, orb, swi3(CHECKER1,x,y,z),swi3(CHECKER2,x,y,z),swi3(MATERIAL,x,y,z),swi3(FUNDCOL,x,y,z));

                col = getColor(camera, rd, pos, nor, lp, basecol, swi3(BaseColor,x,y,z));
            }
            // the plane is hit
            else if (id == 1.0f)
            {
                float3 nor = to_float3(0.0f, 1.0f, 0.0f);
                float3 q = planeToSphere(swi2(pos,x,z));
                q = mul_f3_mat3(q , mouRot);
                found = iterateSpherePoint(&q, &count, A, B, C, D, &orb);
                edist = distABCD(q, A, B, C, D);
                float3 basecol = chooseColor(found, count, LighteningFactor, orb, swi3(CHECKER1,x,y,z),swi3(CHECKER2,x,y,z),swi3(MATERIAL,x,y,z),swi3(FUNDCOL,x,y,z));
                col = getColor(camera, rd, pos, nor, lp, basecol, swi3(BaseColor,x,y,z)) * 0.9f;
            }
            // draw the arcs
            col = _mix(col, to_float3_s(0.0f), (1.0f - smoothstep(0.0f, 0.005f, edist))*0.85f);
            col = _mix(col, to_float3_s(0.0f), 1.0f - _expf(-0.01f*t*t));
            finalcol += col;
        }
    }
    finalcol /= ((float)(AA) * (float)(AA));

// ------------------------------------
// a little post-processing

    finalcol = _mix(finalcol, 1.0f - exp_f3(-finalcol), 0.35f+ColorMix);
    fragColor = to_float4_aw(sqrt_f3(_fmaxf(finalcol, to_float3_s(0.0f))), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Cubemap: Forest_0' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


// Created by SHAU - 2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//-----------------------------------------------------

#define R iResolution
#define ZERO (_fminf(iFrame,0))
#define EPS .002
#define FAR 100.
#define T iTime

//Fabrice - compact rotation
__DEVICE__ mat2 rot(float x) {return mat2(_cosf(x), _sinf(x), -_sinf(x), _cosf(x));}

//Shane IQ
__DEVICE__ float n3D(float3 p) {    
  const float3 s = to_float3(7, 157, 113);
  float3 ip = _floor(p); 
    p -= ip; 
    float4 h = to_float4(0.0f, swi2(s,y,z), s.y + s.z) + dot(ip, s);
    p = p * p * (3.0f - 2.0f * p);
    h = _mix(fract(_sinf(h) * 43758.5453f), fract(_sinf(h + s.x) * 43758.5453f), p.x);
    swi2(h,x,y) = _mix(swi2(h,x,z), swi2(h,y,w), p.y);
    return _mix(h.x, h.y, p.z);
}

//distance functions from IQ
//https://iquilezles.org/articles/distfunctions
__DEVICE__ float sdCappedCylinder( float3 p, float h, float r )
{
  float2 d = _fabs(to_float2(length(swi2(p,x,z)),p.y)) - to_float2(h,r);
  return _fminf(max(d.x,d.y),0.0f) + length(_fmaxf(d,0.0f));
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 q = _fabs(p) - b;
  return length(_fmaxf(q,0.0f)) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}

__DEVICE__ float dot2( in float2 v ) { return dot(v,v); }
__DEVICE__ float sdCappedCone( float3 p, float h, float r1, float r2 )
{
  float2 q = to_float2( length(swi2(p,x,y)), p.z );
  float2 k1 = to_float2(r2,h);
  float2 k2 = to_float2(r2-r1,2.0f*h);
  float2 ca = to_float2(q.x-_fminf(q.x,(q.y<0.0f)?r1:r2), _fabs(q.y)-h);
  float2 cb = q - k1 + k2*clamp( dot(k1-q,k2)/dot2(k2), 0.0f, 1.0f );
  float s = (cb.x<0.0f && ca.y<0.0f) ? -1.0f : 1.0f;
  return s*_sqrtf( _fminf(dot2(ca),dot2(cb)) );
}

__DEVICE__ float sdTorus( float3 p, float2 t )
{
  float2 q = to_float2(length(swi2(p,x,y))-t.x,p.z);
  return length(q)-t.y;
}

__DEVICE__ float sdTriPrism( float3 p, float2 h )
{
  float3 q = _fabs(p);
  return _fmaxf(q.z-h.y,_fmaxf(q.x*0.866025f-p.y*0.5f,-p.y)-h.x*0.5f);
}

__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r )
{
  float3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
  return length( pa - ba*h ) - r;
}

__DEVICE__ float sdEllipsoid(float3 p, float3 r)
{
    float k0 = length(p/r);
    float k1 = length(p/(r*r));
    return k0*(k0-1.0f)/k1;
}

__DEVICE__ float smin(float a, float b, float k) {
  float h = clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
  return _mix(b, a, h) - k * h * (1.0f - h);
}

__DEVICE__ float smax(float a, float b, float k) {
  float h = clamp( 0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f );
  return _mix(a, b, h) + k * h * (1.0f - h);
}

__DEVICE__ float2 near(float2 a, float2 b)
{
    float s = step(a.x,b.x);
    return s*a + (1.0f-s)*b;
}

__DEVICE__ float dfHandle(float3 p, float r)
{
    swi2(p,x,y) *= rot(r);
    float t = _fminf(sdTorus(p,to_float2(1.6f,0.3f)),
                  sdCapsule(p,to_float3(0.0f,0.0f,-0.3f),to_float3(0.0f,0.0f,4.0f),0.3f));
    t = _fminf(t,sdCapsule(p,to_float3(-2.0f,0.0f,0.0f),to_float3(2.0f,0.0f,0.0f),0.2f));
    t = _fminf(t,sdCapsule(p,to_float3(0.0f,-2.0f,0.0f),to_float3(0.0f,2.0f,0.0f),0.2f));
    return t;
}
__DEVICE__ float2 map(float3 p) 
{
    //backplane
    float t = _fminf(sdBox(p-to_float3(0.0f,0.0f,3.2f),to_float3(100,100,1)),
            sdTorus(p-to_float3(0.0f,13.0f,2.2f),to_float2(25.0f,0.2f)));
    t = _fmaxf(t,-sdBox(p-to_float3(0,1,0),to_float3(5.0f,7.0f,10.0f)));
    t = _fminf(t,sdBox(p-to_float3(-23.0f,2.0f,3.2f),to_float3(13.0f,6.0f,2.5f)));
    t = _fminf(t,sdBox(p-to_float3(-9.0f,6.5f,3.2f),to_float3(1.0f,1.5f,2.5f)));
    t = _fminf(t,sdBox(p-to_float3(-8.0f,-2.8f,2.4f),to_float3(1.8f,0.4f,2.0f)));  
    t = smin(t,sdBox(p-to_float3(-13.5f,-4.0f,2.0f),to_float3(1.0f,2.0f,2.0f)),0.5f);
    //side hinge
    float th = _fminf(sdCappedCylinder(p-to_float3(-8.0f,7.2f,1.0f),1.0f,1.0f),
                   sdBox(p-to_float3(-18.0f,7.2f,1.0f),to_float3(10.0f,1.0f,1.0f)));
    th = _fmaxf(th,-p.y-p.x*0.1f+5.0f);
    t = _fminf(t,th);
    //web
    t = _fminf(t,sdCappedCylinder(swi3(p,x,z,y)-to_float3(-10.0f,3.2f,5.0f),0.3f,4.0f));
    t = _fminf(t,sdBox(p-to_float3(-10.0f,0.0f,3.2f),to_float3(0.3f,5.0f,4.0f)));
    t = _fminf(t,sdBox(p-to_float3(-8.5f,5.0f,3.2f),to_float3(1.5f,0.3f,4.0f)));
    //FRONT DOOR
    float td = sdBox(p,to_float3(5.0f,5.0f,0.2f));
    td = _fmaxf(td,-sdCappedCone(p,1.0f,4.8f,3.4f));
    td = _fminf(td,sdTorus(p-to_float3(0.0f,0.0f,-0.2f),to_float2(4.4f,0.2f)));
    //bottom hinge
    td = smin(td,sdCappedCylinder(swi3(p,y,x,z)-to_float3(-5.5f,0.0f,0.5f),1.0f,5.0f),0.2f);
    //top hinge
    td = smin(td,sdCappedCylinder(swi3(p,y,x,z)-to_float3(5.25f,0.0f,0.0f),0.5f,5.0f),0.2f);
    float3 q = p-to_float3(2.0f,0.0f,0.0f)*clamp(round(p/to_float3(2.0f,0.0f,0.0f)),-2.0f,2.0f);
    td = _fmaxf(td,-sdCappedCylinder(swi3(q,y,x,z)-to_float3(5.25f,0.0f,0.0f),0.7f,0.5f));
    td = _fminf(td,sdCappedCylinder(swi3(q,y,x,z)-to_float3(5.25f,0.0f,0.0f),0.5f,0.42f));
    
    //clean edges????
    //td = _fmaxf(td,_fabs(p.x)-5.0f);
    
    q = p;
    q.x = _fabs(q.x);
    //backplane again
    t = _fminf(t,sdBox(q-to_float3(5.0f,20.0f,3.2f),to_float3(1.0f,14.0f,1.3f)));
    //rivets
    float metal = _fminf(sdCappedCylinder(swi3(q,x,z,y)-to_float3(5.0f,0.0f,5.0f),0.2f,1.2f),
                      sdCappedCylinder(swi3(q,x,z,y)-to_float3(4.0f,0.0f,5.0f),0.2f,1.2f));
    metal = _fminf(metal,sdCappedCylinder(swi3(q,x,z,y)-to_float3(5.0f,0.0f,4.2f),0.2f,1.2f));
    q.y = _fabs(q.y);
    td = _fminf(td,sdBox(q-to_float3(4.5f,3.0f,0.0f),to_float3(0.5f,0.3f,0.6f)));
    metal = _fminf(metal,sdCappedCylinder(swi3(q,x,z,y)-to_float3(4.5f,0.0f,3.0f),0.2f,0.8f));
    t = _fminf(t,td);
    //top bracket
    q = p - to_float3(0.0f,5.5f,0.0f);
    swi2(q,y,z) *= rot(0.35f);
    t = _fminf(t,sdBox(q-to_float3(0.0f,4.5f,0.0f),to_float3(4.0f,4.0f,0.1f)));
    float b = sdBox(q-to_float3(0.0f,1.8f,-0.7f),to_float3(5.45f,1.8f,0.2f));
    b = smax(b,sdCappedCylinder(swi3(q,x,z,y)-to_float3(0.0f,0.0f,0.0f),5.4f,1.0f),0.2f);
    b = smax(b,-sdCappedCylinder(swi3(q,x,z,y)-to_float3(0.0f,0.0f,0.0f),3.0f,1.0f),0.2f);
    b = smin(b,sdBox(to_float3(_fabs(p.x),swi2(p,y,z))-to_float3(4.2f,4.8f,-0.65f),to_float3(1.2f,1.0f,0.2f)),0.2f);
    t = _fminf(t,b-0.1f);
    //sides
    q = p;
    q.x = _fabs(q.x);
    t = _fminf(t,sdBox(q-to_float3(4.9f,-0.125f,0.0f),to_float3(0.1f,5.375f,0.5f)));
    t = _fminf(t,sdBox(q-to_float3(5.3f,-0.3f,1.5f),to_float3(0.15f,6.3f,2.1f)));
    t = _fminf(t,_fmaxf(sdTriPrism(swi3(q,y,z,x)-to_float3(-7.0f,0.0f,5.3f),to_float2(2.0f,0.15f)),p.z-3.0f));
    //handles
    t = _fminf(t,sdBox(to_float3_aw(p.x,_fabs(p.y-3.6f),p.z)-to_float3(13.0f,6.0f,2.8f),to_float3(3.0f,0.5f,1.0f)));
    metal = _fminf(metal,dfHandle(to_float3_aw(p.x,_fabs(p.y-3.6f),p.z)-to_float3(13.0f,6.0f,1.0f),0.3f));    
    //cut
    //top
    t = _fmaxf(t,-sdBox(p-to_float3(0.0f,5.0f,0.0f),to_float3(1.6f,2.0f,1.0f)));
    //bottom
    t = _fmaxf(t,-sdBox(p-to_float3(0.0f,-6.0f,0.0f),to_float3(1.6f,0.8f,2.0f)));
    metal = _fminf(metal,sdCappedCylinder(swi3(p,y,x,z)-to_float3(-5.5f,-0.5f,0.5f),0.6f,7.0f));    
    //bottom pistons
    swi2(q,y,z) *= rot(-0.1f);
    metal = _fminf(metal,sdCappedCylinder(q-to_float3(0.0f,-7.0f,0.0f),1.8f,0.2f));
    metal = _fminf(metal,sdCappedCylinder(q-to_float3(1.0f,-16.0f,0.0f),0.4f,9.0f));
    float white = sdCappedCylinder(q-to_float3(1.0f,-14.0f,0.0f),0.7f,5.0f);
    t = _fminf(t,sdCappedCylinder(q-to_float3(1.0f,-14.0f,0.0f),0.5f,5.4f));
    t = _fminf(t,sdCappedCylinder(q-to_float3(1.0f,-18.0f,0.0f),1.0f,0.2f));
    t = _fminf(t,sdBox(q-to_float3(0.0f,-18.0f,0.0f),to_float3(1.0f,0.2f,1.0f)));
    metal = smin(metal,sdBox(q-to_float3(0.0f,-6.5f,0.0f),to_float3(1.0f,0.5f,1.0f)),0.2f);
    white = smin(white,sdBox(q-to_float3(1.8f,-10.5f,0.0f),to_float3(0.4f,1.0f,0.6f)),0.2f); 
    //LEFT CYLINDER
    q = p - to_float3(-7.4f,-5.5f,0.5f);
    float tlc = smin(sdCappedCylinder(q-to_float3(0.0f,6.7f,0.0f),1.0f,3.0f)-0.5f,
                     sdCappedCylinder(q-to_float3(0.0f,5.0f,0.0f),1.6f,0.3f),0.1f);
    tlc = _fminf(tlc, sdCappedCylinder(swi3(q,y,z,x)-to_float3(2.6f,0.0f,0.0f),0.6f,0.8f));
    tlc = _fmaxf(tlc,-sdCappedCylinder(swi3(q,y,z,x)-to_float3(2.6f,0.0f,0.0f),0.5f,0.9f));
    metal = _fminf(metal,sdCappedCylinder(swi3(q,y,x,z),0.9f,0.7f));
    float tcc = smin(sdCappedCylinder(swi3(q,y,x,z),1.5f,0.5f),
                     sdCappedCylinder(swi3(q,y,x,z)-to_float3(1.5f,0.0f,0.0f),0.4f,0.5f),0.2f); 
    tcc = smin(tcc,sdCappedCylinder(swi3(q,y,x,z)-to_float3(-0.8f,0.0f,-1.2f),0.4f,0.5f),0.2f);
    tcc = _fmaxf(tcc,_fabs(q.x)-0.5f);
    metal = _fminf(metal,tcc);
    t = _fminf(t,tlc);
    //RIGHT CYLINDER
    q = p - to_float3(6.4f,-5.5f,0.5f);
    float trc = sdCappedCylinder(q-to_float3(0.0f,6.0f,0.0f),0.2f,2.0f)-0.3f;
    trc = _fmaxf(trc,-sdTorus(swi3(q,x,z,y)-to_float3(0.0f,0.0f,5.0f),to_float2(0.5f,0.1f)));
    trc = _fmaxf(trc,-sdTorus(swi3(q,x,z,y)-to_float3(0.0f,0.0f,7.0f),to_float2(0.5f,0.1f)));
    metal = _fminf(metal,sdCappedCylinder(q-to_float3(0.0f,6.0f,0.0f),0.2f,3.0f));
    trc = _fminf(trc,sdCappedCylinder(q-to_float3(0.0f,2.4f,0.0f),0.4f,0.5f));
    trc = _fmaxf(trc,-sdBox(q-to_float3(0.0f,1.2f,0.0f),to_float3(0.15f,1.2f,1.0f)));
    metal = _fminf(metal,sdBox(q-to_float3(0.0f,1.2f,0.0f),to_float3(0.1f,1.0f,0.3f)));
    metal = _fminf(metal,sdCappedCylinder(swi3(q,y,x,z),1.2f,0.06f));
    metal = _fminf(metal,sdCappedCylinder(swi3(q,y,x,z),1.0f,0.3f));
    //armature
    q.y -= 9.4f;
    trc = _fminf(trc,sdCappedCylinder(swi3(q,x,z,y),0.6f,0.6f));
    metal = _fminf(metal,sdCappedCylinder(swi3(q,x,z,y),0.3f,0.8f));
    trc = _fminf(trc,sdBox(q-to_float3(-0.8f,1.0f,0.0f),to_float3(0.5f,1.0f,0.2f)));
    swi2(q,x,y) *= rot(0.8f);
    trc = _fminf(trc,sdBox(q-to_float3(0.0f,0.0f,0.0f),to_float3(0.2f,1.2f,0.2f)));
    t = _fminf(t,trc);
    //wires
    q = p - to_float3(-9.0f,-10.0f,1.0f);
    q.x += _sinf(q.y*0.3f);
    q.z -= q.x*0.05f;
    swi2(q,x,y) *= rot(-0.5f);
    float tw = _fminf(max(sdTorus(q,to_float2(5.0f,0.2f)),q.y),
                   _fmaxf(sdTorus(q-to_float3(8.0f,0.0f,0.0f),to_float2(3.0f,0.2f)),-q.y));
    tw = _fminf(tw,_fmaxf(sdTorus(q-to_float3(-10.0f,0.0f,0.0f),to_float2(5.0f,0.2f)),-q.y));
    tw = _fmaxf(tw,q.x-7.0f);
    tw = _fmaxf(tw,-q.x-7.0f);
    q = p - to_float3(-9.0f,-12.0f,1.0f);
    q.z -= q.x*-0.05f;
    tw = _fminf(tw,_fmaxf(sdTorus(q,to_float2(4.3f,0.2f)),q.y));
    tw = _fminf(tw,_fmaxf(sdTorus(q-to_float3(6.3f,0.0f,0.0f),to_float2(2.0f,0.2f)),-q.y));    
    tw = _fminf(tw,sdCapsule(q,to_float3(-4.3f,0.0f,0.0f),to_float3(-4.3f,8.0f,0.0f),0.2f));
    tw = _fmaxf(tw,q.x-8.0f);
    //flesh
    q = p-to_float3(0,1,3);
    float nz = n3D(to_float3(p.x*8.1f,p.y,p.z*9.17f));
    nz *= smoothstep(1.0f,2.0f,_fabs(p.x))*smoothstep(4.0f,2.0f,_fabs(p.x))*
          smoothstep(4.0f,-1.6f,p.y)*smoothstep(-4.0f,-1.6f,p.y) *
          0.06f;
    float tf = sdEllipsoid(q-to_float3(0,0.6f,0),to_float3(2.6f+nz,4.8f+nz,1.6f+nz));
    tf = smin(tf,sdEllipsoid(q-to_float3(0,6,2.8f),to_float3(4.0f,8.0f,4.0f)),1.0f);
    q.x = _fabs(q.x);
    tf = smin(tf,sdEllipsoid(q-to_float3(5.0f,-2.0f,3.3f),to_float3(4,20,4)),2.0f);
    //cut
    nz = n3D(p)*0.3f;
    float tfc = sdEllipsoid(q-to_float3(0,0,-1),to_float3(0.5f,4.4f,4.0f));
    tfc = smin(tfc,sdEllipsoid(q-to_float3(0,1.6f,-0.4f),to_float3(0.4f+nz,1.0f+nz,4.2f)),0.1f);
    tfc = smin(tfc,sdEllipsoid(q-to_float3(0,-0.7f,-0.6f),to_float3(0.7f+nz,2.9f+nz,4.2f)),0.1f);
    tf = smax(tf,-tfc,0.2f);
    //join
    float tfl = sdEllipsoid(q-to_float3(0,0.6f,-0.7f),to_float3(0.4f,5.6f,1.0f));
    tfl = smin(tfl,sdEllipsoid(q-to_float3(0,1.6f,-0.6f),to_float3(0.4f+nz,1.0f+nz,1.2f)),0.1f);
    tfl = smin(tfl,sdEllipsoid(q-to_float3(0,-1.1f,-0.6f),to_float3(0.6f+nz,2.4f+nz,1.2f)),0.1f);
    tfl = smax(tfl,-sdEllipsoid(q-to_float3(0,-1.8f,-0.6f),to_float3(0.15f+nz,3.2f+nz,10.0f)),0.2f);
    tf = _fmaxf(tf,_fabs(p.x)-5.0f);
    tf = _fmaxf(tf,_fabs(p.y-1.0f)-7.0f); 
    float2 n = near(to_float2(t,1.0f),to_float2(metal,2.0f));
    n = near(n,to_float2(tw,3.0f));
    n = near(n,to_float2(tf,4.0f));
    n = near(n,to_float2(tfl,5.0f));
    return near(n,to_float2(white,6.0f));
}

__DEVICE__ float3 normal(float3 p) 
{  
    float4 n = to_float4_s(0.0f);
    for (int i=ZERO; i<4; i++) 
    {
        float4 s = to_float4_aw(p, 0.0f);
        s[i] += EPS;
        n[i] = map(swi3(s,x,y,z)).x;
    }
    return normalize(swi3(n,x,y,z)-n.w);
}

//IQ - https://iquilezles.org/articles/raymarchingdf
__DEVICE__ float AO(float3 p, float3 n) 
{
    float ra = 0.0f, w = 1.0f, d = 0.0f;
    for (int i=ZERO; i<5; i++){
        d = float(i) / 5.0f;
        ra += w * (d - map(p + n*d).x);
        if (ra>1.0f) break;
        w *= 0.5f;
    }
    return 1.0f - clamp(ra,0.0f,1.0f);
}

__DEVICE__ float2 march(float3 ro, float3 rd) 
{
    float t = 0.0f, id = 0.0f;   
    for (int i=ZERO; i<100; i++)
    {
        float2 ns = map(ro + rd*t);
        if (_fabs(ns.x)<EPS)
        {
            id = ns.y;
            break;
        }
        t += ns.x;
        if (t>FAR) 
        {
            t = -1.0f;
            break;
        }
        
    }
    return to_float2(t,id);
}

//IQ
//https://www.shadertoy.com/view/lsKcDD
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float mint, in float tmax, int technique )
{
  float res = 1.0f;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<64; i++ )
    {
    float h = map( ro + rd*t ).x;

        // traditional technique
        if( technique==0 )
        {
          res = _fminf( res, 10.0f*h/t );
        }
        // improved technique
        else
        {
            // use this if you are getting artifact on the first iteration, or unroll the
            // first iteration out of the loop
            //float y = (i==0) ? 0.0f : h*h/(2.0f*ph); 

            float y = h*h/(2.0f*ph);
            float d = _sqrtf(h*h-y*y);
            res = _fminf( res, 10.0f*d/_fmaxf(0.0f,t-y) );
            ph = h;
        }
        
        t += h;
        
        if( res<0.0001f || t>tmax ) break;
        
    }
    res = clamp( res, 0.0f, 1.0f );
    return res*res*(3.0f-2.0f*res);
}

__DEVICE__ float3 camera(float2 U, float3 ro, float3 la, float fl) 
{
    float2 uv = (U - R*0.5f) / R.y;
    float3 fwd = normalize(la-ro),
         rgt = normalize(to_float3(fwd.z,0.0f,-fwd.x));
    return normalize(fwd + fl*uv.x*rgt + fl*uv.y*cross(fwd, rgt));
}

__KERNEL__ void ThePassageXxixJipiFuse__Buffer_A(float4 C, float2 U, float iTime, float2 iResolution, int iFrame, sampler2D iChannel1)
{

    float3 pc = to_float3(0),
         la = to_float3(0,-5.0f,0),
         lp = to_float3(10,45,-40),
         ro = to_float3(0,-5.0f,-22);
    
    float3 rd = camera(U,ro,la,1.4f);
    
    float2 s = march(ro,rd);
    if (s.x>0.0f)
    {
        float3 p = ro + rd*s.x;
        float3 n = normal(p);
        float3 ld = normalize(lp - p);
        float3 ld2 = normalize(to_float3(10,45,-40)-p);
        float spg = 0.0f;
        float spm = 0.0f;
        
        float3 sc = to_float3_s(0.7f);
        if (s.y==1.0f)
        {
            spm = 0.2f+n3D(1.7f+p*0.7f)*0.4f;
            spg = 0.3f;
            sc = _mix(to_float3(0.7f,0.6f,0.8f),to_float3_s(0.7f),n3D(to_float3(p.x,p.y*0.3f,p.z)));
        }
        if (s.y==2.0f)
        {
            spg = 1.0f;
            sc = texture(iChannel1,reflect(rd,n)).xyz;
        }
        if (s.y==3.0f)
        {
            spm = 0.3f;
            sc = to_float3_s(0.2f);
        }
        if (s.y==4.0f)
        {
            spg = smoothstep(3.0f,0.0f,_fabs(p.x));
            spm = 0.2f;
            sc = _mix(to_float3(1.0f,0.8f,0.7f),
                     to_float3(1,0.3f,0.3f),
                     spg);
            sc = _mix(sc,to_float3(1.0f,0.8f,0.7f),n3D(p*3.0f));
            float nz = n3D(51.2f+to_float3(p.x*19.31f+_sinf(p.y*0.9f)*1.4f,p.y,p.z*17.37f));
            nz *= smoothstep(3.0f,1.0f,p.y)*smoothstep(3.0f,0.0f,_fabs(p.x));
            sc = _mix(sc,to_float3(0),_fminf(1.0f,nz*2.0f));
            sc *= _fmaxf(0.0f,dot(ld2,n));
        }
        if (s.y==5.0f)
        {
            spg = 1.0f;
            sc = _mix(to_float3(1,0.3f,0.3f),to_float3(1,0.7f,0.7f),n3D(p*3.0f));
            sc *= _fmaxf(0.0f,dot(ld2,n));
        }
        if (s.y==6.0f)
        {
            spg = 1.0f;
            sc = to_float3_aw(1);
        }
        float ao = AO(p,n);
        float specg = _powf(_fmaxf(dot(reflect(-ld,n),-rd),0.0f),32.0f);
        float specm = _powf(_fmaxf(dot(reflect(-ld,n),-rd),0.0f),4.0f);
        float sh = calcSoftshadow(p,ld,EPS,FAR,0);

        pc += sc * _fmaxf(0.0f,dot(ld,n));        
        //pc += to_float3(0,0,0.02f)*_fmaxf(0.0f,-n.y); 
        pc += to_float3(1)*specg*spg;
        pc += to_float3(1)*specm*spm;
        pc *= ao;
        pc *= sh;
        if (s.y==4.0||s.y==5.0f)
        {
            //fix shadow
            float ct = length(swi2(p,x,y)-to_float2(0.6f,-1.3f));
            pc *= smoothstep(0.01f,0.0f,ct-4.7f);
        }
    }
    C = to_float4_aw(pc,1.0f);


  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Created by SHAU - 2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//-----------------------------------------------------

/**
 * Still life of The Passage XXIX by H.R.Giger
 * https://wikioo.org/paintings.php?refarticle=A25TB8&titlepainting=hr+giger+passage+XXIX&artistname=H.R.+Giger
 */
 
#define R iResolution

__DEVICE__ float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

__KERNEL__ void ThePassageXxixJipiFuse(float4 C, float2 U, float2 iResolution, sampler2D iChannel0)
{

    float3 col = texture(iChannel0,U/R).xyz;
    if (U.x<R.x*0.25f || U.x>R.x*0.75f) col = to_float3(0.75f,0.7f,0.8f);
    col = _powf(ACESFilm(col),to_float3_s(0.3545f));
    C = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(C);
}
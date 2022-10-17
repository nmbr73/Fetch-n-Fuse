
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Copyright Inigo Quilez, 2019 - https://iquilezles.org/
// I am the sole copyright owner of this Work.
// You cannot host, display, distribute or share this Work neither
// as it is or altered, here on Shadertoy or anywhere else, in any
// form including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it or train a neural
// network with it without permission. I share this Work for educational
// purposes, and you can link to it, through an URL, proper attribution
// and unmodified screenshot, as part of your educational material. If
// these conditions are too restrictive please contact me and we'll
// definitely work it out.

// Basically the same as https://www.shadertoy.com/view/XlVcWz
// but optimized through symmetry so it only needs to evaluate
// four gears instead of 18.0f Also I made the gears with actual
// boxes rather than displacements, which creates an exact SDF
// allowing me to raymarch the scene at the speed of light, or
// in other words, without reducing the raymarching step size.
// Also I'm using a bounding volume to speed things up further
// so I can affor some nice ligthing and motion blur.
//
// Live streamed tutorial on this shader:
// PART 1: https://www.youtube.com/watch?v=sl9x19EnKng
// PART 2: https://www.youtube.com/watch?v=bdICU2uvOdU
//
// Video capture here: https://www.youtube.com/watch?v=ydTVmDBSGYQ
//
#if HW_PERFORMANCE==0
  #define AA 1
#else
  #define AA 2  // Set AA to 1 if your machine is too slow
#endif


// https://iquilezles.org/articles/smin
__DEVICE__ float smax( float a, float b, float k )
{
    float h = _fmaxf(k-_fabs(a-b),0.0f);
    return _fmaxf(a, b) + h*h*0.25f/k;
}

// https://iquilezles.org/articles/distfunctions
__DEVICE__ float sdSphere( in float3 p, in float r )
{
    return length(p)-r;
}

__DEVICE__ float sdVerticalSemiCapsule( float3 p, float h, float r )
{
    p.y = _fmaxf(p.y-h,0.0f);
    return length( p ) - r;
}

// https://iquilezles.org/articles/distfunctions2d
__DEVICE__ float sdCross( in float2 p, in float2 b, float r ) 
{
    p = abs_f2(p); p = (p.y>p.x) ? swi2(p,y,x) : swi2(p,x,y);
    
    float2  q = p - b;
    float k = _fmaxf(q.y,q.x);
    float2  w = (k>0.0f) ? q : to_float2(b.y-p.x,-k);
    
    return sign_f(k)*length(_fmaxf(w,to_float2_s(0.0f))) + r;
}

// https://www.shadertoy.com/view/MlycD3
__DEVICE__ float dot2( in float2 v ) { return dot(v,v); }
__DEVICE__ float sdTrapezoid( in float2 p, in float r1, float r2, float he )
{
    float2 k1 = to_float2(r2,he);
    float2 k2 = to_float2(r2-r1,2.0f*he);

    p.x = _fabs(p.x);
    float2 ca = to_float2(_fmaxf(0.0f,p.x-((p.y<0.0f)?r1:r2)), _fabs(p.y)-he);
    float2 cb = p - k1 + k2*clamp( dot(k1-p,k2)/dot2(k2), 0.0f, 1.0f );
    
    float s = (cb.x < 0.0f && ca.y < 0.0f) ? -1.0f : 1.0f;
    
    return s*_sqrtf( _fminf(dot2(ca),dot2(cb)) );
}

// https://iquilezles.org/articles/intersectors
__DEVICE__ float2 iSphere( in float3 ro, in float3 rd, in float rad )
{
  float b = dot( ro, rd );
  float c = dot( ro, ro ) - rad*rad;
  float h = b*b - c;
  if( h<0.0f ) return to_float2_s(-1.0f);
  h = _sqrtf(h);
  return to_float2(-b-h, -b+h );
}

//----------------------------------

__DEVICE__ float dents( in float2 q, in float tr, in float y )
{
    const float an = 6.283185f/12.0f;
    float fa = (_atan2f(q.y,q.x)+an*0.5f)/an;
    float sym = an*_floor(fa);
    float2 r = mul_mat2_f2(to_mat2(_cosf(sym),-_sinf(sym), _sinf(sym), _cosf(sym)) , q);
    
#if 1
    float d = length(_fmaxf(abs_f2(r-to_float2(0.17f,0))-tr*to_float2(0.042f,0.041f*y), to_float2_s(0.0f)));
#else
    float d = sdTrapezoid( swi2(r,y,x)-to_float2(0.0f,0.17f), 0.085f*y, 0.028f*y, tr*0.045f );
#endif

  return d - 0.005f*tr;
}

__DEVICE__ float4 gear(float3 q, float off, float time)
{
    {
    float an = 2.0f*time*sign_f(q.y) + off*6.283185f/24.0f;
    float co = _cosf(an), si = _sinf(an);
    swi2S(q,x,z, mul_mat2_f2(to_mat2(co,-si,si,co) , swi2(q,x,z)));
    }
    
    q.y = _fabs(q.y);
    
    float an2 = 2.0f*_fminf(1.0f-2.0f*_fabs(fract(0.5f+time/10.0f)-0.5f),1.0f/2.0f);
    float3 tr = _fminf( 10.0f*an2 - to_float3(4.0f,6.0f,8.0f), to_float3_s(1.0f));
    
    // ring
    float d = _fabs(length(swi2(q,x,z)) - 0.155f*tr.y) - 0.018f;

    // add dents
    float r = length(q);
    d = _fminf( d, dents(swi2(q,x,z),tr.z, r) );

    
    // slice it
    float de = -0.0015f*clamp(600.0f*_fabs(dot(swi2(q,x,z),swi2(q,x,z))-0.155f*0.155f),0.0f,1.0f);
    d = smax( d, _fabs(r-0.5f)-0.03f+de, 0.005f*tr.z );

    // add cross
    float d3 = sdCross( swi2(q,x,z), to_float2(0.15f,0.022f)*tr.y, 0.02f*tr.y );
    float2 w = to_float2( d3, _fabs(q.y-0.485f)-0.005f*tr.y );
    d3 = _fminf(_fmaxf(w.x,w.y),0.0f) + length(_fmaxf(w, to_float2_s(0.0f)))-0.003f*tr.y;
    d = _fminf( d, d3 ); 
        
    // add pivot
    d = _fminf( d, sdVerticalSemiCapsule( q, 0.5f*tr.x, 0.01f ));

    // base
    d = _fminf( d, sdSphere(q-to_float3(0.0f,0.12f,0.0f),0.025f) );
    
    return to_float4(d,q.x,q.z,q.y);
}

__DEVICE__ float2 rot( float2 v )
{
    return to_float2(v.x-v.y,v.y+v.x)*0.707107f;
}
    
__DEVICE__ float4 map( in float3 p, float time )
{
    // center sphere
    float4 d = to_float4( sdSphere(p,0.12f), p.x,p.y,p.z );
    
    // gears. There are 18, but we only evaluate 4    
    float3 qx = to_float3_aw(rot(swi2(p,z,y)),p.x); if(_fabs(qx.x)>_fabs(qx.y)) qx=swi3(qx,z,x,y);
    float3 qy = to_float3_aw(rot(swi2(p,x,z)),p.y); if(_fabs(qy.x)>_fabs(qy.y)) qy=swi3(qy,z,x,y);
    float3 qz = to_float3_aw(rot(swi2(p,y,x)),p.z); if(_fabs(qz.x)>_fabs(qz.y)) qz=swi3(qz,z,x,y);
    float3 qa = abs_f3(p); qa = (qa.x>qa.y && qa.x>qa.z) ? swi3(p,z,x,y) : 
                                (qa.z>qa.y             ) ? swi3(p,y,z,x) :
                                                           swi3(p,x,y,z);
    float4 t;
    t = gear( qa,0.0f,time ); if( t.x<d.x ) d=t;
    t = gear( qx,1.0f,time ); if( t.x<d.x ) d=t;
    t = gear( qz,1.0f,time ); if( t.x<d.x ) d=t;
    t = gear( qy,1.0f,time ); if( t.x<d.x ) d=t;
float zzzzzzzzzzzzzzzz;    
  return d;
}

#define ZERO 0 //_fminf(iFrame,0)

// https://iquilezles.org/articles/normalsSDF
__DEVICE__ float3 calcNormal( in float3 pos, in float time )
{
#if 0
    float2 e = to_float2(1.0f,-1.0f)*0.5773f;
    const float eps = 0.00025f;
    return normalize( swi3(e,x,y,y)*map( pos + swi3(e,x,y,y)*eps, time ).x + 
                      swi3(e,y,y,x)*map( pos + swi3(e,y,y,x)*eps, time ).x + 
                      swi3(e,y,x,y)*map( pos + swi3(e,y,x,y)*eps, time ).x + 
                      swi3(e,x,x,x)*map( pos + swi3(e,x,x,x)*eps, time ).x );
#else
    // klems's trick to prevent the compiler from inlining map() 4 times
    float3 n = to_float3_s(0.0f);
    for( int i=ZERO; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*map(pos+0.0005f*e,time).x;
    }
    return normalize(n);
#endif    
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor, in float time )
{
  float occ = 0.0f;
    float sca = 1.0f;
    for( int i=ZERO; i<5; i++ )
    {
        float h = 0.01f + 0.12f*(float)(i)/4.0f;
        float d = map( pos+h*nor, time ).x;
        occ += (h-d)*sca;
        sca *= 0.95f;
    }
    return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f );
}

// https://iquilezles.org/articles/rmshadows
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float k, in float time )
{
    float res = 1.0f;
    
    // bounding sphere
    float2 b = iSphere( ro, rd, 0.535f );
  if( b.y>0.0f )
    {
        // raymarch
        float tmax = b.y;
        float t    = _fmaxf(b.x,0.001f);
        for( int i=0; i<64; i++ )
        {
            float h = map( ro + rd*t, time ).x;
            res = _fminf( res, k*h/t );
            t += clamp( h, 0.012f, 0.2f );
            if( res<0.001f || t>tmax ) break;
        }
    }
    
    return clamp( res, 0.0f, 1.0f );
}

__DEVICE__ float4 intersect( in float3 ro, in float3 rd, in float time )
{
  float4 res = to_float4_s(-1.0f);
  
  // bounding sphere
  float2 tminmax = iSphere( ro, rd, 0.535f );
  if( tminmax.y>0.0f )
    {
        // raymarch
        float t = _fmaxf(tminmax.x,0.001f);
        for( int i=0; i<128 && t<tminmax.y; i++ )
        {
            float4 h = map(ro+t*rd,time);
            if( h.x<0.001f ) { res=to_float4(t,h.y,h.z,h.w); break; }
            t += h.x;
        }
    }
    
    return res;
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv =          ( cross(cu,cw) );
  return to_mat3_f3( cu, cv, cw );
}

__KERNEL__ void SphereGearsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0)
{

    float3 tot = to_float3_s(0.0f);
    
    #if AA>1
    for( int m=ZERO; m<AA; m++ )
    for( int n=ZERO; n<AA; n++ )
    {
        // pixel coordinates
        float2 o = to_float2((float)(m),(float)(n)) / (float)(AA) - 0.5f;
        float2 p = (2.0f*(fragCoord+o)-iResolution)/iResolution.y;
        float d = 0.5f*_sinf(fragCoord.x*147.0f)*_sinf(fragCoord.y*131.0f);
        float time = iTime - 0.5f*(1.0f/24.0f)*((float)(m*AA+n)+d)/(float)(AA*AA-1);
    #else    
        float2 p = (2.0f*fragCoord-iResolution)/iResolution.y;
        float time = iTime;
    #endif

      // camera  
        float an = 6.2831f*time/40.0f;
        float3 ta = to_float3( 0.0f, 0.0f, 0.0f );
        float3 ro = ta + to_float3( 1.3f*_cosf(an), 0.5f, 1.2f*_sinf(an) );
        
        ro += 0.005f*sin_f3(92.0f*time/40.0f+to_float3(0.0f,1.0f,3.0f));
        ta += 0.009f*sin_f3(68.0f*time/40.0f+to_float3(2.0f,4.0f,6.0f));
        
        // camera-to-world transformation
        mat3 ca = setCamera( ro, ta, 0.0f );
        
        // ray direction
        float fl = 2.0f;
        float3 rd = mul_mat3_f3(ca , normalize( to_float3_aw(p,fl) ));

        // background
        float3 col = to_float3_s(1.0f+rd.y)*0.03f;
        
        // raymarch geometry
        float4 tuvw = intersect( ro, rd, time );
        if( tuvw.x>0.0f )
        {
            // shading/lighting  
            float3 pos = ro + tuvw.x*rd;
            float3 nor = calcNormal(pos, time);
                        
            float3 te = 0.5f*swi3(texture( iChannel0, swi2(tuvw,y,z)*2.0f ),x,y,z)+
                        0.5f*swi3(texture( iChannel0, swi2(tuvw,y,w)*1.0f ),x,y,z);
            
            float3 mate = 0.22f*te;
            float len = length(pos);
            
            mate *= 1.0f + to_float3(2.0f,0.5f,0.0f)*(1.0f-smoothstep(0.121f,0.122f,len) ) ;
            
            float focc  = 0.1f+0.9f*clamp(0.5f+0.5f*dot(nor,pos/len),0.0f,1.0f);
                  focc *= 0.1f+0.9f*clamp(len*2.0f,0.0f,1.0f);
            float ks = clamp(te.x*1.5f,0.0f,1.0f);
            float3  f0 = mate;
            float kd = (1.0f-ks)*0.125f;
            
            float occ = calcAO( pos, nor, time ) * focc;
            
            col = to_float3_s(0.0f);
            
            // side
            {
            float3 lig = normalize(to_float3(0.8f,0.2f,0.6f));
            float dif = clamp( dot(nor,lig), 0.0f, 1.0f );
            float3  hal = normalize(lig-rd);
            float sha = 1.0f; 
            if( dif>0.001f ) sha = calcSoftshadow( pos+0.001f*nor, lig, 20.0f, time );
            float3  spe = _powf(clamp(dot(nor,hal),0.0f,1.0f),16.0f)*(f0+(1.0f-f0)*_powf(clamp(1.0f+dot(hal,rd),0.0f,1.0f),5.0f));
            col += kd*mate*2.0f*to_float3(1.00f,0.70f,0.50f)*dif*sha;
            col += ks*     2.0f*to_float3(1.00f,0.80f,0.70f)*dif*sha*spe*3.14f;
            }

            // top
            {
            float3 ref = reflect(rd,nor);
            float fre = clamp(1.0f+dot(nor,rd),0.0f,1.0f);
            float sha = occ;
            col += kd*mate*25.0f*to_float3(0.19f,0.22f,0.24f)*(0.6f + 0.4f*nor.y)*sha;
            col += ks*     25.0f*to_float3(0.19f,0.22f,0.24f)*sha*smoothstep( -1.0f+1.5f*focc, 1.0f-0.4f*focc, ref.y ) * (f0 + (1.0f-f0)*_powf(fre,5.0f));
            }
            
            // bottom
            {
            float dif = clamp(0.4f-0.6f*nor.y,0.0f,1.0f);
            col += kd*mate*5.0f*to_float3(0.25f,0.20f,0.15f)*dif*occ;
            }
        }
        
        // compress        
        // col = 1.2f*col/(1.0f+col);
        
        // vignetting
        col *= 1.0f-0.1f*dot(p,p);
        
        // gamma        
      tot += pow_f3(col,to_float3_s(0.45f) );
    #if AA>1
    }
    tot /= (float)(AA*AA);
    #endif

    // s-curve    
    tot = _fminf(tot,to_float3_s(1.0f));
    tot = tot*tot*(3.0f-2.0f*tot);
    
    // cheap dithering
    tot += _sinf(fragCoord.x*114.0f)*_sinf(fragCoord.y*211.1f)/512.0f;

    fragColor = to_float4_aw( tot, 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}
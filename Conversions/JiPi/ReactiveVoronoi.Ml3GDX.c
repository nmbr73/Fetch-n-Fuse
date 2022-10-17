
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/a6a1cf7a09adfed8c362492c88c30d74fb3d2f4f7ba180ba34b98556660fada1.mp3' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Created by genis sole - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 4.0f International.

// A remastered version of this can be found here: https://www.shadertoy.com/view/MtyGWK 
// Adds a better traversal, stronger lighting, softer shadows and AO.

#define PI 3.1416f

__DEVICE__ float2 hash2( float2 p )
{
    // procedural white noise  
  return fract_f2(sin_f2(to_float2(dot(p,to_float2(127.1f,311.7f)),
                                   dot(p,to_float2(269.5f,183.3f))))*43758.5453f);
}

// From http://www.iquilezles.org/www/articles/voronoilines/voronoilines.htm
__DEVICE__ float3 voronoi( in float2 _x, float iTime )
{
    float2 n = _floor(_x);
    float2 f = fract_f2(_x);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
    float2 mg, mr;

    float md = 8.0f;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        float2 g = to_float2((float)(i),(float)(j));
        float2 o = hash2( n + g );
    #ifdef ANIMATE
        o = 0.5f + 0.5f*_sinf( iTime + 6.2831f*o );
    #endif
        float2 r = g + o - f;
        float d = dot(r,r);

        if( d<md )
        {
            md = d;
            mr = r;
            mg = g;
        }
    }

    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    md = 8.0f;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        float2 g = mg + to_float2((float)(i),(float)(j));
        float2 o = hash2( n + g );
    #ifdef ANIMATE
        o = 0.5f + 0.5f*_sinf( iTime + 6.2831f*o );
    #endif  
        float2 r = g + o - f;

        if( dot(mr-r,mr-r)>0.00001f )
          md = _fminf( md, dot( 0.5f*(mr+r), normalize(r-mr) ) );
    }

    return to_float3( md, mr.x,mr.y );
}


// Modified version of the above iq's voronoi borders. 
// Returns the distance to the border in a given direction.
__DEVICE__ float3 voronoi( in float2 _x, in float2 dir)
{
    float2 n = _floor(_x);
    float2 f = fract_f2(_x);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
    float2 mg, mr;

    float md = 8.0f;
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        float2 g = to_float2((float)(i),(float)(j));
        float2 o = hash2( n + g );
        float2 r = g + o - f;
        float d = dot(r,r);

        if( d<md )
        {
            md = d;
            mr = r;
            mg = g;
        }
    }

    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    md = 1e5;
    for( int j=-2; j<=2; j++ )
    for( int i=-2; i<=2; i++ )
    {
        float2 g = mg + to_float2((float)(i),(float)(j));
        float2 o = hash2( n + g );
        float2 r = g + o - f;

    
     if( dot(r-mr,r-mr) > 1e-5 ) {
          float2 l = r-mr;
            
          if (dot(dir, l) > 1e-5) {
              md = _fminf(md, dot(0.5f*(mr+r), l)/dot(dir, l));
          }
      }
        
    }
    
    return to_float3( md, n.x+mg.x, n.y+mg.y);
}

__DEVICE__ bool IRayAABox(in float3 ro, in float3 rd, in float3 invrd, in float3 bmin, in float3 bmax, 
               out float3 *p0, out float3 *p1) 
{
    float3 t0 = (bmin - ro) * invrd;
    float3 t1 = (bmax - ro) * invrd;

    float3 tmin = _fminf(t0, t1);
    float3 tmax = _fmaxf(t0, t1);
    
    float fmin = _fmaxf(max(tmin.x, tmin.y), tmin.z);
    float fmax = _fminf(min(tmax.x, tmax.y), tmax.z);
    
    *p0 = ro + rd*fmin;
    *p1 = ro + rd*fmax;
 
    return fmax >= fmin;   
}

__DEVICE__ float3 AABoxNormal(float3 bmin, float3 bmax, float3 p) 
{
  
    //float3 n1 = -(1.0f - smoothstep(to_float3_s(0.0f), to_float3_s(0.03f), p - bmin));
    float3 n1 = (-1.0f + smoothstep(to_float3_s(0.0f), to_float3_s(0.03f), p - bmin));
    float3 n2 = (1.0f -  smoothstep(to_float3_s(0.0f), to_float3_s(0.03f), bmax - p));
    
    return normalize(n1 + n2);
}



// From http://iquilezles.org/www/articles/palettes/palettes.htm
__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__DEVICE__ float3 color(float2 p) {
    return pal(3.434f+(hash2(p).x*0.02f), 
               to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,0.7f,0.4f),to_float3(0.0f,0.15f,0.20f)  );
}

__DEVICE__ float disp(in float2 p, float3 scmin, __TEXTURE2D__ iChannel0) {
    return scmin.y + 0.1f + hash2(p).x * 0.5f + texture(iChannel0, to_float2(hash2(p).x, 0.0f)).x*2.0f;
}

__DEVICE__ float4 map(in float2 p, in float2 dir, float3 scmin, __TEXTURE2D__ iChannel0) {
    
    float3 v = voronoi(p*2.0f, dir)*0.5f;
    return to_float4_aw(v, disp(swi2(v,y,z),scmin,iChannel0));
}

__DEVICE__ float ShadowFactor(in float3 ro, in float3 rd, float3 scmin, float3 scmax, __TEXTURE2D__ iChannel0) {
    float3 p0 = to_float3_s(0.0f);
    float3 p1 = to_float3_s(0.0f);
    
    IRayAABox(ro, rd, 1.0f/rd, scmin, scmax, &p0, &p1);
    p0 = ro + rd*0.02f;
    
    float2 dir = normalize(swi2(rd,x,z));
    float sf = rd.y / length(swi2(rd,x,z));

    float m = -1e5;
    
    const int max_steps = 32;
    for (int i = max_steps; i > 0; --i) {
        if (p0.y < m) break;
        
        if (dot((p1 - p0), rd) < 0.0f) return 1.0f;
  
        float4 v = map(swi2(p0,x,z), dir,scmin,iChannel0);
        
        m = v.w;
        if (p0.y < m) return 0.0f;
        
        p0 += rd*(length(to_float2(v.x, v.x*sf)) + 0.02f);
    }
    
    p0 += rd * (m - p0.y)/rd.y;
    if (dot((p1 - p0), rd) < 0.0f) return 1.0f;   
    
    return 0.0f;
}

__DEVICE__ float3 Shade(in float3 p, in float3 n, in float3 ld, in float2 c, float3 scmin, float3 scmax, __TEXTURE2D__ iChannel0) {
    float3 col = color(c);
  return (col * 0.15f + col * _fmaxf(0.0f, dot(n,ld)) * ShadowFactor(p, ld,scmin,scmax,iChannel0) * 0.85f) * 3.5f;
}

__DEVICE__ float3 Render(in float3 ro, in float3 rd, in float3 ld, float3 scmin, float3 scmax, float3 background, __TEXTURE2D__ iChannel0,float iTime) {
    float3 p0 = to_float3_s(0.0f);
    float3 p1 = to_float3_s(0.0f);
    
    if (!IRayAABox(ro, rd, 1.0f/rd, scmin, scmax, &p0, &p1)) return background;
    
    float2 dir = normalize(swi2(rd,x,z));
    float sf = rd.y / length(swi2(rd,x,z));
    
    float2 lvp = to_float2_s(0);
    float2 vp = swi2(p0,x,z);
    
    float m = -1e5;
    
    float3 n = to_float3_s(0.0f);
    
    const int max_steps = 32;
    for (int i = max_steps; i > 0; --i) {
        if (p0.y < m) {
            n = to_float3(0.0f, 1.0f, 0.0f);
            break;
        }
        
        if (dot((p1 - p0), rd) < 0.0f) return background;
  
        float4 v = map(swi2(p0,x,z), dir,scmin,iChannel0);
    
        lvp = vp;
        vp = swi2(v,y,z);
        
        m = v.w;
        if (p0.y < m) break;
        
        p0 += rd*(length(to_float2(v.x, v.x*sf)) + 0.02f);
    }
    
    
    
    if (n.y != 0.0f) {
      p0 += rd * (-p0.y + m)/rd.y;
        if (dot((p1 - p0), rd) < 0.0f) return background;
    }

    n = normalize(_mix(swi3(to_float3_aw(normalize(lvp - vp), 0.0f),x,z,y), n, 
                  smoothstep(0.00f, 0.03f, voronoi(swi2(p0,x,z)*2.0f,iTime).x*0.5f)));
    
    //if (all(equal(swi2(p0,x,z), lvp))) {
    if (p0.x == lvp.x && p0.z == lvp.y) {
      n = AABoxNormal(scmin, scmax, p0); 
    }
    
    return Shade(p0, n, ld, vp,scmin,scmax,iChannel0);
}

__DEVICE__ void CameraOrbitRay(in float2 fragCoord, in float n, in float3 c, in float d, 
                    out float3 *ro, out float3 *rd, out mat3 *t, float2 iResolution, float4 iMouse) 
{
    float a = 1.0f/_fmaxf(iResolution.x, iResolution.y);
    *rd = normalize(to_float3_aw((fragCoord - iResolution*0.5f)*a, n));
 
    *ro = to_float3(0.0f, 0.0f, -d);
    
    float ff = _fminf(1.0f, step(0.001f, iMouse.x) + step(0.001f, iMouse.y));
    float2 m = PI*ff + (((swi2(iMouse,x,y) + 0.1f) / iResolution) * (PI*2.0f));
    m.y = -m.y;
    m.y = _sinf(m.y*0.5f)*0.6f + 0.6f;
        
    mat3 rotX = to_mat3(1.0f, 0.0f, 0.0f, 0.0f, _cosf(m.y), _sinf(m.y), 0.0f, -_sinf(m.y), _cosf(m.y));
    mat3 rotY = to_mat3(_cosf(m.x), 0.0f, -_sinf(m.x), 0.0f, 1.0f, 0.0f, _sinf(m.x), 0.0f, _cosf(m.x));
    
    *t = mul_mat3_mat3(rotY , rotX);
    
    *ro = mul_mat3_f3(*t , *ro);
    *ro = c + *ro;

    *rd = mul_mat3_f3(*t , *rd);
    
    *rd = normalize(*rd);
}

__DEVICE__ float3 LightDir(in mat3 t) 
{
    float3 l = normalize(to_float3(1.0f, 1.0f, -1.0f));
    return mul_mat3_f3(t , l);
}

__KERNEL__ void ReactiveVoronoiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    const float3 background = to_float3_s(0.04f);
    const float3 scmin = -1.0f*to_float3(1.77f, 1.0f, 1.77f);
    const float3 scmax = to_float3(1.77f, 1.5f, 1.77f);

    float3 ro = to_float3_s(0.0f);
    float3 rd = to_float3_s(0.0f);
    mat3 t = to_mat3_f(1.0f);
    
    CameraOrbitRay(fragCoord, 1.0f, to_float3_s(0.0f), 10.0f, &ro, &rd, &t, iResolution, iMouse);
    fragColor = to_float4_aw(pow_f3(Render(ro, rd, LightDir(t),scmin,scmax,background,iChannel0,iTime), to_float3_s(0.5454f)), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
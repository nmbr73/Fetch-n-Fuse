
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Copyright Inigo Quilez, 2013 - https://iquilezles.org/
// I am the sole copyright owner of this Work.
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.

// Shading technique explained here:
//
// https://iquilezles.org/articles/voxellines



// consider replacing this by a proper noise function
__DEVICE__ float noise( in float3 x, __TEXTURE2D__ iChannel0 )
{
  float3 p = _floor(x);
  float3 f = fract_f3(x);
  f = f*f*(3.0f-2.0f*f);
  float2 uv = (swi2(p,x,y)+to_float2(37.0f,17.0f)*p.z) + swi2(f,x,y);
  float2 rg = swi2(texture( iChannel0, (uv+0.5f)/256.0f),y,x);
  return _mix( rg.x, rg.y, f.z );
}

__DEVICE__ float mapTerrain( float3 p, float iTime, __TEXTURE2D__ iChannel0 )
{
  p *= 0.1f; 
  //swi2(p,x,z) *= 0.6f;
  p.x *= 0.6f;
  p.z *= 0.6f;
  
  float time = 0.5f + 0.15f*iTime;
  float ft = fract( time );
  float it = _floor( time );
  ft = smoothstep( 0.7f, 1.0f, ft );
  time = it + ft;
  float spe = 1.4f;
  
  float f;
    f  = 0.5000f*noise( p*1.00f + to_float3(0.0f,1.0f,0.0f)*spe*time, iChannel0 );
    f += 0.2500f*noise( p*2.02f + to_float3(0.0f,2.0f,0.0f)*spe*time, iChannel0 );
    f += 0.1250f*noise( p*4.01f, iChannel0 );
  return 25.0f*f-10.0f;
}



__DEVICE__ float map(in float3 c, float iTime, float3 gro, __TEXTURE2D__ iChannel0) 
{
  float3 p = c + 0.5f;
  
  float f = mapTerrain( p, iTime, iChannel0 ) + 0.25f*p.y;

  f = _mix( f, 1.0f, step( length(gro-p), 5.0f ) );

  return step( f, 0.5f );
}



__DEVICE__ float castRay( in float3 ro, in float3 rd, out float3 *oVos, out float3 *oDir, float iTime, float3 gro, __TEXTURE2D__ iChannel0 )
{
  float3 pos = _floor(ro);
  float3 ri = 1.0f/rd;
  float3 rs = sign_f3(rd);
  float3 dis = (pos-ro + 0.5f + rs*0.5f) * ri;
  
  float res = -1.0f;
  float3 mm = to_float3_s(0.0f);
  for( int i=0; i<128; i++ ) 
  {
    if( map(pos, iTime, gro, iChannel0)>0.5f ) { res=1.0f; break; }
    mm = step(swi3(dis,x,y,z), swi3(dis,y,z,x)) * step(swi3(dis,x,y,z), swi3(dis,z,x,y));
    dis += mm * rs * ri;
    pos += mm * rs;
  }

  float3 nor = -mm*rs;
  float3 vos = pos;
  
  // intersect the cube  
  float3 mini = (pos-ro + 0.5f - 0.5f*(rs))*ri;
  float t = _fmaxf ( mini.x, _fmaxf ( mini.y, mini.z ) );
  
  *oDir = mm;
  *oVos = vos;

  return t*res;
}

__DEVICE__ float3 path( float t, float ya )
{
  float2 p  = 100.0f*sin_f2( 0.02f*t*to_float2(1.0f,1.2f) + to_float2(0.1f,0.9f) );
  p +=  50.0f*sin_f2( 0.04f*t*to_float2(1.3f,1.0f) + to_float2(1.0f,4.5f) );
  
  return to_float3( p.x, 18.0f + ya*4.0f*_sinf(0.05f*t), p.y );
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv = normalize( cross(cu,cw) );
  return to_mat3_f3( cu, cv, -cw );
}

__DEVICE__ float maxcomp( in float4 v )
{
  return _fmaxf( _fmaxf(v.x,v.y), _fmaxf(v.z,v.w) );
}

__DEVICE__ float isEdge( in float2 uv, float4 va, float4 vb, float4 vc, float4 vd )
{
  float2 st = 1.0f - uv;

  // edges
  float4 wb = smoothstep( to_float4_s(0.85f), to_float4_s(0.99f), to_float4(uv.x,
                                           st.x,
                                           uv.y,
                                           st.y) ) * ( to_float4_s(1.0f) - va + va*vc );
  // corners
  float4 wc = smoothstep( to_float4_s(0.85f), to_float4_s(0.99f), to_float4(uv.x*uv.y,
                                           st.x*uv.y,
                                           st.x*st.y,
                                           uv.x*st.y) ) * ( to_float4_s(1.0f) - vb + vd*vb );
  return maxcomp( _fmaxf(wb,wc) );
}

__DEVICE__ float calcOcc( in float2 uv, float4 va, float4 vb, float4 vc, float4 vd )
{
    float2 st = 1.0f - uv;

    // edges
    float4 wa = to_float4( uv.x, st.x, uv.y, st.y ) * vc;

    // corners
    float4 wb = to_float4(uv.x*uv.y,
                   st.x*uv.y,
                   st.x*st.y,
                   uv.x*st.y)*vd*(to_float4_s(1.0f)-swi4(vc,x,z,y,w))*(to_float4_s(1.0f)-swi4(vc,z,y,w,x));
    
    return wa.x + wa.y + wa.z + wa.w +
           wb.x + wb.y + wb.z + wb.w;
}

__DEVICE__ float3 render( in float3 ro, in float3 rd, float iTime, float3 gro, __TEXTURE2D__ iChannel0 )
{
  float3 col = to_float3_s(0.0f);
    
  float3 lig = normalize( to_float3(-0.4f,0.3f,0.7f) );
  
    // raymarch  
  float3 vos, dir;
  float t = castRay( ro, rd, &vos, &dir, iTime, gro, iChannel0 );
  if( t>0.0f )
  {
    float3 nor = -dir*sign_f3(rd);
    float3 pos = ro + rd*t;
    float3 uvw = pos - vos;
    
    float3 v1  = vos + nor + swi3(dir,y,z,x);
    float3 v2  = vos + nor - swi3(dir,y,z,x);
    float3 v3  = vos + nor + swi3(dir,z,x,y);
    float3 v4  = vos + nor - swi3(dir,z,x,y);
    float3 v5  = vos + nor + swi3(dir,y,z,x) + swi3(dir,z,x,y);
    float3 v6  = vos + nor - swi3(dir,y,z,x) + swi3(dir,z,x,y);
    float3 v7  = vos + nor - swi3(dir,y,z,x) - swi3(dir,z,x,y);
    float3 v8  = vos + nor + swi3(dir,y,z,x) - swi3(dir,z,x,y);
    float3 v9  = vos + swi3(dir,y,z,x);
    float3 v10 = vos - swi3(dir,y,z,x);
    float3 v11 = vos + swi3(dir,z,x,y);
    float3 v12 = vos - swi3(dir,z,x,y);
    float3 v13 = vos + swi3(dir,y,z,x) + swi3(dir,z,x,y); 
    float3 v14 = vos - swi3(dir,y,z,x) + swi3(dir,z,x,y) ;
    float3 v15 = vos - swi3(dir,y,z,x) - swi3(dir,z,x,y);
    float3 v16 = vos + swi3(dir,y,z,x) - swi3(dir,z,x,y);

    float4 vc = to_float4( map(v1, iTime, gro, iChannel0),  map(v2, iTime, gro, iChannel0),  map(v3, iTime, gro, iChannel0),  map(v4, iTime, gro, iChannel0)  );
    float4 vd = to_float4( map(v5, iTime, gro, iChannel0),  map(v6, iTime, gro, iChannel0),  map(v7, iTime, gro, iChannel0),  map(v8, iTime, gro, iChannel0)  );
    float4 va = to_float4( map(v9, iTime, gro, iChannel0),  map(v10, iTime, gro, iChannel0), map(v11, iTime, gro, iChannel0), map(v12, iTime, gro, iChannel0) );
    float4 vb = to_float4( map(v13, iTime, gro,iChannel0), map(v14, iTime, gro, iChannel0), map(v15, iTime, gro,  iChannel0), map(v16, iTime, gro, iChannel0) );
    
    float2 uv = to_float2( dot(swi3(dir,y,z,x), uvw), dot(swi3(dir,z,x,y), uvw) );
      
    // wireframe
    float www = 1.0f - isEdge( uv, va, vb, vc, vd );

    float3 wir = smoothstep( to_float3_s(0.4f), to_float3_s(0.5f), abs_f3(uvw-0.5f) );
    float vvv = (1.0f-wir.x*wir.y)*(1.0f-wir.x*wir.z)*(1.0f-wir.y*wir.z);

    col = to_float3_s(0.5f);
    col += 0.8f*to_float3(0.1f,0.3f,0.4f);
    col *= 1.0f - 0.75f*(1.0f-vvv)*www;

    // lighting
    float dif = clamp( dot( nor, lig ), 0.0f, 1.0f );
    float bac = clamp( dot( nor, normalize(lig*to_float3(-1.0f,0.0f,-1.0f)) ), 0.0f, 1.0f );
    float sky = 0.5f + 0.5f*nor.y;
    float amb = clamp(0.75f + pos.y/25.0f,0.0f,1.0f);
    float occ = 1.0f;

    // ambient occlusion
    occ = calcOcc( uv, va, vb, vc, vd );
    occ = 1.0f - occ/8.0f;
    occ = occ*occ;
    occ = occ*occ;
    occ *= amb;

    // lighting
    float3 lin = to_float3_s(0.0f);
    lin += 2.5f*dif*to_float3(1.00f,0.90f,0.70f)*(0.5f+0.5f*occ);
    lin += 0.5f*bac*to_float3(0.15f,0.10f,0.10f)*occ;
    lin += 2.0f*sky*to_float3(0.40f,0.30f,0.15f)*occ;

    // line glow  
    float lineglow = 0.0f;
    lineglow += smoothstep( 0.4f, 1.0f,      uv.x )*(1.0f-va.x*(1.0f-vc.x));
    lineglow += smoothstep( 0.4f, 1.0f, 1.0f-uv.x )*(1.0f-va.y*(1.0f-vc.y));
    lineglow += smoothstep( 0.4f, 1.0f,      uv.y )*(1.0f-va.z*(1.0f-vc.z));
    lineglow += smoothstep( 0.4f, 1.0f, 1.0f-uv.y )*(1.0f-va.w*(1.0f-vc.w));
    lineglow += smoothstep( 0.4f, 1.0f,      uv.y*      uv.x )*(1.0f-vb.x*(1.0f-vd.x));
    lineglow += smoothstep( 0.4f, 1.0f,      uv.y* (1.0f-uv.x))*(1.0f-vb.y*(1.0f-vd.y));
    lineglow += smoothstep( 0.4f, 1.0f, (1.0f-uv.y)*(1.0f-uv.x))*(1.0f-vb.z*(1.0f-vd.z));
    lineglow += smoothstep( 0.4f, 1.0f, (1.0f-uv.y)*     uv.x )*(1.0f-vb.w*(1.0f-vd.w));

    float3 linCol = 2.0f*to_float3(5.0f,0.6f,0.0f);
    linCol *= (0.5f+0.5f*occ)*0.5f;
    lin += 3.0f*lineglow*linCol;

    col = col*lin;
    col += 8.0f*linCol*to_float3(1.0f,2.0f,3.0f)*(1.0f-www);//*(0.5f+1.0f*sha);
    col += 0.1f*lineglow*linCol;
    col *= _fminf(0.1f,_expf( -0.07f*t ));

    // blend to black & white    
    float3 col2 = to_float3_s(1.3f)*(0.5f+0.5f*nor.y)*occ*www*(0.9f+0.1f*vvv)*_expf( -0.04f*t );;
    float mi = _sinf(-1.57f+0.5f*iTime);
    mi = smoothstep( 0.70f, 0.75f, mi );
    col = _mix( col, col2, mi );
  }

  // gamma  
  col = pow_f3( col, to_float3_s(0.45f) );

  return col;
}

__KERNEL__ void VoxelEdgesFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
  CONNECT_SLIDER0(ViewZ, -100.0f, 100.0f, 0.0f);

  float3 gro = to_float3_s(0.0f);

  // inputs  
  float2 q = fragCoord / iResolution;
  float2 p = -1.0f + 2.0f*q;
  p.x *= iResolution.x/ iResolution.y;

  float2 mo = swi2(iMouse,x,y) / iResolution;
  if( iMouse.z <= 0.00001f ) mo = to_float2_s(0.0f);
  
  float time = 2.0f*iTime + 50.0f*mo.x;
  // camera
  float cr = 0.2f*_cosf(0.1f*iTime);
  float3 ro = path( time+0.0f, 1.0f ) + to_float3_aw(ViewXY, ViewZ);
  float3 ta = path( time+5.0f, 1.0f ) - to_float3(0.0f,6.0f,0.0f);
  gro = ro;

  mat3 cam = setCamera( ro, ta, cr );
  
  // build ray
  float r2 = p.x*p.x*0.32f + p.y*p.y;
  p *= (7.0f-_sqrtf(37.5f-11.5f*r2))/(r2+1.0f);
  float3 rd = normalize( mul_mat3_f3(cam , to_float3_aw(swi2(p,x,y),-2.5f) ));

  float3 col = render( ro, rd, iTime, gro, iChannel0 );
    
  // vignetting  
  col *= 0.5f + 0.5f*_powf( 16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.1f );
  
  fragColor = to_float4_aw( col, 1.0f );
  
  SetFragmentShaderComputedColor(fragColor);
}


#ifdef VR
__DEVICE__ void mainVR( out float4 fragColor, in float2 fragCoord, in float3 fragRayOri, in float3 fragRayDir )
{
  float time = 1.0f*iTime;

  float cr = 0.0f;
  float3 ro = path( time+0.0f, 0.0f ) + to_float3(0.0f,0.7f,0.0f);
  float3 ta = path( time+2.5f, 0.0f ) + to_float3(0.0f,0.7f,0.0f);

  mat3 cam = setCamera( ro, ta, cr );
  float3 col = render( ro + cam*fragRayOri, cam*fragRayDir );
    
  fragColor = to_float4_aw( col, 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}
#endif
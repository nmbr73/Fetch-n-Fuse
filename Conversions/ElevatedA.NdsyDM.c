
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect 'Texture: Gray Noise Medium' to iChannel0


// Copyright Inigo Quilez, 2016 - https://iquilezles.org/
// I am the sole copyright owner of this Work.
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.

// on the derivatives based noise: http://iquilezles.org/www/articles/morenoise/morenoise.htm
// on the soft shadow technique: http://iquilezles.org/www/articles/rmshadows/rmshadows.htm
// on the fog calculations: http://iquilezles.org/www/articles/fog/fog.htm
// on the lighting: http://iquilezles.org/www/articles/outdoorslighting/outdoorslighting.htm
// on the raymarching: http://iquilezles.org/www/articles/terrainmarching/terrainmarching.htm


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define AA 1   // make this 2 or even 3 if you have a really powerful GPU

#define USE_SMOOTH_NOISE 0   // enable to prevent discontinuities

#define SC (250.0f)

// value noise, and its analytical derivatives
__DEVICE__ float3 noised( in float2 x, __TEXTURE2D__ iChannel0 )
{
    float2 f = fract(x);
    #if USE_SMOOTH_NOISE==0
    float2 u = f*f*(3.0f-2.0f*f);
    float2 du = 6.0f*f*(1.0f-f);
    #else
    float2 u = f*f*f*(f*(f*6.0f-15.0f)+10.0f);
    float2 du = 30.0f*f*f*(f*(f-2.0f)+1.0f);
    #endif


  // texture version    
  float2 p = _floor(x);
  float a = texture( iChannel0, (p+to_float2(0.5f,0.5f))/256.0f ).x;
  float b = texture( iChannel0, (p+to_float2(1.5f,0.5f))/256.0f ).x;
  float c = texture( iChannel0, (p+to_float2(0.5f,1.5f))/256.0f ).x;
  float d = texture( iChannel0, (p+to_float2(1.5f,1.5f))/256.0f ).x;

  float2 tmp = du*(to_float2(b-a,c-a)+(a-b-c+d)*swi2(u,y,x)); 
  return to_float3(a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y,tmp.x,tmp.y);
}

// value noise, its analytical derivatives, and its second analytical derivatives
__DEVICE__ float3 noisedd( in float2 x, out float4 *dd, __TEXTURE2D__ iChannel0 )
{
    float2 f = fract(x);
    #if USE_SMOOTH_NOISE==0
    float2 u = f*f*(3.0f-2.0f*f);
    float2 du = 6.0f*f*(1.0f-f);
    float2 ddu = 6.0f-12.0f*f;
    #else
    float2 u = f*f*f*(f*(f*6.0f-15.0f)+10.0f);
    float2 du = 30.0f*f*f*(f*(f-2.0f)+1.0f);
    float2 ddu = 60.0f*f*(f*(2.0f*f-3.0f)+1.0f);
    #endif


  // texture version    
  float2 p = _floor(x);
  float a = texture( iChannel0, (p+to_float2(0.5f,0.5f))/256.0f ).x;
  float b = texture( iChannel0, (p+to_float2(1.5f,0.5f))/256.0f ).x;
  float c = texture( iChannel0, (p+to_float2(0.5f,1.5f))/256.0f ).x;
  float d = texture( iChannel0, (p+to_float2(1.5f,1.5f))/256.0f ).x;

    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k4 = a - b - c + d;

    float v = du.y*k4*du.x;
    *dd = to_float4(
        // (n/dx)/dx
        ddu.x*u.y*k4+ddu.x*k1,
        // (n/dx)/dy
        v,
        // (n/dy)/dx
        v,
        // (n/dy)/dy
        ddu.y*u.x*k4+ddu.y*k2);
    
    float2 tmp = du*(to_float2(k1,k2)+k4*swi2(u,y,x));
  return to_float3(k0+k1*u.x+k2*u.y+k4*u.x*u.y, tmp.x,tmp.y);
}

#define _m2 const mat2 m2 = to_mat2(0.8f,-0.6f,0.6f,0.8f)
// transpose of m2
#define _m2t const mat2 m2t = to_mat2(0.8f,0.6f,-0.6f,0.8f)


__DEVICE__ float terrainH( in float2 x, __TEXTURE2D__ iChannel0 )
{
  _m2;
  
  float2  p = x*0.003f/SC;
  float a = 0.0f;
  float b = 1.0f;
  float2  d = to_float2_s(0.0f);
  for( int i=0; i<16; i++ )
    {
      float3 n = noised(p,iChannel0);
      d += swi2(n,y,z);
      a += b*n.x/(1.0f+dot(d,d));
      b *= 0.5f;
      p = mul_mat2_f2(m2,p*2.0f);
    }

    #if USE_SMOOTH_NOISE==1
    a *= 0.9f;
    #endif
  return SC*120.0f*a;
}

__DEVICE__ float terrainM( in float2 x, __TEXTURE2D__ iChannel0 )
{
  _m2;
  float2  p = x*0.003f/SC;
  float a = 0.0f;
  float b = 1.0f;
  float2  d = to_float2_s(0.0f);
  for( int i=0; i<9; i++ )
    {
      float3 n = noised(p,iChannel0);
      d += swi2(n,y,z);
      a += b*n.x/(1.0f+dot(d,d));
      b *= 0.5f;
      p = mul_mat2_f2(m2,p*2.0f);
    }
    #if USE_SMOOTH_NOISE==1
    a *= 0.9f;
    #endif
  return SC*120.0f*a;
}

__DEVICE__ float terrainL( in float2 x, __TEXTURE2D__ iChannel0 )
{
  _m2;
  float2  p = x*0.003f/SC;
  float a = 0.0f;
  float b = 1.0f;
  float2  d = to_float2_s(0.0f);
  for( int i=0; i<3; i++ )
    {
      float3 n = noised(p,iChannel0);
      d += swi2(n,y,z);
      a += b*n.x/(1.0f+dot(d,d));
      b *= 0.5f;
      p = mul_mat2_f2(m2,p*2.0f);
    }
    #if USE_SMOOTH_NOISE==1
    a *= 0.9f;
    #endif
  return SC*120.0f*a;
}

__DEVICE__ float raycast( in float3 ro, in float3 rd, in float tmin, in float tmax, __TEXTURE2D__ iChannel0 )
{
  float t = tmin;
  for( int i=0; i<300; i++ )
  {
    float3 pos = ro + t*rd;
    float h = pos.y - terrainM( swi2(pos,x,z), iChannel0 );
    if( _fabs(h)<(0.0015f*t) || t>tmax ) break;
    t += 0.4f*h;
  }

  return t;
}

__DEVICE__ float softShadow(in float3 ro, in float3 rd, float dis, __TEXTURE2D__ iChannel0 )
{
  float minStep = clamp(dis*0.01f,SC*0.5f,SC*50.0f);

  float res = 1.0f;
  float t = 0.001f;
  for( int i=0; i<80; i++ )
  {
      float3  p = ro + t*rd;
        float h = p.y - terrainM( swi2(p,x,z),iChannel0 );
    res = _fminf( res, 16.0f*h/t );
    t += _fmaxf(minStep,h);
    if( res<0.001f ||p.y>(SC*200.0f) ) break;
  }
  return clamp( res, 0.0f, 1.0f );
}

__DEVICE__ float2 terrainHd( in float2 p, __TEXTURE2D__ iChannel0 )
{
  _m2;
  _m2t;
  
  float b = 1.0f;
  float2  e = to_float2_s(0.0f);
  float2  d = to_float2_s(0.0f);
  float s = 0.5f;
  float f = 2.0f;
  float4  r = to_float4_s(0.0f);
  mat2  m = to_mat2(1.0f,0.0f,0.0f,1.0f);
  for( int i=0; i<16; i++ )
    {
        float4 dd;
        float3 n = noisedd(p,&dd, iChannel0);
        float2 duxy = mul_mat2_f2(m,swi2(n,y,z));
        r += to_float4_f2f2(mul_mat2_f2(m,swi2(dd,x,y)),mul_mat2_f2(m,swi2(dd,z,w)));
        e += swi2(n,y,z);
        float term = 1.0f+dot(e,e);
        float x = 2.0f*(e.x*r.x+e.y*r.z);
        float y = 2.0f*(e.x*r.y+e.y*r.w);
        d += b*(term*duxy-n.x*to_float2(x, y))/(term*term);
        b *= s;
        p = f*mul_mat2_f2(m2,p);
        m = mul_f_mat2(f,mul_mat2_mat2(m2t,m));
    }
  return d;
}


__DEVICE__ float3 calcNormal( in float3 pos, float t, __TEXTURE2D__ iChannel0 )
{
    float sca = 0.003f/SC;
    float amp = SC*120.0f;
#if USE_SMOOTH_NOISE==1
    amp *= 0.9f;
#endif

    float2 grad = amp*sca*terrainHd(swi2(pos,x,z)*sca,iChannel0);
    
    return normalize(to_float3(-grad.x, 1.f, -grad.y));
}

__DEVICE__ float fbm( float2 p, __TEXTURE2D__ iChannel0 )
{
    _m2;
    float f = 0.0f;
    f += 0.5000f*texture( iChannel0, p/256.0f ).x; p = mul_mat2_f2(m2,p*2.02f);
    f += 0.2500f*texture( iChannel0, p/256.0f ).x; p = mul_mat2_f2(m2,p*2.03f);
    f += 0.1250f*texture( iChannel0, p/256.0f ).x; p = mul_mat2_f2(m2,p*2.01f);
    f += 0.0625f*texture( iChannel0, p/256.0f ).x;
    return f/0.9375f;
}

#define kMaxT  5000.0f*SC

__DEVICE__ float4 render( in float3 ro, in float3 rd, __TEXTURE2D__ iChannel0 )
{
    float3 light1 = normalize( to_float3(-0.8f,0.4f,-0.3f) );
    // bounding plane
    float tmin = 1.0f;
    float tmax = kMaxT;
#if 1
    float maxh = 250.0f*SC;
    float tp = (maxh-ro.y)/rd.y;
    if( tp>0.0f )
    {
        if( ro.y>maxh ) tmin = _fmaxf( tmin, tp );
        else            tmax = _fminf( tmax, tp );
    }
#endif
  float sundot = clamp(dot(rd,light1),0.0f,1.0f);
  float3 col;
  
  float t = raycast( ro, rd, tmin, tmax, iChannel0 );
  if( t>tmax)
  {
    // sky    
    col = to_float3(0.3f,0.5f,0.85f) - rd.y*rd.y*0.5f;
    col = _mix( col, 0.85f*to_float3(0.7f,0.75f,0.85f), _powf( 1.0f-_fmaxf(rd.y,0.0f), 4.0f ) );
    // sun
    col += 0.25f*to_float3(1.0f,0.7f,0.4f)*_powf( sundot,5.0f );
    col += 0.25f*to_float3(1.0f,0.8f,0.6f)*_powf( sundot,64.0f );
    col += 0.2f*to_float3(1.0f,0.8f,0.6f)*_powf( sundot,512.0f );
    // clouds

    float2 sc = swi2(ro,x,z) + swi2(rd,x,z)*(SC*1000.0f-ro.y)/rd.y;
    col = _mix( col, to_float3(1.0f,0.95f,1.0f), 0.5f*smoothstep(0.5f,0.8f,fbm(0.0005f*sc/SC,iChannel0)) );
    // horizon
    col = _mix( col, 0.68f*to_float3(0.4f,0.65f,1.0f), _powf( 1.0f-_fmaxf(rd.y,0.0f), 16.0f ) );
    t = -1.0f;
  }
  else
  {
    // mountains    
    float3 pos = ro + t*rd;
    float3 nor = calcNormal( pos, t, iChannel0 );
    //nor = normalize( nor + 0.5f*( to_float3(-1.0f,0.0f,-1.0f) + to_float3(2.0f,1.0f,2.0f)*texture(iChannel1,0.01f*swi2(pos,x,z)).xyz) );
    float3 ref = reflect( rd, nor );
    float fre = clamp( 1.0f+dot(rd,nor), 0.0f, 1.0f );
    float3 hal = normalize(light1-rd);
    
    // rock
    float r = texture( iChannel0, (7.0f/SC)*swi2(pos,x,z)/256.0f ).x;
    col = (r*0.25f+0.75f)*0.9f*_mix( to_float3(0.08f,0.05f,0.03f), to_float3(0.10f,0.09f,0.08f), 
                                     texture(iChannel0,0.00007f*to_float2(pos.x,pos.y*48.0f)/SC).x );
    col = _mix( col, 0.20f*to_float3(0.45f,0.30f,0.15f)*(0.50f+0.50f*r),smoothstep(0.70f,0.9f,nor.y) );
        
        
    col = _mix( col, 0.15f*to_float3(0.30f,0.30f,0.10f)*(0.25f+0.75f*r),smoothstep(0.95f,1.0f,nor.y) );
    col *= 0.1f+1.8f*_sqrtf(fbm(swi2(pos,x,z)*0.04f,iChannel0)*fbm(swi2(pos,x,z)*0.005f,iChannel0));

    // snow
    float h = smoothstep(55.0f,80.0f,pos.y/SC + 25.0f*fbm(0.01f*swi2(pos,x,z)/SC,iChannel0) );
    float e = smoothstep(1.0f-0.5f*h,1.0f-0.1f*h,nor.y);
    float o = 0.3f + 0.7f*smoothstep(0.0f,0.1f,nor.x+h*h);
    float s = h*e*o;
    col = _mix( col, 0.29f*to_float3(0.62f,0.65f,0.7f), smoothstep( 0.1f, 0.9f, s ) );

    // lighting    
    float amb = clamp(0.5f+0.5f*nor.y,0.0f,1.0f);
    float dif = clamp( dot( light1, nor ), 0.0f, 1.0f );
    float bac = clamp( 0.2f + 0.8f*dot( normalize( to_float3(-light1.x, 0.0f, light1.z ) ), nor ), 0.0f, 1.0f );
    float sh = 1.0f; if( dif>=0.0001f ) sh = softShadow(pos+light1*SC*0.05f,light1,t,iChannel0);
    
    float3 lin  = to_float3_s(0.0f);
    lin += dif*to_float3(8.00f,5.00f,3.00f)*1.3f*to_float3( sh, sh*sh*0.5f+0.5f*sh, sh*sh*0.8f+0.2f*sh );
    lin += amb*to_float3(0.40f,0.60f,1.00f)*1.2f;
    lin += bac*to_float3(0.40f,0.50f,0.60f);
    col *= lin;
        
    col += (0.7f+0.3f*s)*(0.04f+0.96f*_powf(clamp(1.0f+dot(hal,rd),0.0f,1.0f),5.0f))*
           to_float3(7.0f,5.0f,3.0f)*dif*sh*
           _powf( clamp(dot(nor,hal), 0.0f, 1.0f),16.0f);
        
    col += s*0.65f*_powf(fre,4.0f)*to_float3(0.3f,0.5f,0.6f)*smoothstep(0.0f,0.6f,ref.y);

    //col = col*3.0f/(1.5f+col);
        
    // fog
    float fo = 1.0f-_expf(-_powf(0.001f*t/SC,1.5f) );
    float3 fco = 0.65f*to_float3(0.4f,0.65f,1.0f);// + 0.1f*to_float3(1.0f,0.8f,0.5f)*_powf( sundot, 4.0f );
    col = _mix( col, fco, fo );

  }
    // sun scatter
    col += 0.3f*to_float3(1.0f,0.7f,0.3f)*_powf( sundot, 8.0f );

    // gamma
  col = sqrt_f3(col);
    
  return to_float4_aw( col, t );
}

__DEVICE__ float3 camPath( float time )
{
  return SC*1100.0f*to_float3( _cosf(0.0f+0.23f*time), 0.0f, _cosf(1.5f+0.21f*time) );
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, in float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv = normalize( cross(cu,cw) );
  return to_mat3_f3( cu, cv, cw );
}

__DEVICE__ void moveCamera( float time, out float3 *oRo, out float3 *oTa, out float *oCr, out float *oFl, __TEXTURE2D__ iChannel0 )
{
  float3 ro = camPath( time );
  float3 ta = camPath( time + 3.0f );
  ro.y = terrainL( swi2(ro,x,z),iChannel0 ) + 22.0f*SC;
  ta.y = ro.y - 20.0f*SC;
  float cr = 0.2f*_cosf(0.1f*time);
    *oRo = ro;
    *oTa = ta;
    *oCr = cr;
    *oFl = 3.0f;
}

//Eigentlich BufferA zur MotionBlurerzeugung __
__KERNEL__ void ElevatedAFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    float time = iTime*0.1f - 0.1f + 0.3f + 4.0f*iMouse.x/iResolution.x;

    // camera position
    float3 ro, ta; float cr, fl;
    moveCamera( time, &ro, &ta, &cr, &fl, iChannel0 );

    // camera2world transform    
    mat3 cam = setCamera( ro, ta, cr );

    // pixel
    float2 p = (-iResolution + 2.0f*fragCoord)/iResolution.y;

    float t = kMaxT;
    float3 tot = to_float3_s(0.0f);
  #if AA>1
    for( int m=0; m<AA; m++ )
    for( int n=0; n<AA; n++ )
    {
        // pixel coordinates
        float2 o = to_float2(float(m),float(n)) / float(AA) - 0.5f;
        float2 s = (-iResolution + 2.0f*(fragCoord+o))/iResolution.y;
  #else    
        float2 s = p;
  #endif

        // camera ray    
        float3 rd = mul_mat3_f3(cam , normalize(to_float3_aw(s,fl)));

        float4 res = render( ro, rd, iChannel0 );
        t = _fminf( t, res.w );
 
        tot += swi3(res,x,y,z);
  #if AA>1
    }
    tot /= float(AA*AA);
  #endif


    //-------------------------------------
  // velocity vectors (through depth reprojection)
    //-------------------------------------
    float vel = 0.0f;
    if( t<0.0f )
    {
        vel = -1.0f;
    }
    else
    {

        // old camera position
        float oldTime = time - 0.1f * 1.0f/24.0f; // 1/24 of a second blur
        float3 oldRo, oldTa; float oldCr, oldFl;
        moveCamera( oldTime, &oldRo, &oldTa, &oldCr, &oldFl, iChannel0 );
        mat3 oldCam = setCamera( oldRo, oldTa, oldCr );

        // world space
        #if AA>1
        float3 rd = mul_mat3_f3(cam , normalize(to_float3_aw(p,fl)));
        #endif
        float3 wpos = ro + rd*t;
        // camera space
//        float3 cpos = to_float3( dot( wpos - oldRo, oldCam[0] ),
//                                 dot( wpos - oldRo, oldCam[1] ),
//                                 dot( wpos - oldRo, oldCam[2] ) );
        float3 cpos = to_float3( dot( wpos - oldRo, oldCam.r0 ),
                                 dot( wpos - oldRo, oldCam.r1 ),
                                 dot( wpos - oldRo, oldCam.r2 ) );

        // ndc space
        float2 npos = oldFl * swi2(cpos,x,y) / cpos.z;
        // screen space
        float2 spos = 0.5f + 0.5f*npos*to_float2(iResolution.y/iResolution.x,1.0f);


        // compress velocity vector in a single float
        float2 uv = fragCoord/iResolution;
        spos = clamp( 0.5f + 0.5f*(spos - uv)/0.25f, 0.0f, 1.0f );
        vel = _floor(spos.x*1023.0f) + _floor(spos.y*1023.0f)*1024.0f;
    }
    
    fragColor = to_float4_aw( tot, vel );


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Previsualization: Buffer A' to iChannel0


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


// on the derivatives based noise: http://iquilezles.org/www/articles/morenoise/morenoise.htm
// on the soft shadow technique: http://iquilezles.org/www/articles/rmshadows/rmshadows.htm
// on the fog calculations: http://iquilezles.org/www/articles/fog/fog.htm
// on the lighting: http://iquilezles.org/www/articles/outdoorslighting/outdoorslighting.htm
// on the raymarching: http://iquilezles.org/www/articles/terrainmarching/terrainmarching.htm


#ifdef MotionBlur
__KERNEL__ void XXX(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord/iResolution;
    float4 data = _tex2DVecN( iChannel0,uv.x,uv.y,15);

    float3 col = to_float3_s(0.0f);
    if( data.w < 0.0f )
    {
        col = swi3(data,x,y,z);
    }
    else
    {
        // decompress velocity vector
        float ss = mod_f(data.w,1024.0f)/1023.0f;
        float st = _floor(data.w/1024.0f)/1023.0f;

        // motion blur (linear blur across velocity vectors
        float2 dir = (-1.0f + 2.0f*to_float2( ss, st ))*0.25f;
        col = to_float3_s(0.0f);
        for( int i=0; i<32; i++ )
        {
            float h = (float)(i)/31.0f;
            float2 pos = uv + dir*h;
            col += _tex2DVecN( iChannel0,pos.x,pos.y,15).xyz;
        }
        col /= 32.0f;
    }
    
    // vignetting  
    col *= 0.5f + 0.5f*_powf( 16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y), 0.1f );

    col = clamp(col,0.0f,1.0f);
    col = col*0.6f + 0.4f*col*col*(3.0f-2.0f*col) + to_float3(0.0f,0.0f,0.04f);
    

col = swi3(_tex2DVecN( iChannel0,uv.x,uv.y,15),x,y,z);
    
    fragColor = to_float4_aw( col, 1.0f );
    
    
    
    


  SetFragmentShaderComputedColor(fragColor);
}

#endif
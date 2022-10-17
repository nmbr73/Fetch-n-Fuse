
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Stars' to iChannel0
// Connect Image 'Texture: Organic 3' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Copyright Inigo Quilez, 2013 - https://iquilezles.org/
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

// Some columns, derived from Slisesix
//
//   https://www.shadertoy.com/view/NtlSDs


#define AA 1


// https://iquilezles.org/articles/distfunctions
__DEVICE__ float udBox( in float3 p, in float3 abc )
{
  return length(_fmaxf(abs_f3(p)-abc, to_float3_s(0.0f)));
}
// https://iquilezles.org/articles/distfunctions
__DEVICE__ float sdBox( in float3 p, in float3 b ) 
{
    float3 q = abs_f3(p) - b;
    return _fminf(_fmaxf(q.x,_fmaxf(q.y,q.z)),0.0f) + length(_fmaxf(q,to_float3_s(0.0f)));
}
// https://iquilezles.org/articles/smin
__DEVICE__ float smin( float a, float b, float k )
{
    float h = _fmaxf(k-_fabs(a-b),0.0f);
    return _fminf(a, b) - h*h*0.25f/k;
}
// https://iquilezles.org/articles/smin
__DEVICE__ float smax( float a, float b, float k )
{
    float h = _fmaxf(k-_fabs(a-b),0.0f);
    return _fmaxf(a, b) + h*h*0.25f/k;
}

// https://iquilezles.org/articles/biplanar
__DEVICE__ float roundcube( float3 p, float3 n, __TEXTURE2D__ iChannel0 )
{
  float uuuuuuuuuuuuuuuuuuuu;
  n = abs_f3(n);
  float _x = texture( iChannel0, swi2(p,y,z) ).x;
  float _y = texture( iChannel0, swi2(p,z,x) ).x;
  float _z = texture( iChannel0, swi2(p,x,y) ).x;
  return (_x*n.x + _y*n.y + _z*n.z)/(n.x+n.y+n.z);
}

#define ZERO 0 //(_fminf(iFrame,0))

//------------------------------------------

__DEVICE__ float3 column( in float _x, in float _y, in float _z )
{
  float y2=_y-0.25f;
  float y3=_y-0.25f;
  float y4=_y-1.0f;

  float dsp = _fabs( _fminf(_cosf(1.5f*0.75f*6.283185f*_x/0.085f), _cosf(1.5f*0.75f*6.283185f*_z/0.085f)));
  dsp *= 1.0f-smoothstep(0.8f,0.9f,_fabs(_x/0.085f)*_fabs(_z/0.085f));
  float di1=sdBox( to_float3(_x,mod_f(_y+0.08f,0.16f)-0.08f,_z), to_float3(0.10f*0.85f+dsp*0.03f*0.25f,0.079f,0.10f*0.85f+dsp*0.03f*0.25f)-0.008f )-0.008f;
  float di2=sdBox( to_float3(_x,_y,_z), to_float3(0.12f,0.29f,0.12f)-0.007f )-0.007f;
  float di3=sdBox( to_float3(_x,y4,_z), to_float3(0.14f,0.02f,0.14f)-0.006f )-0.006f;
  float nx = _fmaxf( _fabs(_x), _fabs(_z) );
  float nz = _fminf( _fabs(_x), _fabs(_z) );  
  float di4=sdBox( to_float3(nx, _y, nz), to_float3(0.14f,0.3f,0.05f)-0.004f )-0.004f;
  float di5=smax(-(_y-0.291f),sdBox( to_float3(nx, (y2+nz)*0.7071f, (nz-y2)*0.7071f), to_float3(0.12f, 0.16f*0.7071f, 0.16f*0.7071f)-0.004f)-0.004f,0.007f + 0.0001f);
  float di6=sdBox( to_float3(nx, (y3+nz)*0.7071f, (nz-y3)*0.7071f), to_float3(0.14f, 0.10f*0.7071f, 0.10f*0.7071f)-0.004f)-0.004f;

  float dm1 = _fminf(_fminf(di5,di3),di2);
  float dm2 = _fminf(di6,di4);
  float3 res = to_float3( dm1, 3.0f, 1.0f );
  if( di1<res.x ) res = to_float3( di1, 2.0f, dsp );
  if( dm2<res.x ) res = to_float3( dm2, 5.0f, 1.0f );
    
  return res;
}

__DEVICE__ float wave( in float _x, in float _y )
{
    return _sinf(_x)*_sinf(_y);
}

#define SC 15.0f
__DEVICE__ float3 map( float3 pos, __TEXTURE2D__ iChannel0)
{
  pos /= SC;

  // floor
  float2 id = _floor((swi2(pos,x,z)+0.1f)/0.2f );
  float h = 0.012f + 0.008f*_sinf(id.x*2313.12f+id.y*3231.219f);
  float3 ros = to_float3( mod_f(pos.x+0.1f,0.2f)-0.1f, pos.y, mod_f(pos.z+0.1f,0.2f)-0.1f );
  float3 res = to_float3( udBox( ros, to_float3(0.096f,h,0.096f)-0.005f )-0.005f, 0.0f, 0.0f );

  // ceilin
  float x = fract( pos.x+128.0f ) - 0.5f;
  float z = fract( pos.z+128.0f ) - 0.5f;
  float y = (1.0f - pos.y)*0.6f;// + 0.1f;
  float dis = 0.4f - smin(_sqrtf(y*y+x*x),_sqrtf(y*y+z*z),0.01f);
  float dsp = _fabs(_sinf(31.416f*pos.y)*_sinf(31.416f*pos.x)*_sinf(31.416f*pos.z));
  dis -= 0.02f*dsp;

  dis = _fmaxf( dis, y );
  if( dis<res.x )
  {
      res = to_float3(dis,1.0f,dsp);
  }

  // columns
  float2 fc = fract_f2( swi2(pos,x,z)+128.5f ) - 0.5f;
  float3 dis2 = column( fc.x, pos.y, fc.y );
  if( dis2.x<res.x )
    {
        res = dis2;
    }
    
  fc = fract_f2( swi2(pos,x,z)+128.5f )-0.5f;
  dis = length(to_float3(fc.x,pos.y,fc.y)-to_float3(0.0f,-0.565f,0.0f))-0.6f;
  dis -= texture(iChannel0,1.5f*swi2(pos,x,z)).x*0.02f;
  if( dis<res.x ) res=to_float3(dis,4.0f,1.0f);
  
  res.x*=SC;
  return res;
}

__DEVICE__ float4 calcColor( in float3 pos, in float3 nor, in float sid, out float ke, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 )
{
  float3 col = to_float3_s( 1.0f );
  float ks = 1.0f;
  ke = 0.0f;

  float kk = 0.2f+0.8f*roundcube( 1.0f*pos, nor, iChannel0 );

  // floor
  if( sid<0.5f )
  {
    col = swi3(texture( iChannel1, 6.0f*swi2(pos,x,z) ),x,y,z);
    float2 id = _floor((swi2(pos,x,z)+0.1f)/0.2f );
    col *= 1.0f + 0.5f*_sinf(id.y*2313.12f+id.x*3231.219f);
  }
  // ceilin
  else if( sid<1.5f )
  {
    float fx = fract( pos.x+128.0f ); 
    float fz = fract( pos.z+128.0f ); 
    col = to_float3(0.7f,0.6f,0.5f)*1.3f;
    float p = 1.0f;
    p *= smoothstep( 0.02f, 0.03f, _fabs(fx-0.1f) );
    p *= smoothstep( 0.02f, 0.03f, _fabs(fx-0.9f) );
    p *= smoothstep( 0.02f, 0.03f, _fabs(fz-0.1f) );
    p *= smoothstep( 0.02f, 0.03f, _fabs(fz-0.9f) );
    //col = _mix( to_float3(0.6f,0.2f,0.1f), col, p );
    col = _mix( to_float3_s(2.0f), col, p );
  }
  // columns
  else if( sid<2.5f )
  {
    float id = _floor((pos.y+0.08f)/0.16f);
    col = to_float3(0.7f,0.6f,0.5f);
    col *= 1.0f + 0.4f*_cosf(id*312.0f + _floor(pos.x+0.5f)*33.1f + _floor(pos.z+0.5f)*13.7f);
  }
  // columns bottom
  else if( sid<3.5f )
  {
    col = to_float3(0.7f,0.6f,0.5f);
    col *= 0.25f + 0.75f*smoothstep(0.0f,0.1f,pos.y);
  }
  // dirt
  else if( sid<4.5f )
  {
    col = to_float3(0.2f,0.15f,0.1f)*0.5f;
    ks = 0.05f;
  }
  // colums stone
  else // if( sid<5.5f )
  {
    col = to_float3_s(1.0f);
    
    col *= 0.25f + 0.75f*smoothstep(0.0f,0.1f,pos.y);
    ks = 1.0f;
    kk = kk*0.5f+0.5f;
  }

  return to_float4_aw(col * 1.2f * kk,ks);
}

__DEVICE__ float3 raycast( in float3 ro, in float3 rd, in float precis, in float maxd, __TEXTURE2D__ iChannel0 )
{
    float t = 0.001f;
    float dsp = 0.0f;
    float sid = -1.0f;
    for( int i=0; i<128; i++ )
    {
      float3 res = map( ro+rd*t, iChannel0 );
      if( _fabs(res.x)<(precis*t)||t>maxd ) break;
      sid = res.y;
      dsp = res.z;
      t += res.x;
    }

    if( t>maxd ) sid=-1.0f;
    return to_float3( t, sid, dsp );
}

// https://iquilezles.org/articles/rmshadows
__DEVICE__ float softshadow( in float3 ro, in float3 rd, in float mint, in float maxt, in float k, __TEXTURE2D__ iChannel0 )
{
    float res = 1.0f;
    float t = mint;
    for( int i=0; i<32; i++ )
    {
        float h = map( ro + rd*t, iChannel0 ).x;
        res = _fminf( res, k*h/t );
        t += clamp(h,0.1f,1.0f);
    if( res<0.001f || t>maxt ) break;
    }
    return clamp( res, 0.0f, 1.0f );
}

// https://iquilezles.org/articles/normalsSDF
__DEVICE__ float3 calcNormal( in float3 pos, __TEXTURE2D__ iChannel0 )
{
#if 0
  float3 eps = to_float3( 0.001f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(pos+swi3(eps,x,y,y),iChannel0).x - map(pos-swi3(eps,x,y,y),iChannel0).x,
      map(pos+swi3(eps,y,x,y),iChannel0).x - map(pos-swi3(eps,y,x,y),iChannel0).x,
      map(pos+swi3(eps,y,y,x),iChannel0).x - map(pos-swi3(eps,y,y,x),iChannel0).x );
  return normalize(nor);
#else
    // inspired by tdhooper and klems - a way to prevent the compiler from inlining map() 4 times
    float3 n = to_float3_s(0.0f);
    for( int i=ZERO; i<4; i++ )
    {
      float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
      n += e*map(pos+e*0.001f,iChannel0).x;
    }
    return normalize(n);
#endif    
}

__DEVICE__ float3 doBumpMap( in float3 pos, in float3 nor, __TEXTURE2D__ iChannel0 )
{
  const float e = 0.001f;
  const float b = 0.005f;
  
  float ref = roundcube( 7.0f*pos, nor, iChannel0 );
  float3 gra = -b*to_float3( roundcube(7.0f*to_float3(pos.x+e, pos.y, pos.z),nor, iChannel0)-ref,
                             roundcube(7.0f*to_float3(pos.x, pos.y+e, pos.z),nor, iChannel0)-ref,
                             roundcube(7.0f*to_float3(pos.x, pos.y, pos.z+e),nor, iChannel0)-ref )/e;
  
  float3 tgrad = gra - nor*dot(nor,gra);
    
  return normalize( nor-tgrad );
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor, __TEXTURE2D__ iChannel0 )
{
    float ao = 0.0f;
    float sca = 15.0f;
    for( int i=ZERO; i<5; i++ )
    {
        float hr = SC*(0.01f + 0.01f*(float)(i*i));
        float dd = map( pos + hr*nor, iChannel0 ).x;
        ao += (hr-dd)*sca/SC;
        sca *= 0.85f;
    }
    return 1.0f - clamp( ao*0.3f, 0.0f, 1.0f );
}

__DEVICE__ float3 getLightPos( in int i, float iTime )
{
    float3 lpos;
    
    lpos.x = 0.5f + 2.2f*_cosf(0.22f+0.1f*iTime + 17.0f*float(i) );
    lpos.y = 0.25f;
    lpos.z = 1.5f + 2.2f*_cosf(2.24f+0.1f*iTime + 13.0f*float(i) );

    // make the lights avoid the columns
    float2 ilpos = _floor( swi2(lpos,x,z) );
    float2 flpos = swi2(lpos,x,z) - ilpos;
    flpos = flpos - 0.5f;
    if( length(flpos)<0.2f ) flpos = 0.2f*normalize(flpos);
    swi2S(lpos,x,z, ilpos + flpos);

    return lpos*SC;
}

__DEVICE__ float4 getLightCol( in int i, float iTime )
{
    float li = _sqrtf(0.5f + 0.5f*_sinf(2.0f*iTime+ 23.1f*(float)(i)));
    float h = (float)(i)/8.0f;
    float3 c = _mix( to_float3(1.0f,0.8f,0.6f), to_float3(1.0f,0.3f,0.05f), 0.5f+0.5f*_sinf(40.0f*h) );
    return to_float4_aw( c, li );
}

const int kNumLights = 9;

__DEVICE__ float3 render( in float3 ro, in float3 rd, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 )
{ 
    float3 col = to_float3_s(0.0f);
    float3 res = raycast(ro,rd,0.00001f*SC,10.0f*SC, iChannel0);
    float t = res.x;
    if( res.y>-0.5f )
    {
      float3 pos = ro + t*rd;
      float3 nor = calcNormal( pos, iChannel0 );

      float ao = calcAO( pos, nor, iChannel0 );
      ao *= 0.7f + 0.6f*res.z;
      
      pos /= SC;
      t /= SC;
        
      float ke = 0.0f;
      float4 mate = calcColor( pos, nor, res.y, ke, iChannel0, iChannel1 );
      col = swi3(mate,x,y,z);
      float ks = mate.w;

      nor = doBumpMap( pos, nor, iChannel0 );
      
      // lighting
      float fre = clamp(1.0f+dot(nor,rd),0.0f,1.0f);
      float3 lin = 0.03f*ao*to_float3(0.25f,0.20f,0.20f)*(0.5f+0.5f*nor.y);
      float3 spe = to_float3_s(0.0f);
      for( int i=0; i<kNumLights; i++ )
      {
          float3 lpos = getLightPos(i, iTime);
          float4 lcol = getLightCol(i, iTime);
          
          float3 lig = lpos/SC - pos;
          float llig = dot( lig, lig);
          float im = 1.0f/_sqrtf(llig);//inversesqrt( llig );
          lig = lig * im;
          float dif = clamp( dot( nor, lig ), 0.0f, 1.0f );
          float at = 2.0f*_exp2f( -2.3f*llig )*lcol.w;
          dif *= at;
          float at2 = _exp2f( -0.35f*llig );

          float sh = 0.0f;
          if( dif>0.001f ) { sh = softshadow( pos*SC, lig, 0.02f*SC, _sqrtf(llig)*SC, 32.0f, iChannel0 ); dif *= sh; }
            float dif2 = clamp( dot(nor,normalize(to_float3(-lig.x,0.0f,-lig.z))), 0.0f, 1.0f );
            
            lin += 2.50f*dif*swi3(lcol,x,y,z);
            lin += 0.10f*dif2*to_float3(0.35f,0.20f,0.10f)*at2*ao*to_float3(1.5f,1.0f,0.5f);
            lin += fre*fre*col*ao*ke*10.0f*clamp(0.5f+0.5f*dot(nor,lig),0.0f,1.0f)*sh;
                        
            float3 hal = normalize(lig-rd);
            float pp = clamp( dot(nor,hal), 0.0f, 1.0f );
            pp = _powf(pp,1.0f+ke*3.0f);
            spe += ks*(5.0f)*swi3(lcol,x,y,z)*at*dif*(0.04f+0.96f*_powf(1.0f-clamp(dot(hal,-rd),0.0f,1.0f),5.0f))*(_powf(pp,16.0f) + 0.5f*_powf(pp,4.0f));
            
        }
    
        col = col*lin + 2.0f*spe + 4.0f*ke*fre*col*col*ao;
    }
  else
    {
      t /= SC;
    }
    
  col *= _expf( -0.055f*t*t );

    
    // lights
  for( int i=0; i<kNumLights; i++ )
  {
        float3 lpos = getLightPos(i, iTime);
        float4 lcol = getLightCol(i, iTime);
        
        float3 lv = (lpos - ro)/SC;
        float ll = length( lv );
        if( ll<t )
        {
            float dle = clamp( dot( rd, lv/ll ), 0.0f, 1.0f );
            dle = 1.0f-smoothstep( 0.0f, 0.2f*(0.7f+0.3f*lcol.w), _acosf(dle)*ll );
            col += dle*dle*6.0f*lcol.w*swi3(lcol,x,y,z)*_expf( -0.07f*ll*ll );
        }
    }
    
  return col;
}

__KERNEL__ void ColumnsAndLightsFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 mo = swi2(iMouse,x,y)/iResolution;

    float3 tot = to_float3_s(0.0f);

    #if AA>1
    for( int m=ZERO; m<AA; m++ )
    for( int n=ZERO; n<AA; n++ )
    {
        // pixel coordinates
        float2 o = to_float2((float)(m),(float)(n)) / (float)(AA) - 0.5f;

        float2 p = (2.0f*(fragCoord+o)-iResolution)/iResolution.y;
#else
        float2 p = (2.0f*fragCoord-iResolution)/iResolution.y;
#endif        
        float time = iTime;

        // camera  
        float3 ce = to_float3( 0.5f, 0.25f, 1.5f );
        float3 ro = ce + to_float3( 1.3f*_cosf(0.11f*time + 6.0f*mo.x), 0.65f*(1.0f-mo.y)- 0.4f, 1.3f*_sinf(0.11f*time + 6.0f*mo.x) );
        float3 ta = ce + to_float3( 0.95f*_cosf(1.2f+0.08f*time), 0.4f*0.25f+0.75f*ro.y- 0.2f, 0.95f*_sinf(2.0f+0.07f*time) );
        ro *= SC;
        ta *= SC;
        float roll = -0.15f*_sinf(0.1f*time);

        // distort
        float r2 = p.x*p.x*0.3164f + p.y*p.y;
        p *= (7.15f-_sqrtf(38.0f-12.0f*r2))/(r2+1.0f);

        // camera tx
        float3 cw = normalize( ta-ro );
        float3 cp = to_float3( _sinf(roll), _cosf(roll),0.0f );
        float3 cu = normalize( cross(cw,cp) );
        float3 cv = normalize( cross(cu,cw) );
        float3 rd = normalize( p.x*cu + p.y*cv + 1.5f*cw );

        float3 col = render( ro, rd, iTime, iChannel0, iChannel1 );

        col = col*2.0f/(1.0f+col);
        col = pow_f3( col, to_float3_s(0.4545f) );
        col *= to_float3(1.0f,1.05f,1.0f);
        col += to_float3(0.0f,0.03f,0.0f);
        tot += col;

#if AA>1
    }
    tot /= (float)(AA*AA);
#endif

    // vigneting
    {
    float2 q = fragCoord/iResolution;
    tot *= 0.25f+0.75f*_powf( 16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.15f );
    }
    
    fragColor = to_float4_aw( tot, 1.0f );
    
    SetFragmentShaderComputedColor(fragColor);
}

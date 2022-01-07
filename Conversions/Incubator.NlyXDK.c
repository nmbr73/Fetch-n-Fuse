
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Texture: London' to iChannel0


// This is a test shader based on the demo shader by inigo quilez. Mod by Zina.

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// Demo created by inigo quilez - iq/2013 (see original here):
// More info here: http://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm

__DEVICE__ float sdPlane( float3 p )
{
  return +p.y;
}

__DEVICE__ float sdSphere( float3 p, float s )
{
    return length(p)-s;
}

__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 d = abs_f3(p) - b;
  return _fminf(_fmaxf(d.x,_fmaxf(d.y,d.z)),0.0f) + length(_fmaxf(d,to_float3_s(0.0f)));
}

__DEVICE__ float sdEllipsoid( in float3 p, in float3 r )
{
    return (length( p/r ) - 1.0f) * _fminf(_fminf(r.x,r.y),r.z);
}

__DEVICE__ float udRoundBox( float3 p, float3 b, float r )
{
  return length(_fmaxf(abs_f3(p)-b,to_float3_s(0.0f)))-r;
}

__DEVICE__ float sdTorus( float3 p, float2 t )
{
  return length( to_float2(length(swi2(p,x,z))-t.x,p.y) )-t.y;
}

__DEVICE__ float sdHexPrism( float3 p, float2 h )
{
    float3 q = abs_f3(p);
#if 0
    return _fmaxf(q.z-h.y,_fmaxf((q.x*0.866025f+q.y*0.5f),q.y)-h.x);
#else
    float d1 = q.z-h.y;
    float d2 = _fmaxf((q.x*0.866025f+q.y*0.5f),q.y)-h.x;
    return length(_fmaxf(to_float2(d1,d2),to_float2_s(0.0f))) + _fminf(_fmaxf(d1,d2), 0.0f);
#endif
}

__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r )
{
  float3 pa = p-a, ba = b-a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
  return length( pa - ba*h ) - r;
}

__DEVICE__ float sdTriPrism( float3 p, float2 h )
{
    float3 q = abs_f3(p);
#if 0
    return _fmaxf(q.z-h.y,_fmaxf(q.x*0.866025f+p.y*0.5f,-p.y)-h.x*0.5f);
#else
    float d1 = q.z-h.y;
    float d2 = _fmaxf(q.x*0.866025f+p.y*0.5f,-p.y)-h.x*0.5f;
    return length(_fmaxf(to_float2(d1,d2),to_float2_s(0.0f))) + _fminf(_fmaxf(d1,d2), 0.0f);
#endif
}

__DEVICE__ float sdCylinder( float3 p, float2 h )
{
  float2 d = abs_f2(to_float2(length(swi2(p,x,z)),p.y)) - h;
  return _fminf(_fmaxf(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}

__DEVICE__ float sdCone( in float3 p, in float3 c )
{
    float2 q = to_float2( length(swi2(p,x,z)), p.y );
    float d1 = -q.y-c.z;
    float d2 = _fmaxf( dot(q,swi2(c,x,y)), q.y);
    return length(_fmaxf(to_float2(d1,d2),to_float2_s(0.0f))) + _fminf(_fmaxf(d1,d2), 0.0f);
}

__DEVICE__ float sdConeSection( in float3 p, in float h, in float r1, in float r2 )
{
    float d1 = -p.y - h;
    float q = p.y - h;
    float si = 0.5f*(r1-r2)/h;
    float d2 = _fmaxf( _sqrtf( dot(swi2(p,x,z),swi2(p,x,z))*(1.0f-si*si)) + q*si - r2, q );
    return length(_fmaxf(to_float2(d1,d2),to_float2_s(0.0f))) + _fminf(_fmaxf(d1,d2), 0.0f);
}


__DEVICE__ float length2( float2 p )
{
  return _sqrtf( p.x*p.x + p.y*p.y );
}

__DEVICE__ float length6( float2 p )
{
  p = p*p*p; p = p*p;
  return _powf( p.x + p.y, 1.0f/6.0f );
}

__DEVICE__ float length8( float2 p )
{
  p = p*p; p = p*p; p = p*p;
  return _powf( p.x + p.y, 1.0f/8.0f );
}

__DEVICE__ float sdTorus82( float3 p, float2 t )
{
  float2 q = to_float2(length2(swi2(p,x,z))-t.x,p.y);
  return length8(q)-t.y;
}

__DEVICE__ float sdTorus88( float3 p, float2 t )
{
  float2 q = to_float2(length8(swi2(p,x,z))-t.x,p.y);
  return length8(q)-t.y;
}

__DEVICE__ float sdCylinder6( float3 p, float2 h )
{
  return _fmaxf( length6(swi2(p,x,z))-h.x, _fabs(p.y)-h.y );
}

//----------------------------------------------------------------------

__DEVICE__ float opS( float d1, float d2 )
{
    return _fmaxf(-d2,d1);
}

__DEVICE__ float2 opU( float2 d1, float2 d2 )
{
  return (d1.x<d2.x) ? d1 : d2;
}

__DEVICE__ float3 opRep( float3 p, float3 c )
{
    return mod_f(p,c)-0.5f*c;
}

__DEVICE__ float3 opTwist( float3 p )
{
    float  c = _cosf(10.0f*p.y+10.0f);
    float  s = _sinf(10.0f*p.y+10.0f);
    mat2   m = to_mat2(c,-s,s,c);
    return to_float3_aw(mul_mat2_f2(m,swi2(p,x,z)),p.y);
}

//----------------------------------------------------------------------

__DEVICE__ float2 map( in float3 pos, float iTime ) {
   
    pos = opRep(pos, to_float3(1.0f + 0.1f * _sinf(iTime), 2.0f, 2.5f + 0.1f * _sinf(iTime)));
    //float displace = opDisplace(pos);
    //float cone = sdCone(pos, to_float3(2.0f, 1.0f, 1.0f));
    float ellipsoid = sdEllipsoid(pos, to_float3(0.2f, 0.2f + 0.1f * _sinf(iTime * 4.0f), 0.2f));
    
    float torus = sdTorus(pos, to_float2(2.0f, 1.0f));
    float plane = pos.y;
    //float d = _sinf(pos.x);
    //float d = 1.0f - pos.x * pos.y;
    float sphere = length(pos) - 1.0f;
    float d = _fmaxf(plane, torus);
    d = _fminf(d, ellipsoid);
    return(to_float2(d, 1.0f));
    
}

__DEVICE__ float2 castRay( in float3 ro, in float3 rd, float iTime )
{
    float tmin = 1.0f;
    float tmax = 20.0f;
    
#if 0
    float tp1 = (0.0f-ro.y)/rd.y; if( tp1>0.0f ) tmax = _fminf( tmax, tp1 );
    float tp2 = (1.6f-ro.y)/rd.y; if( tp2>0.0f ) { if( ro.y>1.6f ) tmin = _fmaxf( tmin, tp2 );
                                                   else            tmax = _fminf( tmax, tp2 ); }
#endif

    float precis = 0.002f;
    float t = tmin;
    float m = -1.0f;
    for( int i=0; i<50; i++ )
    {
      float2 res = map( ro+rd*t, iTime );
        if( res.x<precis || t>tmax ) break;
        t += res.x;
        m = res.y;
    }

    if( t>tmax ) m=-1.0f;
    return to_float2( t, m );
}


__DEVICE__ float softshadow( in float3 ro, in float3 rd, in float mint, in float tmax, float iTime )
{
  float res = 1.0f;
    float t = mint;
    for( int i=0; i<16; i++ )
    {
    float h = map( ro + rd*t, iTime ).x;
        res = _fminf( res, 8.0f*h/t );
        t += clamp( h, 0.02f, 0.10f );
        if( h<0.001f || t>tmax ) break;
    }
    return clamp( res, 0.0f, 1.0f );

}

__DEVICE__ float3 calcNormal( in float3 pos, float iTime )
{
  float3 eps = to_float3( 0.001f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(pos+swi3(eps,x,y,y),iTime).x - map(pos-swi3(eps,x,y,y),iTime).x,
      map(pos+swi3(eps,y,x,y),iTime).x - map(pos-swi3(eps,y,x,y),iTime).x,
      map(pos+swi3(eps,y,y,x),iTime).x - map(pos-swi3(eps,y,y,x),iTime).x );
  return normalize(nor);
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor, float iTime )
{
    float occ = 0.0f;
    float sca = 1.0f;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01f + 0.12f*(float)(i)/4.0f;
        float3 aopos =  nor * hr + pos;
        float dd = map( aopos,iTime ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95f;
    }
    return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f );    
}




__DEVICE__ float3 render( in float3 ro, in float3 rd, float iTime, float ratio, __CONSTANTREF__ ShaderParameters*  _shaderParameters, __TEXTURE2D__ iChannel0 )
{ 

    //CONNECT_CHECKBOX0(Textur,0);    
    #define _sP _shaderParameters
    
    
    float3 col = to_float3(0.7f, 0.9f, 1.0f) +rd.y*0.8f;
    float2 res = castRay(ro,rd,iTime);
    float t = res.x;
    float m = res.y;
    if( m>-0.5f )
    {
        float3 pos = ro + t*rd;
        float3 nor = calcNormal( pos, iTime );
        float3 ref = reflect( rd, nor );

        // material        
        col = 0.45f + 0.3f*sin_f3( to_float3(0.05f,0.08f,0.10f)*(m-1.0f) );
    
        if( m<1.5f )
        {
            float f = mod_f( _floor(5.0f*pos.z) + _floor(5.0f*pos.x), 2.0f);
            col = 0.4f + 0.1f*f*to_float3_s(1.0f);

            if (_sP->ctrlCheckbox[0])
            {              
              float2 tuv = _sP->ctrlSlider[0]*to_float2(pos.x/ratio,pos.z); 
              col = swi3(_tex2DVecN(iChannel0, tuv.x,tuv.y,15),x,y,z);
            }
        }

        // lighitng        
        float occ = calcAO( pos, nor,iTime );
        float3  lig = normalize( to_float3(-0.6f, 0.7f, -0.5f) );
        float amb = clamp( 0.5f+0.5f*nor.y, 0.0f, 1.0f );
        float dif = clamp( dot( nor, lig ), 0.0f, 1.0f );
        float bac = clamp( dot( nor, normalize(to_float3(-lig.x,0.0f,-lig.z))), 0.0f, 1.0f )*clamp( 1.0f-pos.y,0.0f,1.0f);
        float dom = smoothstep( -0.1f, 0.1f, ref.y );
        float fre = _powf( clamp(1.0f+dot(nor,rd),0.0f,1.0f), 2.0f );
        float spe = _powf(clamp( dot( ref, lig ), 0.0f, 1.0f ),16.0f);
        
        dif *= softshadow( pos, lig, 0.02f, 2.5f,iTime );
        dom *= softshadow( pos, ref, 0.02f, 2.5f,iTime );

        float3 lin = to_float3_s(0.0f);
        lin += 1.20f*dif*to_float3(1.00f,0.85f,0.55f);
        lin += 1.20f*spe*to_float3(1.00f,0.85f,0.55f)*dif;
        lin += 0.20f*amb*to_float3(0.50f,0.70f,1.00f)*occ;
        lin += 0.30f*dom*to_float3(0.50f,0.70f,1.00f)*occ;
        lin += 0.30f*bac*to_float3(0.25f,0.25f,0.25f)*occ;
        lin += 0.40f*fre*to_float3(1.00f,1.00f,1.00f)*occ;
        col = col*lin;

        col = _mix( col, to_float3(0.8f,0.9f,1.0f), 1.0f-_expf( -0.002f*t*t ) );

    }

  return ( clamp(col,0.0f,1.0f) );
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv = normalize( cross(cu,cw) );
  return to_mat3_f3( cu, cv, cw );
}

__KERNEL__ void IncubatorFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  CONNECT_SLIDER0(JiPiSlider, 0.0f, 3.0f, 1.0f);
  CONNECT_CHECKBOX0(Textur,0);

  float ratio = iResolution.x/iResolution.y;
  
  float2 q = fragCoord/iResolution;
  float2 p = -1.0f+2.0f*q;
  p.x *= iResolution.x/iResolution.y;
  float2 mo = swi2(iMouse,x,y)/iResolution;
     
  float time = 15.0f + iTime;

  // camera  
  float3 ro = to_float3( -0.5f+3.5f*_cosf(0.1f*time + 6.0f*mo.x), 1.0f + 2.0f*mo.y, 0.5f + 3.5f*_sinf(0.1f*time + 6.0f*mo.x) );
  float3 ta = to_float3( -0.5f, -0.4f, 0.5f );
  
  // camera-to-world transformation
  mat3 ca = setCamera( ro, ta, 0.0f );
    
  // ray direction
  float3 rd = mul_mat3_f3(ca , normalize( to_float3_aw(swi2(p,x,y),2.0f) ));

  // render  
  float3 col = render( ro, rd, iTime, ratio, _shaderParameters, iChannel0 );

  col = pow_f3( col, to_float3_s(0.4545f) );

  fragColor=to_float4_aw( col, 1.0f );


  SetFragmentShaderComputedColor(fragColor);
}
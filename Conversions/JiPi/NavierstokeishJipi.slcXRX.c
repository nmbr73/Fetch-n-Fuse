
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


// Undefine the next line for camera input.
//#define WATERPAINTING
// Heavily bitting P_Malin's https://www.shadertoy.com/view/XdlGz8 for the fake 3D effect.
#define FAKETHREED

#ifdef xxx
const float forceVector = 10.0f;
const float forceColour = 10.0f;
const float dx = 0.5f;
const float dt = dx * dx * 0.5f;
const float siz = 0.05f;
const float di = 1.25f;
const float alp = ( dx * dx ) / dt;
const float rbe = 1.0f / ( 4.0f + alp );
const float vo = 10.0f;
const float vf = 0.0025f;
const float mul = 10.0f;
const float e = 0.05f;
#endif

__DEVICE__ float dis( float2 uv, float2 mou )
{
    return length( uv - mou );
}

__DEVICE__ float cir( float2 uv, float2 mou, float r )
{
    float o = smoothstep( r, r - 0.05f, dis( uv, mou ) );
    return o;
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


//#define keyTex iChannel3
//#define KEY_I texture(keyTex,to_float2((105.5f-32.0f)/256.0f,(0.5f+0.0f)/3.0f)).x

const float arrow_density = 0.2f;
const float arrow_length = 0.95f;

const float3 luma = {0.2126f, 0.7152f, 0.0722f};

__DEVICE__ float segm(in float2 p, in float2 a, in float2 b) //from iq
{
  float2 pa = p - a;
  float2 ba = b - a;
  float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f);
  return length(pa - ba*h)*20.0f*arrow_density;
}

__DEVICE__ float cur( float2 uv, float2 iResolution, __TEXTURE2D__ iChannel0 )
{
    
    float xpi = 1.0f / iResolution.x;
    float ypi = 1.0f / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;
    
    float top = texture( iChannel0, to_float2( x, y + ypi ) ).x;
    float lef = texture( iChannel0, to_float2( x - xpi, y ) ).x;
    float rig = texture( iChannel0, to_float2( x + xpi, y ) ).x;
    float dow = texture( iChannel0, to_float2( x, y - ypi ) ).x;
    
    float dY = ( top - dow ) * 0.5f;
    float dX = ( rig - lef ) * 0.5f;
    
    return dX * dY;
}

__DEVICE__ float2 vor( float2 uv, float2 iResolution, __TEXTURE2D__ iChannel0, float dt, float vo )
{
float aaaaaaaaaaaaaaaaaaaa;    
    float2 pre = uv;
    
    float xpi = 1.0f / iResolution.x;
    float ypi = 1.0f / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;

    float2 dir = to_float2_s(0);
    dir.y = ( cur( to_float2( x, y + ypi ) , iResolution, iChannel0) ) - ( cur( to_float2( x, y - ypi ) , iResolution, iChannel0) );
    dir.x = ( cur( to_float2( x + xpi, y ) , iResolution, iChannel0) ) - ( cur( to_float2( x - xpi, y ) , iResolution, iChannel0) );
    
    dir = normalize( dir );
    
    if( length( dir ) > 0.0f )
    
    uv -= dt * vo * cur( uv , iResolution, iChannel0) * dir;
    
    return uv;
}

__KERNEL__ void NavierstokeishJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(KEY_I, 1);   
    
    CONNECT_SLIDER6(forceVector, -10.0f, 30.0f, 10.0f);
    CONNECT_SLIDER7(forceColour, -10.0f, 30.0f, 10.0f);
    CONNECT_SLIDER8(dx, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER9(DT, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER10(siz, -1.0f, 1.0f, 0.05f);
    CONNECT_SLIDER11(di, -1.0f, 3.0f, 1.25f);
    CONNECT_SLIDER12(RBE, -1.0f, 10.0f, 4.0f);
    CONNECT_SLIDER13(vo, -10.0f, 30.0f, 10.0f);
    CONNECT_SLIDER14(vf, -1.0f, 1.0f, 0.0025f);
    CONNECT_SLIDER15(mul, -10.0f, 30.0f, 10.0f);
      
    //const float forceVector = 10.0;  
    //const float dx = 0.5;
    const float dt = dx * dx * DT;//0.5;
    //const float siz = 0.05;
    //const float vo = 10.0;
    //const float vf = 0.0025;
    //const float mul = 10.0;
    const float e = 0.05;
      
    fragCoord+=0.5f;

    float2 p = fragCoord / iResolution.y;
    float2 uv = fragCoord / iResolution;
    float2 mou = swi2(iMouse,x,y) / iResolution.y;
    p *= mul;
    mou *= mul;
float AAAAAAAAAAAAAAAAAA;    
    float fO = 0.0f;
    fO +=  texture( iChannel1, vor( uv,iResolution, iChannel0,dt,vo ) ).x 
         + texture( iChannel1, vor( uv,iResolution, iChannel0,dt,vo ) ).y 
         + texture( iChannel1, vor( uv,iResolution, iChannel0,dt,vo ) ).z;
    fO *= 0.333f;
    
    float2 ep = to_float2( e, 0 );
    float2 rz= to_float2_s( 0 );
    float2 fra = fract_f2( uv );

    float t0 = 0.0f, t1 = 0.0f, t2 = 0.0f;
    t0 += _tex2DVecN( iChannel0,uv.x,uv.y,15).w * dt * vf;
    t1 += texture( iChannel0, uv + swi2(ep,x,y) ).w * dt * vf;
    t2 += texture( iChannel0, uv + swi2(ep,y,x) ).w * dt * vf;
    float2 g = to_float2( ( t1 - t0 ), ( t2 - t0 ) ) / swi2(ep,x,x);
    float2 t = to_float2( -g.y, g.x );

    p += 0.9f * t + g * 0.3f;
    rz += t;
    
    float2 fld = rz;
    
    if( cir( p, mou, siz * mul ) > 0.1f && iMouse.z > 0.5f )
            
        fld += forceVector * swi2(_tex2DVecN( iChannel2,uv.x,uv.y,15),x,y);
    
    float o = 0.0f;
    
    if( iFrame <= 4 || KEY_I )
      o = _tex2DVecN( iChannel0,uv.x,uv.y,15).w * 0.99f;
    
    fO += o;
    
    if( uv.y < 0.00f || uv.x < 0.00f || uv.x > 1.0f || uv.y > 1.0f ) o *= 0.0f;
    
    fragColor = to_float4( 0, fld.x, fld.y, fO );
    
    if(iFrame<1 || Reset) fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: London' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0
// Connect Buffer B 'Previsualization: Buffer D' to iChannel3


__DEVICE__ float hash( float2 a )
{
    return fract( _sinf( a.x * 3433.8f + a.y * 3843.98f ) * 45933.8f );
}

__DEVICE__ float noise( float2 uv )
{
    float2 lv = fract_f2( uv );
    lv = lv * lv * ( 3.0f - 2.0f * lv );
    float2 id = _floor( uv );
    
    float bl = hash( id );
    float br = hash( id + to_float2( 1, 0 ) );
    float b = _mix( bl, br, lv.x );
    
    float tl = hash( id + to_float2( 0, 1 ) );
    float tr = hash( id + to_float2_s( 1 ) );
    float t = _mix( tl, tr, lv.x );
    
    float c = _mix( b, t, lv.y );
    
    return c;

}

__DEVICE__ float fbm( float2 uv )
{

    float f = noise( uv * 4.0f );
    f += noise( uv * 8.0f ) * 0.5f;  
    f += noise( uv * 16.0f ) * 0.25f; 
    f += noise( uv * 32.0f ) * 0.125f; 
    f += noise( uv * 64.0f ) * 0.0625f;
    f /= 2.0f;
    
    return f;
}

#ifdef xxx
__DEVICE__ float cur( float2 uv, float2 R, __TEXTURE2D__ iChannel0 )
{
    
    float xpi = 1.0f / iResolution.x;
    float ypi = 1.0f / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;
    
    float top = texture( iChannel0, to_float2( x, y + ypi ) ).x;
    float lef = texture( iChannel0, to_float2( x - xpi, y ) ).x;
    float rig = texture( iChannel0, to_float2( x + xpi, y ) ).x;
    float dow = texture( iChannel0, to_float2( x, y - ypi ) ).x;
    
    float dY = ( top - dow ) * 0.5f;
    float dX = ( rig - lef ) * 0.5f;
    
    return dX * dY;
}

__DEVICE__ float2 vor( float2 uv, float2 R, __TEXTURE2D__ iChannel0 )
{
    
    float2 pre = uv;
    
    float xpi = 1.0f / iResolution.x;
    float ypi = 1.0f / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;

    float2 dir = to_float2_s( 0 );
    dir.y = ( cur( to_float2( x, y + ypi ),R,iChannel0 ) ) - ( cur( to_float2( x, y - ypi ),R,iChannel0 ) );
    dir.x = ( cur( to_float2( x + xpi, y ),R,iChannel0 ) ) - ( cur( to_float2( x - xpi, y ),R,iChannel0 ) );
    
    dir = normalize( dir );
    
    if( length( dir ) > 0.0f )
    
    uv -= dt * vo * cur( uv,R,iChannel0 ) * dir;
    
    return uv;
}
#endif


__DEVICE__ float2 dif( float2 uv, float2 R, __TEXTURE2D__ iChannel0, float di, float rbe, float alp)
{
    float xpi = 1.0f / iResolution.x;
    float ypi = 1.0f / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;
    
    float2 cen = swi2(_tex2DVecN( iChannel0,uv.x,uv.y,15),x,y);
    float2 top = swi2(texture( iChannel0, to_float2( x, y + ypi ) ),x,y);
    float2 lef = swi2(texture( iChannel0, to_float2( x - xpi, y ) ),x,y);
    float2 rig = swi2(texture( iChannel0, to_float2( x + xpi, y ) ),x,y);
    float2 dow = swi2(texture( iChannel0, to_float2( x, y - ypi ) ),x,y);
    
    return ( di * rbe ) * ( top + lef + rig + dow + alp * cen ) * rbe;
}

__DEVICE__ float2 adv( float2 uv, float2 R, float iTimeDelta, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, float dt, float vo )
{
    // Eulerian.
    float2 pre = swi2(texture( iChannel1, vor( uv,R,iChannel0, dt,vo ) ),y,z);
    pre = iTimeDelta * dt * pre;
    
    uv -= pre;
    
    return uv;
}

__DEVICE__ float4 forc( float2 uv, float2 p, float2 mou, __TEXTURE2D__ tex, out float *cen, float iTime, int iFrame, float4 iMouse, float2 R, __TEXTURE2D__ iChannel2, float siz, float forceColour, bool WP )
{
  float4 col = to_float4_s( 0 );
    
  if(WP)
  {  
    if( iFrame <= 10 )
      col += 0.2f * _tex2DVecN( iChannel2,uv.x,uv.y,15);
    
    if( iMouse.z > 0.5f )
      col += cir( p, mou, siz );
  }
  else
  {
    float tim = iTime * 0.1f;
    if( iMouse.z > 0.5f && cir( p, mou, siz ) > 0.1f )
      col += forceColour * to_float4( noise( uv + tim ), noise( uv + tim + 1.0f ), noise( uv + tim + 2.0f ), 1 );
  }
  return col;
}

__DEVICE__ float2 div( float2 uv, float2 p, float2 mou, float4 iMouse, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel3, float dt, float siz, float forceVector )
{
    
    float xpi = 1.0f / iResolution.x;
    float ypi = 1.0f / iResolution.y;
    
    float x = uv.x;
    float y = uv.y;
    
    float cen = _tex2DVecN( iChannel0,uv.x,uv.y,15).w;
    float top = texture( iChannel0, to_float2( x, y + ypi ) ).x;
    float lef = texture( iChannel0, to_float2( x - xpi, y ) ).x;
    float rig = texture( iChannel0, to_float2( x + xpi, y ) ).x;
    float dow = texture( iChannel0, to_float2( x, y - ypi ) ).x;
    
    float dX = dt * ( rig - lef ) * 0.5f;
    float dY = dt * ( top - dow ) * 0.5f;
    
    float2 vel = to_float2_s( 0 );
    
    if( iMouse.z > 0.5f && cir( p, mou, siz ) > 0.1f )
    
    vel = forceVector * swi2(_tex2DVecN( iChannel3,uv.x,uv.y,15),x,y);
    
    return to_float2( dX, dY ) + vel;

}

__DEVICE__ float2 pre( float2 uv, float2 p, float2 mou, float iTime, float4 iMouse, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel3, float di, float dx, float dt, float siz, float forceVector )
{
    float2 pre = uv;
    uv -= ( di * dx * dx ) * div( uv, p, mou, iMouse, R, iChannel0, iChannel3,dt,siz,forceVector );
    
    return uv;

}

__DEVICE__ float2 vel( float2 uv, float2 p, float2 mou, float iTime, float4 iMouse, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel3, float di, float dx, float dt, float siz, float forceVector )
{
    float2 pr = pre( uv, p, mou, iTime, iMouse, R, iChannel0, iChannel3,di, dx,dt,siz,forceVector);
    float2 die = div( uv, p, mou, iMouse, R, iChannel0, iChannel3,dt,siz,forceVector );
    
    uv += dt * die - pr;
    return uv;
}

__DEVICE__ float4 jac( float2 uv, float2 p, float2 mou, out float *cen, float iTime, float iTimeDelta, int iFrame, float4 iMouse, float2 R, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3,
                       float di, float dx, float dt, float siz, float forceVector, float rbe, float alp, float forceColour, float vo,bool WP )
{

    float4 col = to_float4_s( 0.0f ); float dam = 1.0f; float4 colO = to_float4_s( 0 ); float2 pre = uv;
    
    float2 tem = uv;
 
    uv = adv( uv, R, iTimeDelta, iChannel0, iChannel1,dt,vo );
    uv -= dt * ( vel( uv, p, mou, iTime, iMouse, R, iChannel0, iChannel3,di,dx,dt,siz,forceVector ) * dif( uv,R,iChannel0,di,rbe,alp ) );
       
    col += forc( uv, p, mou, iChannel0, cen, iTime, iFrame, iMouse, R, iChannel2,siz,forceColour,WP );
    colO = _tex2DVecN( iChannel0,uv.x,uv.y,15) + col * dt;
    colO *= 0.99f;
float bbbbbbbbbbbbbbbbbbbbb;    
    if( pre.y < 0.01f || pre.x < 0.01f || pre.x > 1.0f || pre.y > 1.0f ) colO *= 0.0f;
    
    return colO;
}

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
    
  return Q;
}



__KERNEL__ void NavierstokeishJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, int iFrame, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_CHECKBOX2(Waterpainting, 0); 
    
        //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    fragCoord+=0.5f;

    CONNECT_SLIDER6(forceVector, -10.0f, 30.0f, 10.0f);
    CONNECT_SLIDER7(forceColour, -10.0f, 30.0f, 10.0f);
    CONNECT_SLIDER8(dx, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER9(DT, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER10(siz, -1.0f, 1.0f, 0.05f);
    CONNECT_SLIDER11(di, -1.0f, 3.0f, 1.25f);
    CONNECT_SLIDER12(RBE, -1.0f, 10.0f, 4.0f);
    CONNECT_SLIDER13(vo, -10.0f, 30.0f, 10.0f);
    CONNECT_SLIDER14(vf, -1.0f, 1.0f, 0.0025f);
    CONNECT_SLIDER15(mul, -10.0f, 30.0f, 10.0f);
    
    //const float forceVector = 10.0;
    //const float forceColour = 10.0;
    //const float dx = 0.5;
    const float dt = dx * dx * DT;//0.5;
    //const float siz = 0.05;
    //const float di = 1.25;
    const float alp = ( dx * dx ) / dt;
    const float rbe = 1.0 / (RBE + alp);//( 4.0 + alp );
    //const float vo = 10.0;
    //const float vf = 0.0025;
    //const float mul = 10.0;
    const float e = 0.05;

    float2 uv = fragCoord / iResolution;
    float2 p = fragCoord / iResolution.y;

    float2 mou = swi2(iMouse,x,y) / iResolution.y;
    //float ini = 0.0f;
    float cen = 0.0f;
    
    float4 colO = jac( uv, p, mou, &cen, iTime, iTimeDelta, iFrame, iMouse, R, iChannel0, iChannel1, iChannel2, iChannel3,
                       di,dx,dt,siz,forceVector,rbe,alp,forceColour,vo,Waterpainting);
    
    fragColor = colO;
    
    if (Blend1>0.0) fragColor = Blending(iChannel2, fragCoord/R, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);
    
    if(iFrame<1 || Reset) fragColor = to_float4_s(0.0f);
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void NavierstokeishJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f;
    
    CONNECT_SLIDER6(forceVector, -10.0f, 30.0f, 10.0f);
    CONNECT_SLIDER10(siz, -1.0f, 1.0f, 0.05f);
    
    //const float forceVector = 10.0;
    //const float siz = 0.05;
   
    float2 p = fragCoord / iResolution.y;
    float2 uv = fragCoord / iResolution;
    float2 mou = swi2(iMouse,x,y) / iResolution.y;
    float2 last = swi2(texture( iChannel0, uv),z,w);
float CCCCCCCCCCCCCCCCCCC;    
    float2 acc = to_float2_s(0);
    if( cir( p, mou, siz ) > 0.05f )
      acc += ( mou - last ) * forceVector;
    
    fragColor = to_float4_f2f2( acc, mou );

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel2
// Connect Image 'Previsualization: Buffer B' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel3


struct C_Sample
{
  float3 vAlbedo;
  float3 vNormal;
};
  
__DEVICE__ C_Sample SampleMaterial(const in float2 vUV, __TEXTURE2D__ sampler,  const in float2 vTextureSize, const in float fNormalScale, bool bumpmap,float4 Color1)
{
  C_Sample result;
  
  float2 vInvTextureSize = to_float2_s(1.0f) / vTextureSize;
  
  float3 cSampleNegXNegY = swi3(texture(sampler, vUV + (to_float2(-1.0f, -1.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);
  float3 cSampleZerXNegY = swi3(texture(sampler, vUV + (to_float2( 0.0f, -1.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);
  float3 cSamplePosXNegY = swi3(texture(sampler, vUV + (to_float2( 1.0f, -1.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);
  
  float3 cSampleNegXZerY = swi3(texture(sampler, vUV + (to_float2(-1.0f, 0.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);
  float3 cSampleZerXZerY = swi3(texture(sampler, vUV + (to_float2( 0.0f, 0.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);
  float3 cSamplePosXZerY = swi3(texture(sampler, vUV + (to_float2( 1.0f, 0.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);
  
  float3 cSampleNegXPosY = swi3(texture(sampler, vUV + (to_float2(-1.0f,  1.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);
  float3 cSampleZerXPosY = swi3(texture(sampler, vUV + (to_float2( 0.0f,  1.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);
  float3 cSamplePosXPosY = swi3(texture(sampler, vUV + (to_float2( 1.0f,  1.0f)) * swi2(vInvTextureSize,x,y)),x,y,z);

  // convert to linear  
  float3 cLSampleNegXNegY = cSampleNegXNegY * cSampleNegXNegY;
  float3 cLSampleZerXNegY = cSampleZerXNegY * cSampleZerXNegY;
  float3 cLSamplePosXNegY = cSamplePosXNegY * cSamplePosXNegY;

  float3 cLSampleNegXZerY = cSampleNegXZerY * cSampleNegXZerY;
  float3 cLSampleZerXZerY = cSampleZerXZerY * cSampleZerXZerY;
  float3 cLSamplePosXZerY = cSamplePosXZerY * cSamplePosXZerY;

  float3 cLSampleNegXPosY = cSampleNegXPosY * cSampleNegXPosY;
  float3 cLSampleZerXPosY = cSampleZerXPosY * cSampleZerXPosY;
  float3 cLSamplePosXPosY = cSamplePosXPosY * cSamplePosXPosY;

  // Average samples to get albdeo colour
  result.vAlbedo = ( cLSampleNegXNegY + cLSampleZerXNegY + cLSamplePosXNegY 
                   + cLSampleNegXZerY + cLSampleZerXZerY + cLSamplePosXZerY
                   + cLSampleNegXPosY + cLSampleZerXPosY + cLSamplePosXPosY ) / 9.0f;  
  
  float3 vScale = swi3(Color1,x,y,z);//to_float3_s(0.3333f);
  
  //#ifdef USE_LINEAR_FOR_BUMPMAP

    float fSampleNegXNegY = dot(cLSampleNegXNegY, vScale);
    float fSampleZerXNegY = dot(cLSampleZerXNegY, vScale);
    float fSamplePosXNegY = dot(cLSamplePosXNegY, vScale);
    
    float fSampleNegXZerY = dot(cLSampleNegXZerY, vScale);
    float fSampleZerXZerY = dot(cLSampleZerXZerY, vScale);
    float fSamplePosXZerY = dot(cLSamplePosXZerY, vScale);
    
    float fSampleNegXPosY = dot(cLSampleNegXPosY, vScale);
    float fSampleZerXPosY = dot(cLSampleZerXPosY, vScale);
    float fSamplePosXPosY = dot(cLSamplePosXPosY, vScale);
  
  if(bumpmap)  
  {
      fSampleNegXNegY = dot(cSampleNegXNegY, vScale);
      fSampleZerXNegY = dot(cSampleZerXNegY, vScale);
      fSamplePosXNegY = dot(cSamplePosXNegY, vScale);
      
      fSampleNegXZerY = dot(cSampleNegXZerY, vScale);
      fSampleZerXZerY = dot(cSampleZerXZerY, vScale);
      fSamplePosXZerY = dot(cSamplePosXZerY, vScale);
      
      fSampleNegXPosY = dot(cSampleNegXPosY, vScale);
      fSampleZerXPosY = dot(cSampleZerXPosY, vScale);
      fSamplePosXPosY = dot(cSamplePosXPosY, vScale);  
  }
  
  // Sobel operator - http://en.wikipedia.org/wiki/Sobel_operator
  
  float2 vEdge;
  vEdge.x = (fSampleNegXNegY - fSamplePosXNegY) * 0.25f 
          + (fSampleNegXZerY - fSamplePosXZerY) * 0.5
          + (fSampleNegXPosY - fSamplePosXPosY) * 0.25f;

  vEdge.y = (fSampleNegXNegY - fSampleNegXPosY) * 0.25f 
          + (fSampleZerXNegY - fSampleZerXPosY) * 0.5
          + (fSamplePosXNegY - fSamplePosXPosY) * 0.25f;

  result.vNormal = normalize(to_float3_aw(vEdge * fNormalScale, 1.0f));  
 
  return result;
}

__KERNEL__ void NavierstokeishJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
  
  CONNECT_COLOR0(Color1, 0.333f, 0.333f, 0.333f, 1.0f);
  CONNECT_SLIDER0(fLightHeight, -1.0f, 1.0f, 0.2f);
  CONNECT_SLIDER1(fViewHeight, -1.0f, 5.0f, 2.0f);

  
  CONNECT_CHECKBOX3(Bumpmap, 1);
  CONNECT_CHECKBOX4(SHOW_NORMAL_MAP, 0);
  CONNECT_CHECKBOX5(SHOW_ALBEDO, 0);
  
  CONNECT_CHECKBOX6(Alpha, 0);
  CONNECT_SLIDER5(AlphaThres, 0.0f, 1.0f, 0.0f);
  
  
  fragCoord+=0.5f;
  
  float2 vUV = fragCoord / iResolution;

  C_Sample materialSample;
    
  float fNormalScale = 10.0f;
  materialSample = SampleMaterial( vUV, iChannel0, iResolution, fNormalScale, Bumpmap,Color1 );
  
  // Random Lighting...
  
  //float fLightHeight = 0.2f;
  //float fViewHeight = 2.0f;
  
  float3 vSurfacePos = to_float3_aw(vUV, 0.0f);
  
  float3 vViewPos = to_float3(0.5f, 0.5f, fViewHeight);
      
  float3 vLightPos = to_float3_aw( to_float2(_sinf(iTime),_cosf(iTime)) * 0.25f + 0.5f , fLightHeight);
    
  if( iMouse.z > 0.0f )
  {
    vLightPos = to_float3_aw(swi2(iMouse,x,y) / iResolution, fLightHeight);
  }
  
  float3 vDirToView = normalize( vViewPos - vSurfacePos );
  float3 vDirToLight = normalize( vLightPos - vSurfacePos );
    
  float fNDotL = clamp( dot(materialSample.vNormal, vDirToLight), 0.0f, 1.0f);
  float fDiffuse = fNDotL;
  
  float3 vHalf = normalize( vDirToView + vDirToLight );
  float fNDotH = clamp( dot(materialSample.vNormal, vHalf), 0.0f, 1.0f);
  float fSpec = _powf(fNDotH, 10.0f) * fNDotL * 0.5f;
  
  float3 vResult = materialSample.vAlbedo * fDiffuse + fSpec;
  
  vResult = sqrt_f3(vResult);
  
  if(SHOW_NORMAL_MAP)
    vResult = materialSample.vNormal * 0.5f + 0.5f;
  
  
  if(SHOW_ALBEDO)
    vResult = sqrt_f3(materialSample.vAlbedo);
  
  if(Alpha)
  {
    float tex = texture(iChannel0,fragCoord/R).w <= AlphaThres ? 0.0f : 1.0f;
        
    vResult *= tex;
    Color1.w = tex == 0.0f ? Color1.w : 1.0f;
  }
  
  fragColor = to_float4_aw(vResult,Color1.w);
 
  SetFragmentShaderComputedColor(fragColor);
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0


// Frozen wasteland
// By Dave Hoskins
// https://www.shadertoy.com/view/Xls3D2
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


#define ITR 90
#define FAR 110.0f
#define time iTime
#define MOD3 to_float3(0.16532f,0.17369f,0.15787f)
#define SUN_COLOUR  to_float3(1.0f, 0.95f, 0.85f)

//#define TRIANGLE_NOISE  // .. This
//#define FOUR_D_NOISE  // ...Or this
//#define TEXTURE_NOISE    // .. Or this (faster, but not as sharp edged)
//#define VALUE_NOISE     // .. or more normal noise.


__DEVICE__ float height(in float2 p)
{
    float h = _sinf(p.x*0.1f+p.y*0.2f)+_sinf(p.y*0.1f-p.x*0.2f)*0.5f;
    h += _sinf(p.x*0.04f+p.y*0.01f+3.0f)*4.0f;
    h -= _sinf(h*10.0f)*0.1f;
    return h;
}

__DEVICE__ float camHeight(in float2 p)
{
    float h = _sinf(p.x*0.1f+p.y*0.2f)+_sinf(p.y*0.1f-p.x*0.2f)*0.5f;
    h += _sinf(p.x*0.04f+p.y*0.01f+3.0f)*4.0f;
    return h;
}

__DEVICE__ float smin( float a, float b)
{
  const float k = 2.7f;
  float h = clamp( 0.5f + 0.5f*(b-a)/k, 0.0f, 1.0f );
  return _mix( b, a, h ) - k*h*(1.0f-h);
}

#define MOD2 to_float2(0.16632f,0.17369f)
#define MOD3 to_float3(0.16532f,0.17369f,0.15787f)
__DEVICE__ float tri(in float x){return _fabs(fract(x)-0.5f);}

__DEVICE__ float hash12(float2 p)
{
  p  = fract(p * MOD2);
    p += dot(swi2(p,x,y), swi2(p,y,x)+19.19f);
    return fract(p.x * p.y);
}
__DEVICE__ float vine(float3 p, in float c, in float h)
{
    p.y += _sinf(p.z*0.5625f+1.3f)*3.5f-0.5f;
    p.x += _cosf(p.z*2.0f)*1.0f;
    float2 q = to_float2(mod_f(p.x, c)-c/2.0f, p.y);
    return length(q) - h*1.4f -_sinf(p.z*3.0f+_sinf(p.x*7.0f)*0.5f)*0.1f;
}

//========================================================================
// ################ DIFFERENT NOISE FUNCTIONS ################
//#ifdef TRIANGLE_NOISE
__DEVICE__ float3 tri3(in float3 p){return to_float3( tri(p.z+tri(p.y)), tri(p.z+tri(p.x)), tri(p.y+tri(p.x)));}
__DEVICE__ float Noise3dTN(in float3 p, float iTime, __TEXTURE2D__ iChannel0)
{
  float z=1.4f;
  float rz = 0.0f;
  float3 bp = p;
  for (float i=0.0f; i<= 2.0f; i+=1.0f )
  {
    float3 dg = tri3(bp);
    p += (dg);

    bp *= 2.0f;
    z *= 1.5f;
    p *= 1.3f;
        
    rz+= (tri(p.z+tri(p.x+tri(p.y))))/z;
    bp += 0.14f;
  }
  return rz;
}


//--------------------------------------------------------------------------------
//#ifdef FOUR_D_NOISE

__DEVICE__ float4 quad(in float4 p){return abs_f4(fract_f4(swi4(p,y,z,w,x)+swi4(p,w,z,x,y))-0.5f);}

__DEVICE__ float Noise3dFD(in float3 q, float iTime, __TEXTURE2D__ iChannel0)
{
  float z=1.4f;
  float4 p = to_float4_aw(q, iTime*0.1f);
  float rz = 0.0f;
  float4 bp = p;
  for (float i=0.0f; i<= 2.0f; i+=1.0f )
  {
    float4 dg = quad(bp);
    p += (dg);

    z *= 1.5f;
    p *= 1.3f;
        
    rz+= (tri(p.z+tri(p.w+tri(p.y+tri(p.x)))))/z;
    
    bp = swi4(bp,y,x,z,w)*2.0f+0.14f;
  }
  return rz;
}


//--------------------------------------------------------------------------------
//#ifdef TEXTURE_NOISE
__DEVICE__ float Noise3dTEX(in float3 _x, float iTime, __TEXTURE2D__ iChannel0)
{

    _x*=10.0f;
    float h = 0.0f;
    float a = 0.34f;
    for (int i = 0; i < 4; i++)
    {
        float3 p = _floor(_x);
        float3 f = fract_f3(_x);
        f = f*f*(3.0f-2.0f*f);

        float2 uv = (swi2(p,x,y)+to_float2(37.0f,17.0f)*p.z) + swi2(f,x,y);
        float2 rg = swi2(texture( iChannel0, (uv+ 0.5f)/256.0f),y,x);
        h += _mix( rg.x, rg.y, f.z )*a;
        a*=0.5f;
        _x+=_x;
    }
    return h;
}



//--------------------------------------------------------------------------------
//#ifdef VALUE_NOISE
__DEVICE__ float Hash(float3 p)
{
  p  = fract(p * MOD3);
  p += dot(swi3(p,x,y,z), swi3(p,y,z,x) + 19.19f);
  return fract(p.x * p.y * p.z);
}

__DEVICE__ float Noise3dVN(in float3 p, float iTime, __TEXTURE2D__ iChannel0)
{
    float2 add = to_float2(1.0f, 0.0f);
    p *= 10.0f;
    float h = 0.0f;
    float a = 0.3f;
    for (int n = 0; n < 4; n++)
    {
        float3 i = _floor(p);
        float3 f = fract_f3(p); 
        f *= f * (3.0f-2.0f*f);

        h += _mix(
             _mix(_mix(Hash(i), Hash(i + swi3(add,x,y,y)),f.x),
                  _mix(Hash(i + swi3(add,y,x,y)), Hash(i + swi3(add,x,x,y)),f.x),
                  f.y),
             _mix(_mix(Hash(i + swi3(add,y,y,x)), Hash(i + swi3(add,x,y,x)),f.x),
                  _mix(Hash(i + swi3(add,y,x,x)), Hash(i + swi3(add,x,x,x)),f.x),
                  f.y),
                  f.z)*a;
        a*=0.5f;
        p += p;
    }
    return h;
}


//Function
__DEVICE__ float Noise3d(in float3 p, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{

  if( NoiseModus == 1)  //#define TRIANGLE_NOISE	// .. This
  {  
     return Noise3dTN(p,iTime,iChannel0);
  }
  else if ( NoiseModus == 2) //#define FOUR_D_NOISE	// ...Or this
  {  
     return Noise3dFD(p,iTime,iChannel0);
  }
  else if ( NoiseModus == 3) //#define TEXTURE_NOISE		// .. Or this (faster, but not as sharp edged)
  {  
     return Noise3dTEX(p,iTime,iChannel0);
  }
  //else if ( NoiseModus == 3) //#define VALUE_NOISE 		// .. or more normal noise.
  {  
     return Noise3dVN(p,iTime,iChannel0);
  }

}
//--------------------------------------------------------------------------------
__DEVICE__ float map(float3 p, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{
    p.y += height(swi2(p,z,x));
    float d = p.y+0.5f;
    
    d = smin(d, vine(p+to_float3(0.8f,0.0f,0),30.0f,3.3f) );
    d = smin(d, vine(swi3(p,z,y,x)+to_float3(0.0f,0,17.0f),33.0f,1.4f) );
    d += Noise3d(p*0.05f, iTime, iChannel0, NoiseModus)*(p.y*1.2f);
    //swi2(p,x,z) *=0.3f;
    p.x *=0.3f;
    p.z *=0.3f;
    
    d+= Noise3d(p*0.3f, iTime, iChannel0, NoiseModus);
    return d;
}
__DEVICE__ float fogmap(in float3 p, in float d, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{
    swi2S(p,x,z, swi2(p,x,z) - time*7.0f+_sinf(p.z*0.3f)*3.0f);
    p.y -= time*0.5f;
    return (_fmaxf(Noise3d(p*0.008f+0.1f, iTime, iChannel0, NoiseModus)-0.1f,0.0f)*Noise3d(p*0.1f, iTime, iChannel0, NoiseModus))*0.3f;
}

__DEVICE__ float march(in float3 ro, in float3 rd, out float *drift, in float2 scUV, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{
    float precis = 0.1f;
    float mul = 0.34f;
    float h;
    float d = hash12(scUV)*1.5f;
    *drift = 0.0f;
    for( int i=0; i<ITR; i++ )
    {
        float3 p = ro+rd*d;
        h = map(p, iTime, iChannel0, NoiseModus);
        if(h < precis*(1.0f+d*0.05f) || d > FAR) break;
        *drift +=  fogmap(p, d, iTime, iChannel0, NoiseModus);
        d += h*mul;
        mul+=0.004f;
        //precis +=0.001f;
    }
    *drift = _fminf(*drift, 1.0f);
  return d;
}

__DEVICE__ float3 normal( in float3 pos, in float d, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus )
{
  float2 eps = to_float2( d *d* 0.003f+0.01f, 0.0f);
  float3 nor = to_float3(
                          map(pos+swi3(eps,x,y,y), iTime, iChannel0, NoiseModus) - map(pos-swi3(eps,x,y,y), iTime, iChannel0, NoiseModus),
                          map(pos+swi3(eps,y,x,y), iTime, iChannel0, NoiseModus) - map(pos-swi3(eps,y,x,y), iTime, iChannel0, NoiseModus),
                          map(pos+swi3(eps,y,y,x), iTime, iChannel0, NoiseModus) - map(pos-swi3(eps,y,y,x), iTime, iChannel0, NoiseModus) );
  return normalize(nor);
}

__DEVICE__ float bnoise(in float3 p, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{
    //swi2(p,x,z)*=0.4f;
    p.x *= 0.4f;
    p.z *= 0.4f;
    
    float n = Noise3d(p*3.0f, iTime, iChannel0, NoiseModus)*0.4f;
    n += Noise3d(p*1.5f, iTime, iChannel0, NoiseModus)*0.2f;
    return n*n*0.2f;
}

__DEVICE__ float3 bump(in float3 p, in float3 n, in float ds, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{
    //swi2(p,x,z) *= 0.4f;
    p.x *= 0.4f;
    p.z *= 0.4f;
    //p *= 1.0f;
    float2 e = to_float2(0.01f,0);
    float n0 = bnoise(p, iTime, iChannel0, NoiseModus);
    float3 d = to_float3(bnoise(p+swi3(e,x,y,y), iTime, iChannel0, NoiseModus)-n0, bnoise(p+swi3(e,y,x,y), iTime, iChannel0, NoiseModus)-n0, bnoise(p+swi3(e,y,y,x), iTime, iChannel0, NoiseModus)-n0)/e.x;
    n = normalize(n-d*10.0f/(ds));
    return n;
}

__DEVICE__ float shadow(in float3 ro, in float3 rd, in float mint, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{
    float res = 1.0f;
    
    float t = mint;
    for( int i=0; i<12; i++ )
    {
      float h = map(ro + rd*t, iTime, iChannel0, NoiseModus);
      res = _fminf( res, 4.0f*h/t );
      t += clamp( h, 0.1f, 1.5f );
    }
    return clamp( res, 0.0f, 1.0f );
}

__DEVICE__ float3 Clouds(float3 sky, float3 rd, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{
    
  rd.y = _fmaxf(rd.y, 0.0f);
  float ele = rd.y;
  float v = (200.0f)/(_fabs(rd.y)+0.01f);

  rd.y = v;
  swi2S(rd,x,z, swi2(rd,x,z) * v - time*8.0f);
  //swi2(rd,x,z) *= 0.0004f;
  rd.x *= 0.0004f;
  rd.z *= 0.0004f;
    
  float f = Noise3d(swi3(rd,x,z,z)*3.0f, iTime, iChannel0, NoiseModus) * Noise3d(swi3(rd,z,x,x)*1.3f, iTime, iChannel0, NoiseModus)*2.5f;
  f = f*_powf(ele, 0.5f)*2.0f;
  f = clamp(f-0.15f, 0.01f, 1.0f);

  return  _mix(sky, to_float3_s(1),f );
}


__DEVICE__ float3 Sky(float3 rd, float3 ligt, float3 SkyColor)
{
    rd.y = _fmaxf(rd.y, 0.0f);
    
    //float3 sky = _mix(to_float3(0.1f, 0.15f, 0.25f), to_float3_s(0.8f), _powf(0.8f-rd.y, 3.0f));
    float3 sky = _mix(SkyColor, to_float3_s(0.8f), _powf(0.8f-rd.y, 3.0f));
    return  _mix(sky, SUN_COLOUR, _fminf(_powf(_fmaxf(dot(rd,ligt), 0.0f), 4.5f)*1.2f, 1.0f));
}
__DEVICE__ float Occ(float3 p, float iTime, __TEXTURE2D__ iChannel0, int NoiseModus)
{
    float h = 0.0f;
    h  = clamp(map(p, iTime, iChannel0, NoiseModus), 0.5f, 1.0f);
   return _sqrtf(h);   
}



__KERNEL__ void FrozenWastelandFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_COLOR1(SkyColor, 0.1f, 0.15f, 0.25f, 1.0f);
  
  CONNECT_BUTTON0(NoiseModus, 0, Triangle, FourD, Texture, Value, BTN5);
  
  
  float2 p = fragCoord/iResolution-0.5f;
  float2 q = fragCoord/iResolution;
  p.x*=iResolution.x/iResolution.y;
  float2 mo = swi2(iMouse,x,y) / iResolution-0.5f;
  //mo = ( mo==to_float2(-0.5f) ) ? mo=to_float2(-0.1f,0.07f) : mo;
  mo = ( mo.x==-0.5f && mo.y==-0.5f ) ? to_float2(-0.1f,0.07f) : mo;
    
  mo.x *= iResolution.x/iResolution.y;
  
  float3 ro = to_float3(0.0f+smoothstep(0.0f,1.0f,tri(time*1.5f)*0.3f)*1.5f, smoothstep(0.0f,1.0f,tri(time*3.0f)*3.0f)*0.08f, -time*3.5f-130.0f);
  ro.y -= camHeight(swi2(ro,z,x))-0.4f;
  mo.x += smoothstep(0.7f,1.0f,_sinf(time*0.35f))*0.5f-1.5f - smoothstep(-0.7f,-1.0f,_sinf(time*0.35f))*0.5f;
 
  float3 eyedir = normalize(to_float3(_cosf(mo.x),mo.y*2.0f-0.05f+_sinf(time*0.5f)*0.1f,_sinf(mo.x)));
  float3 rightdir = normalize(to_float3(_cosf(mo.x+1.5708f),0.0f,_sinf(mo.x+1.5708f)));
  float3 updir = normalize(cross(rightdir,eyedir));
  float3 rd = normalize((p.x*rightdir+p.y*updir)*1.0f+eyedir);
  
  float3 ligt = normalize( to_float3(1.5f, 0.9f, -0.5f) );
  float fg;
  float rz = march(ro,rd, &fg, fragCoord, iTime, iChannel0, NoiseModus);
  float3 sky = Sky(rd, ligt, swi3(SkyColor,x,y,z));
    
  float3 col = sky;
 
  if ( rz < FAR )
  {
      float3 pos = ro+rz*rd;
      float3 nor= normal( pos, rz, iTime, iChannel0, NoiseModus);
      float d = distance_f3(pos,ro);
      nor = bump(pos,nor,d, iTime, iChannel0, NoiseModus);
      float shd = shadow(pos,ligt,0.04f, iTime, iChannel0, NoiseModus);
      
      float dif = clamp( dot( nor, ligt ), 0.0f, 1.0f );
      float3 ref = reflect(rd,nor);
      float spe = _powf(clamp( dot( ref, ligt ), 0.0f, 1.0f ),5.0f)*2.0f;

      float fre = _powf( clamp(1.0f+dot(rd, nor),0.0f,1.0f), 3.0f );
      col = to_float3_s(0.75f);
      col = col*dif*shd + fre*spe*shd*SUN_COLOUR +_fabs(nor.y)*to_float3(0.12f, 0.13f, 0.13f);
      // Fake the red absorption of ice...
      d = Occ(pos+nor*3.0f, iTime, iChannel0, NoiseModus);
      col *= to_float3(d, d, _fminf(d*1.2f, 1.0f));
      // Fog from ice storm...
      col = _mix(col, sky, smoothstep(FAR-25.0f,FAR,rz));
      
  }
  else
  {
      col = Clouds(col, rd, iTime, iChannel0, NoiseModus);
  }
    

  // Fog mix...
  col = _mix(col, to_float3(0.6f, 0.65f, 0.7f), fg);

  // Post...
  col = _mix(col, to_float3_s(0.5f), -0.3f);
  //col = col*col * (3.0f-2.0f*col);
  //col = clamp(_powf(col,to_float3_s(1.5f)),0.0f, 1.0f);

  col = sqrt_f3(col);
    
  // Borders...
  float f = smoothstep(0.0f, 3.0f, iTime)*0.5f;
  col *= f+f*_powf(70.0f *q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.2f);
    
  fragColor = to_float4_aw( col+swi3(Color,x,y,z)-0.5f, Color.w );

  SetFragmentShaderComputedColor(fragColor);
}

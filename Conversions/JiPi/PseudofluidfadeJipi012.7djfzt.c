
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Abstract 1' to iChannel0

#define R iResolution

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float hash12(float2 p){
  float3 p3  = fract_f3(swi3(p,x,y,x) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 9.0f);
  return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float noise(float2 _x){

  float2 f = fract(_x)*fract(_x)*(3.0f-2.0f*fract(_x));
  return _mix(_mix(hash12(_floor(_x)),
                   hash12(_floor(_x)+to_float2(1,0)),f.x),
              _mix(hash12(_floor(_x)+to_float2(0,1)),
                   hash12(_floor(_x)+to_float2_s(1)),f.x),f.y);
}

__DEVICE__ float4 circle(float2 uv, float2 pos, float sz, float iTime){
    // draw a circle at mouse coordinates
    float s = (3.0f+0.0f*_powf(noise(to_float2_s(iTime*0.5f)),2.0f))/sz;
    uv += pos+to_float2_s(1.0f/s);
    float val = clamp(1.0f-length(s*uv-1.0f), 0.0f, 1.0f);
    val = _powf(5.0f*val, 1.0f);
    return to_float4_s(clamp(val, 0.0f, 1.0f));
}

__DEVICE__ float2 hash21(float p){
    float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

__KERNEL__ void PseudofluidfadeJipi012Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_CHECKBOX1(Textur, 0);

    CONNECT_SLIDER0(Noise, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER1(Noise2, -10.0f, 10.0f, 0.88f);

    fragCoord+=0.5f;

    float2 uv = fragCoord / swi2(iResolution,y,y);
    float brightness = 1.0f;//0.6f+0.4f*noise(to_float2(iTime*2.9f));
//    fragColor = circle(uv, -1.0f*to_float2(0.888888f, 0.35f), 1.0f, iTime)-circle(uv, -1.0f*to_float2(0.888888f, 0.35f), 0.88f, iTime);
    //fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15)*circle(uv, -to_float2(0.888888f, 0.35f), 0.88f, iTime)*clamp(1.0f, 0.0f, 1.0f);

    if (Textur)
    {
       float2 tuv = fragCoord/iResolution;
       float4 tex = _tex2DVecN(iChannel0,tuv.x,tuv.y,15);
       
       if ( tex.w > 0.0f )
       {
          //float s = (3.0f+Noise*_powf(noise(to_float2_s(iTime*0.5f)),2.0f))/Noise2; 
          float s = noise(to_float2_s(Noise2)) * Noise;
          fragColor = to_float4_s(s);
       }
    }
    else
      fragColor = circle(uv, -1.0f*to_float2(0.888888f, 0.35f), 1.0f, iTime)-circle(uv, -1.0f*to_float2(0.888888f, 0.35f), 0.88f, iTime);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// simplex noise obviously not by me, see main() below
__DEVICE__ float3 mod289(float3 _x){
  return _x - _floor(_x * (1.0f / 289.0f)) * 289.0f;
}

__DEVICE__ float4 mod289(float4 _x){
  return _x - _floor(_x * (1.0f / 289.0f)) * 289.0f;
}

__DEVICE__ float4 permute(float4 _x){
  return mod289(((_x*34.0f)+1.0f)*_x);
}

__DEVICE__ float4 taylorInvSqrt(float4 r){
  return to_float4_s(1.79284291400159f) - 0.85373472095314f * r;
}

__DEVICE__ float simplex(float3 v){
  const float2  C = to_float2(1.0f/6.0f, 1.0f/3.0f) ;
  const float4  D = to_float4(0.0f, 0.5f, 1.0f, 2.0f);

  float3 i  = _floor(v + dot(v, swi3(C,y,y,y)) );
  float3 x0 =   v - i + dot(i, swi3(C,x,x,x)) ;

  float3 g = step(swi3(x0,y,z,x), swi3(x0,x,y,z));
  float3 l = 1.0f - g;
  float3 i1 = _fminf( swi3(g,x,y,z), swi3(l,z,x,y) );
  float3 i2 = _fmaxf( swi3(g,x,y,z), swi3(l,z,x,y) );

  float3 x1 = x0 - i1 + swi3(C,x,x,x);
  float3 x2 = x0 - i2 + swi3(C,y,y,y);
  float3 x3 = x0 - swi3(D,y,y,y);

  i = mod289(i);
  float4 p = permute( permute( permute(
             i.z + to_float4(0.0f, i1.z, i2.z, 1.0f ))
           + i.y + to_float4(0.0f, i1.y, i2.y, 1.0f ))
           + i.x + to_float4(0.0f, i1.x, i2.x, 1.0f ));

  float n_ = 0.142857142857f;
  float3  ns = n_ * swi3(D,w,y,z) - swi3(D,x,z,x);

  float4 j = p - 49.0f * _floor(p * ns.z * ns.z);

  float4 x_ = _floor(j * ns.z);
  float4 y_ = _floor(j - 7.0f * x_ );

  float4 _x = x_ *ns.x + swi4(ns,y,y,y,y);
  float4 _y = y_ *ns.x + swi4(ns,y,y,y,y);
  float4 h = to_float4_s(1.0f) - abs_f4(_x) - abs_f4(_y);

  float4 b0 = to_float4_f2f2( swi2(_x,x,y), swi2(_y,x,y) );
  float4 b1 = to_float4_f2f2( swi2(_x,z,w), swi2(_y,z,w) );

  float4 s0 = _floor(b0)*2.0f + 1.0f;
  float4 s1 = _floor(b1)*2.0f + 1.0f;
  float4 sh = -1.0f*step(h, to_float4_s(0.0f));

  float4 a0 = swi4(b0,x,z,y,w) + swi4(s0,x,z,y,w)*swi4(sh,x,x,y,y) ;
  float4 a1 = swi4(b1,x,z,y,w) + swi4(s1,x,z,y,w)*swi4(sh,z,z,w,w) ;

  float3 p0 = to_float3_aw(swi2(a0,x,y),h.x);
  float3 p1 = to_float3_aw(swi2(a0,z,w),h.y);
  float3 p2 = to_float3_aw(swi2(a1,x,y),h.z);
  float3 p3 = to_float3_aw(swi2(a1,z,w),h.w);

  float4 norm = taylorInvSqrt(to_float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

  float4 m = _fmaxf(to_float4_s(0.6f) - to_float4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), to_float4_s(0.0f));
  m = m * m;
  return 42.0f * dot( m*m, to_float4( dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3) ) );
  }

__DEVICE__ float fbm3(float3 v) {

    float result = simplex(v);
    result += simplex(v * 2.0f) / 2.0f;
    result += simplex(v * 4.0f) / 4.0f;
    result /= (1.0f + 1.0f/2.0f + 1.0f/4.0f);
    return result;
}

__DEVICE__ float3 snoiseVec3( float3 _x ){

  float s  = simplex(( _x ));
  float s1 = simplex(to_float3( _x.y - 19.1f , _x.z + 33.4f , _x.x + 47.2f ));
  float s2 = simplex(to_float3( _x.z + 74.2f , _x.x - 124.5f , _x.y + 99.4f ));
  float3 c = to_float3( s , s1 , s2 );
  return c;

}

__DEVICE__ float3 curlNoise(float3 p)
{
  const float e = 0.1f;
  float3 dx = to_float3( e    , 0.0f , 0.0f );
  float3 dy = to_float3( 0.0f , e    , 0.0f );
  float3 dz = to_float3( 0.0f , 0.0f , e   );

  float3 p_x0 = snoiseVec3( p - dx );
  float3 p_x1 = snoiseVec3( p + dx );
  float3 p_y0 = snoiseVec3( p - dy );
  float3 p_y1 = snoiseVec3( p + dy );
  float3 p_z0 = snoiseVec3( p - dz );
  float3 p_z1 = snoiseVec3( p + dz );

  float x = p_y1.z - p_y0.z - p_z1.y + p_z0.y;
  float y = p_z1.x - p_z0.x - p_x1.z + p_x0.z;
  float z = p_x1.y - p_x0.y - p_y1.x + p_y0.x;

  //const float divisor = 1.0f / ( 2.0f * e );
  //return normalize( to_float3( x , y , z ) * divisor );
  // technically incorrect but I like this better...
  return to_float3( x , y , z );
}

__KERNEL__ void PseudofluidfadeJipi012Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{



    fragCoord+=0.5f;

    float2 uv = -1.0f+2.0f*fragCoord / iResolution;
    uv.x *= iResolution.x/iResolution.y;
    uv *= 1.25f;
    
    // noise seed
    float3 v = to_float3_aw(uv, iTime*0.3f);
    const float disp_freq = 0.6f;
    //swi2(v,x,y) *= disp_freq;
    v.x *= disp_freq;
    v.y *= disp_freq;
    
    // get first some density variation
    float4 disp = 0.5f*_mix(to_float4_s(0), pow_f4(0.5f+0.5f*to_float4_s(fbm3(-2.0f*v+11.2f)), to_float4_s(2)), 0.05f);
    //swi2(v,x,y) /= disp_freq;
    v.x /= disp_freq;
    v.y /= disp_freq;
    
    // add to randomization coordinates
    v += disp.x*24.0f;
    
    // vector field ("fluid" direction)
    float2 off =  0.25f*swi2(curlNoise((v)),x,y);// to_float2(fbm3(v), fbm3(v+99.0f));
    off.y = _fminf(0.0f, off.y-0.15f);
    // maybe apply density to vector field too?
    //off /= 1.0f+disp.x*10.0f;
    float2 uv2 = fragCoord / iResolution;
    // get "emitter" from buffer A
    fragColor += _tex2DVecN(iChannel0,uv2.x,uv2.y,15);
    // mutate previous state with field direction
    fragColor += texture(iChannel1, (0.5f+0.5f*(1.0f*(-1.0f+2.0f*uv2)))+off*0.025f);
    
    // disperse and output
    //fragColor = _powf( 0.97f*clamp(fragColor-disp, 0.0f, 111.0f), to_float4(1.002f*(1.0f+disp)));
    fragColor = pow_f4( clamp(fragColor-disp, 0.0f, 1.0f), (1.002f*(1.0f+disp)));


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------


__KERNEL__ void PseudofluidfadeJipi012Fuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution)
{

  fragCoord+=0.5f;

  float2 uv = fragCoord / swi2(iResolution,y,y);
  fragColor  = _mix(to_float4_s(0), to_float4(1,0,0,1), _powf( (0.5f+0.5f*_sinf(uv.y*120.0f)),99.0f));
  fragColor *= _mix(to_float4_s(0), to_float4(1,0,0,1), _powf( (0.5f+0.5f*_sinf(uv.x*120.0f)),99.0f));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void PseudofluidfadeJipi012Fuse__Buffer_D(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    fragCoord+=0.5f;

    float2 uv = -1.0f+2.0f*fragCoord / iResolution;
    uv.x *= iResolution.x/iResolution.y;
    uv *= 1.25f;

    // noise seed
    float3 v = to_float3_aw(uv, iTime*0.3f);
    const float disp_freq = 0.6f;
    //swi2(v,x,y) *= disp_freq;
    v.x *= disp_freq;
    v.y *= disp_freq;
    
    
    // get first some density variation
    float4 disp = 0.5f*_mix(to_float4_s(0), pow_f4(0.5f+0.5f*to_float4_s(fbm3(-2.0f*v+11.2f)), to_float4_s(2)), 0.05f);
    //swi2(v,x,y) /= disp_freq;
    v.x /= disp_freq;
    v.y /= disp_freq;
    
    // add to randomization coordinates
    v += disp.x*24.0f;
    
    // vector field ("fluid" direction)
    float2 off =  0.25f*swi2(curlNoise((v)),x,y);// to_float2(fbm3(v), fbm3(v+99.0f));
    off.y = _fminf(0.0f, off.y-0.15f);
    // maybe apply density to vector field too?
    //off /= 1.0f+disp.x*10.0f;
    float2 uv2 = fragCoord / iResolution;
    // mutate previous state with field direction
    fragColor = texture(iChannel0, (0.5f+0.5f*(1.0f*(-1.0f+2.0f*uv2)))+off*0.025f);
    
  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1


//#define POINTS
__KERNEL__ void PseudofluidfadeJipi012Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX2(Grid, 0);
    
    //CONNECT_CHECKBOX1(Textur, 0);
    CONNECT_SLIDER2(Textur, -10.0f, 10.0f, 0.0f);
    
    
    fragCoord+=0.5f; 

    float2 uv = fragCoord/iResolution;
    fragColor = to_float4_s(1.0f)-(_tex2DVecN(iChannel0,uv.x,uv.y,15));
    float4 temp = fragColor;
    fragColor = smoothstep(to_float4_s(0.0f), to_float4_s(1.1f), pow_f4(fragColor, to_float4_s(0.4545f)));
    
    if (Textur > 0.0f)
    {
      float4 tex = _tex2DVecN(iChannel2,uv.x,uv.y,15);
      if(tex.w>0.0f)
        fragColor = _mix(fragColor, tex, Textur);
    }
    
    if(Grid)
      swi3S(fragColor,x,y,z, _mix(swi3(fragColor,x,y,z), to_float3(1,0,0), _tex2DVecN(iChannel1,uv.x,uv.y,15).w));
    
    

  SetFragmentShaderComputedColor(fragColor);
}
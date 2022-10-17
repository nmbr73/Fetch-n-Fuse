
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ float sdSphere(float3 p, float r )
{
  return length(p) - r;
}

__DEVICE__ float hash1( float2 p )
{
    p  = 50.0f*fract_f2( p*0.3183099f );
    return fract( p.x*p.y*(p.x+p.y) );
}

__DEVICE__ float hash1( float n )
{
    return fract( n*17.0f*fract( n*0.3183099f ) );
}

__DEVICE__ float noise( in float3 _x )
{
    float3 p = _floor(_x);
    float3 w = fract_f3(_x);
    
    #if 1
    float3 u = w*w*w*(w*(w*6.0f-15.0f)+10.0f);
    #else
    float3 u = w*w*(3.0f-2.0f*w);
    #endif
    


    float n = 111.0f*p.x + 317.0f*p.y + 157.0f*p.z;
    
    float a = hash1(n+(  0.0f+  0.0f+  0.0f));
    float b = hash1(n+(111.0f+  0.0f+  0.0f));
    float c = hash1(n+(  0.0f+317.0f+  0.0f));
    float d = hash1(n+(111.0f+317.0f+  0.0f));
    float e = hash1(n+(  0.0f+  0.0f+157.0f));
    float f = hash1(n+(111.0f+  0.0f+157.0f));
    float g = hash1(n+(  0.0f+317.0f+157.0f));
    float h = hash1(n+(111.0f+317.0f+157.0f));

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return -1.0f+2.0f*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z);
}

__DEVICE__ float fbm_4( in float3 _x )
{
  const mat3 m3  = to_mat3( 0.00f,  0.80f,  0.60f,
                      -0.80f,  0.36f, -0.48f,
                      -0.60f, -0.48f,  0.64f );
  
    float f = 2.0f;
    float s = 0.5f;
    float a = 0.0f;
    float b = 0.5f;
    for( int i=0; i<4; i++ )
    {
        float n = noise(_x);
        a += b*n;
        b *= s;
        _x = f*mul_mat3_f3(m3,_x);
    }
  return a;
}
__DEVICE__ float fbm_2( in float3 _x )
{
  const mat3 m3  = to_mat3( 0.00f,  0.80f,  0.60f,
                      -0.80f,  0.36f, -0.48f,
                      -0.60f, -0.48f,  0.64f );
  
    float f = 2.0f;
    float s = 0.5f;
    float a = 0.0f;
    float b = 0.5f;
    for( int i=0; i<2; i++ )
    {
        float n = noise(_x);
        a += b*n;
        b *= s;
        _x = f*mul_mat3_f3(m3,_x);
    }
  return a;
}

__DEVICE__ float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f );
    return _mix( d2, -d1, h ) + k*h*(1.0f-h); }
__DEVICE__ float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); 
}
    
__DEVICE__ float map(float3 p,float iTime){


  float s;

  float s1 =sdSphere(p+to_float3(1.0f,0.1f,0.0f), 1.7f*(_sinf(iTime+p.x)*0.005f+1.0f));
  float s2 =sdSphere(p+to_float3(-1.0f,0.0f,0.1f), 1.8f*(_sinf(iTime+p.x+1.0f)*0.005f+1.0f));
  s = opSmoothUnion(s1,s2,0.5f);
  
  float3 noiseP =p*0.7f;

  float n =smoothstep(-0.2f,1.0f,fbm_4(noiseP*2.0f+fbm_4(noiseP*2.0f)*1.5f))*0.1f;
  s-=n;
  
  float skin = smoothstep(0.5f,1.0f,1.0f-n*5.0f);
  s+=skin*smoothstep(-1.0f,1.0f,fbm_2(noiseP*50.0f)*fbm_2(noiseP*4.0f))*0.02f;

  return s;

}
__DEVICE__ float4 getColor(float3 p){

  float3 noiseP =p*0.7f;
  float n1=_fabs(fbm_2(noiseP*1.0f));

  float n =smoothstep(-0.2f,1.0f,fbm_4(noiseP*2.0f+fbm_4(noiseP*2.0f)*1.5f));
  float3 base1 = _mix(to_float3(0.2f,0,0.1f),to_float3(0.9f,0.2f,0.3f),(n));
  float3 lum = to_float3(0.299f, 0.587f, 0.114f);
  float3 gray = to_float3_s(dot(lum, base1));
  float4 color =to_float4(0,0,0,0);
  swi3S(color,x,y,z, _mix(base1, gray, (_powf(n1,2.0f))));
  color.w =40.0f;
  float s = smoothstep(0.2f,0.4f,n);
  color.w -=s*20.0f;
  swi3S(color,x,y,z, swi3(color,x,y,z)+to_float3_s(s)*to_float3(0.7f,0.7f,0.4f)*0.5f);
  return color;
}

__DEVICE__ float3 calcNormal(float3 p, float iTime) {
    float2 e = to_float2(1.0f, -1.0f) * 0.0005f; // epsilon
    float r = 1.0f; // radius of sphere
    return normalize(
      swi3(e,x,y,y) * map(p + swi3(e,x,y,y),iTime) +
      swi3(e,y,y,x) * map(p + swi3(e,y,y,x),iTime) +
      swi3(e,y,x,y) * map(p + swi3(e,y,x,y),iTime) +
      swi3(e,x,x,x) * map(p + swi3(e,x,x,x),iTime));
}



__DEVICE__ float rayMarch(float3 ro, float3 rd, float start, float end, float iTime) {
  float depth = start;

  for (int i = 0; i < 256; i++) {
    float3 p = ro + depth * rd;
    
    float d =map(p,iTime);
    
    depth += d;
    if (d < 0.001f || depth > end) break;
  }

  return depth;
}
__DEVICE__ float calcSoftshadow( in float3 ro, in float3 rd, in float mint, in float tmax, float iTime )
{
    float res = 1.0f;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    
    for( int i=0; i<16; i++ )
    {
      float h = map( ro + rd*t, iTime );
      
      // use this if you are getting artifact on the first iteration, or unroll the
      // first iteration out of the loop
      //float y = (i==0) ? 0.0f : h*h/(2.0f*ph); 

      float y = h*h/(2.0f*ph);
      float d = _sqrtf(h*h-y*y);
      res = _fminf( res, 10.0f*d/_fmaxf(0.0f,t-y) );
      ph = h;
      
      t += h;
      
      if( res<0.0001f || t>tmax ) break;
        
    }
    res = clamp( res, 0.0f, 1.0f );
    return res*res*(3.0f-2.0f*res);
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr )
{
  float3 cw = normalize(ta-ro);
  float3 cp = to_float3(_sinf(cr), _cosf(cr),0.0f);
  float3 cu = normalize( cross(cw,cp) );
  float3 cv =          ( cross(cu,cw) );
  return to_mat3_f3( cu, cv, cw );
}

__KERNEL__ void MeatballsFuse(float4 fragColor, float2 fragCoord, float iTime, float4 iMouse, float2 iResolution)
{


  float time =_sinf( iTime*0.2f)*0.2f+1.3f;
  // camera  
  float3 ta = to_float3( 0.0f, 0.0f, 0.0f );

  ta.x = 5.0f - iMouse.x/iResolution.x*10.0f;
  ta.y = 5.0f - iMouse.y/iResolution.y*10.0f;
  
  float3 ro = ta + to_float3( 10.0f*_cosf(time ), 0, 10.0f*_sinf(time ) );
  // camera-to-world transformation
  mat3 ca = setCamera( ro, ta, 0.0f );
  float2 p = (2.0f*fragCoord-iResolution)/iResolution.y;

  // focal length
  const float fl = 3.5f;
        
  // ray direction
  float3 rd = mul_mat3_f3(ca , normalize( to_float3_aw(p,fl) ));

  float3 col = to_float3_s(0);


  float d = rayMarch(ro, rd, 0.0f, 100.0f,iTime); 

  if (d > 100.0f) 
  {
    col = to_float3(0.2f,0.2f,0.4f)*0.5f*(1.0f-_powf(length(p)*0.5f,2.0f)); // ray didn't hit anything
  } 
  else 
  {
    float3 p = ro + rd * d; // point on sphere we discovered from ray marching
    float3 N = calcNormal(p,iTime);
    float4 colin = getColor(p);
    float3 albedo = swi3(colin,x,y,z);
    float3 lightpos =to_float3(2.0f,3.0f,3.0f);
    float3 L = normalize(lightpos - p);
    
    float shadow = calcSoftshadow(p,L, 0.01f, 3.0f, iTime);
    float3 irr =to_float3_s(_fmaxf(0.0f,dot(N,L))*2.0f)*shadow+to_float3(0.1f,0.1f,0.2f);
    col =irr*albedo;
    
    float3  ref = reflect(rd,N);            
    float fre = clamp(1.0f+dot(N,rd),0.0f,1.0f);
    float spe = (colin.w/15.0f)*_powf( clamp(dot(ref,L),0.0f, 1.0f), colin.w )*2.0f*(0.5f+0.5f*_powf(fre,42.0f));
    col += spe*shadow;
   
    col +=to_float3_s(_powf(1.0f+dot(rd,N),2.0f))*to_float3(0.2f,0.1f,0.1f);

  }

  // Output to screen
  fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
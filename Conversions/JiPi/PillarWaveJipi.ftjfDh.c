
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery Blurred_0' to iChannel0


// Pillar Wave
//
// SDF from IQ
// AA Option from FN

#define PI   3.14159256f
#define TAU  2.0f*3.14159256f

// Rotation matrix around the X axis.
// https://www.shadertoy.com/view/fdjGRD
__DEVICE__ mat3 rotateX(float theta) {
    float c = _cosf(theta);
    float s = _sinf(theta);
    return to_mat3_f3(
                     to_float3(1, 0, 0),
                     to_float3(0, c, -s),
                     to_float3(0, s, c)
                     );
}

struct ray{
 float3 direction;
 float3 origin;
};

// Rounded Box SDF Function
__DEVICE__ float sdBox( float3 p, float3 boxDim, float3 boxLoc, mat3 transform ){
  p = mul_f3_mat3((p - boxLoc) , transform);
  float3 q = abs_f3(p) - boxDim;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f) - 0.035f;
}

__DEVICE__ float3 calcNormalBox(in float3 p, float3 dimVal, float3 loc, mat3 transform){
  float2 e = to_float2(1.0f, -1.0f) * 0.0005f; 
  return normalize(
                  swi3(e,x,y,y) * sdBox(p + swi3(e,x,y,y), dimVal, loc, transform) +
                  swi3(e,y,y,x) * sdBox(p + swi3(e,y,y,x), dimVal, loc, transform) +
                  swi3(e,y,x,y) * sdBox(p + swi3(e,y,x,y), dimVal, loc, transform) +
                  swi3(e,x,x,x) * sdBox(p + swi3(e,x,x,x), dimVal, loc, transform));
}

// easings.net
__DEVICE__ float easeInOutQuint(float x){
  return x < 0.5f ? 16.0f * x * x * x * x * x : 1.0f - _powf(-2.0f * x + 2.0f, 5.0f) / 2.0f;
}

// Sphere Tracing
__DEVICE__ bool sphereTrace(float3 ro, float3 rd, out float3 *p, out float3 *pN, float iTime){
  float mint = -5.0f;  // Minimum trace distance
  float maxt = 20.0f;   // Maximum trace distance
   
  float dist = mint;
  while(dist < maxt){
    float3 p = ro + rd*dist;       
    // *p = ro + rd*dist;       

    float3 boxDimension = to_float3(0.04f,0.5f,0.5f);
    float3 boxLocation;
        
    float minD = 9999.0f;
    float i;
       
    for(i=0.0f; i<17.0f; i++){
      boxLocation = to_float3(-1.44f+0.18f*i,0.0f,-2.0f);
      float currD = sdBox(p,boxDimension, boxLocation, rotateX(PI*easeInOutQuint(fract(iTime/4.0f)-(i*0.015f))));
      minD = _fminf(currD, minD);
      if (minD < 0.001f) break;
    }
        
    *pN = calcNormalBox(p, boxDimension, boxLocation, rotateX(PI*easeInOutQuint(fract(iTime/4.0f)-(i*0.015f))));
    dist = dist + minD;
        
    if (minD < 0.001f) return true;
        
  }
  return false;
}

__DEVICE__ void mainImage0( out float4 *fragColor, in float2 fragCoord, float2 iResolution, float iTime, __TEXTURE2D__ iChannel0 ){
  
  float2 uv = ( fragCoord - 0.5f* iResolution ) /iResolution.y;
   
  // Background Horizon 1
  // float3 col = to_float3(smoothstep(0.0f,1.0f,_powf(19.0f,-_fabs(uv.y)-_fabs(uv.x)*0.4f)));
  
  //Background Horizon 2
  float3 col = to_float3_s(_powf(1.0f-(_fabs(uv.y)),4.0f));
   
  // Create ray at eye location, through each point in the "screen"
  ray r;
  r.origin = to_float3(0.0f,0.0f,4.0f); 
  r.direction = normalize(to_float3_aw(uv,1.0f) - r.origin);

  float3 p=to_float3_s(0.0f), pN;
  if(sphereTrace(r.origin, r.direction, &p, &pN, iTime)){
      
      float3 rr = reflect(r.direction, pN); 
      float3 reflecter = swi3(decube_f3(iChannel0,rr),x,y,z);
     
      col = reflecter*0.3f;
     
      // Diffuse light    
      float3 light = to_float3(0.0f,0.0f,0.3f);
      // float3 light = to_float3_aw(0.5f+0.4f*_sinf(iTime),_cosf(iTime),1.0f); 
      float dif = clamp(dot(pN, normalize(light-p)),0.0f,1.0f);
      dif *= 3.0f/dot(light - p, light - p);
      col *= to_float3_s(_powf(dif, 0.4545f));
    }
     
   *fragColor = to_float4_aw(col,1.0f);
}


__DEVICE__ float _fwidth(float inp, float2 iR, float Par){
    //simulate fwidth
    float uvx = inp + Par/iR.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp + Par/iR.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}



// smart AA, from FabriceNeyret (FN).
__KERNEL__ void PillarWaveJipiFuse(float4 O, float2 U, float iTime, float2 iResolution, sampler2D iChannel0)
{
  
    CONNECT_SLIDER0(Par, -10.0f, 10.0f, 1.0f);

    mainImage0(&O,U, iResolution,iTime,iChannel0);
    bool AA = true;//false;  // AA option
    if(AA == true)
    if ( _fwidth(length(O), iResolution, Par) > 0.01f ) {  // difference threshold between neighbor pixels
        float4 o;
        for (int k=0; k < 9; k+= k==3?2:1 )
          { mainImage0(&o,U+to_float2(k%3-1,k/3-1)/3.0f, iResolution,iTime,iChannel0); O += o; }
        O /= 9.0f;
    //  O.x++;                        // uncomment to see where the oversampling occurs
    }

//O = _tex2DVecN(iChannel0,U.x/iResolution.x,U.y/iResolution.y,15);

  SetFragmentShaderComputedColor(O);
}
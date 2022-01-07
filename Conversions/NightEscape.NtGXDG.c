
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------




#define ei(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))
__DEVICE__ float hash(float2 p)
{
  
    float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}
__DEVICE__ float noise(float2 p)
{
    float4 w = to_float4(floor(p.x),floor(p.y), ceil(p.x),ceil(p.y) );
    float 
        _00 = hash(swi2(w,x,y)),
        _01 = hash(swi2(w,x,w)),
        _10 = hash(swi2(w,z,y)),
        _11 = hash(swi2(w,z,w)),
         _0 = _mix(_00,_01,fract(p.y)),
         _1 = _mix(_10,_11,fract(p.y));
    return _mix(_0,_1,fract(p.x));
}
__DEVICE__ float fbmt (float2 p,float T) {
    float w = 0.0f;
    #define N 11.0f
    for (float i = 0.0f; i < N; i+=1.0f)
    {
        p = mul_f2_mat2(p*1.7f,ei(1e-3*T));
        w += noise(p)/N;
    }
    return w;
}
__DEVICE__ float fbm (float2 p) {
  
    float w = 0.0f;
    #define N 11.0f
    for (float i = 0.0f; i < N; i+=1.0f)
    {
        p = mul_f2_mat2(p*1.7f,ei(0.5f));
        w += noise(p)/N;
    }
    return w;
}
__DEVICE__ float4 pw (float4 p, float a) {
    return to_float4(_powf(p.x,a),_powf(p.y,a),_powf(p.z,a),_powf(p.w,a));
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__KERNEL__ void NightEscapeFuse(float4 Q, float2 U, float2 iResolution, float iTime, int iFrame)
 {
   
  CONNECT_SLIDER0(alpha,0.0f,1.0f,1.0f); 
  CONNECT_SLIDER1(Verzerrung1,-5.0f,5.0f,1.9f); 
  CONNECT_SLIDER2(Wolken,-5.0f,5.0f,3.0f); 
   
  float2 R=iResolution; float T=iTime; int I=iFrame; 

   
  U = 2.0f*(U-0.5f*R)/R.y;
  //float d = 1.0f+1.9f*(U.y);
  float d = 1.0f+Verzerrung1*(U.y);
  //float2 u = U*3.0f/d;
  float2 u = U*Wolken/d;
  if (d<0.0f) {
     u=-u;
     u.x += 0.01f*_sinf(-T+1.0f*u.x)*_sinf(3.0f*u.y);
     u.y += 0.05f*_sinf(1.0f*u.x)*_sinf(1.0f*u.y);
     U.x += 0.05f*_sinf(-T+5.0f*u.x)*_sinf(60.0f*u.y);
     U.y += 0.05f*_sinf(1.0f*u.x)*_sinf(1.0f*u.y);
  }
 
  d = _fabs(d);
  u.x += _sinf(u.y);
  float cloud = clamp(1.0f-0.01f*(_powf(5.0f*fbmt(10.0f+u+0.01f*T,T),3.0f)),0.0f,1.0f);
  float night = 0.8f*clamp(0.5f-0.05f*u.x,0.0f,1.0f);
  float stars = 0.5f+0.5f*clamp(2e6*_powf(fbm(29.0f*u),15.0f),0.0f,1.0f);
  float4 sunset = 0.2f+0.5f*sin_f4(-0.9f-0.1f*u.y+2e-2*u.x+to_float4(1,2,3,4));
  sunset = _mix(sunset,to_float4_s(2.0f)-2.0f*to_float4_s(stars),night);
  Q = _mix(to_float4_s(_expf(-0.01f*u.x*u.x)),sunset,cloud);
  float mountain = 17.0f*_powf(fbm(to_float2(10.0f+0.05f*U.x,0)),7.0f);
  Q *= to_float4_s((1.0f)-step(d-mountain,0.0f));
  
  Q.w=alpha;
  
  SetFragmentShaderComputedColor(Q);
}
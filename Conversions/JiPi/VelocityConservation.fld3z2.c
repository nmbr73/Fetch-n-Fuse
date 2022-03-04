
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define A(u) texture(iChannel0,(u)/iResolution)
#define B(u) texture(iChannel1,(u)/iResolution)
#define C(u) texture(iChannel2,(u)/iResolution)

__KERNEL__ void VelocityConservationFuse__Buffer_A(float4 fragColor, float2 u, float2 iResolution, sampler2D iChannel0)
{
    u+=0.5f;

    float2  a = to_float2_s(0);
    float h = A(u).z;
    float z    = 8.0f;//kernel convolution size
    float blur = 4.0f/z;
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
        float2 c = to_float2(i,j)*blur; //c = swi2(c,y,x)*to_float2(-1,1);
        float h2 = A(u+to_float2(i,j)).z;
        a += c*(h2-h)*_expf(-dot(c,c));
    }}
    fragColor = swi4(a,x,y,x,y);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Texture: Organic 3' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel0



__KERNEL__ void VelocityConservationFuse__Buffer_B(float4 fragColor, float2 u, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    u+=0.5f; 
  
    float4 t = A(u);
    float2 m =  swi2(t,x,y)
               +swi2(B(u),x,y)*(t.z-0.4f)
               +t.z*to_float2(0,0.0f)
               -C(u).x*swi2(t,x,y)*0.0f;
    float s = 0.0f;
    float z    = 8.0f;//kernel convolution size
    float blur = 4.0f/z;
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
      float2 c = (m+to_float2(i,j))*blur;
      s += _expf(-dot(c,c));
    }}
    if(s==0.0f){s = 1.0f;}
    s = 1.0f/s;
    
    fragColor = to_float4(m.x,m.y,s,0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void VelocityConservationFuse__Buffer_C(float4 fragColor, float2 u, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    u+=0.5f;

    float lz = 0.0f;
    float tz = 0.0f;
    float4 a = to_float4_s(0);
    float z    = 8.0f;//kernel convolution size
    float blur = 4.0f/z;
    for(float i=-z; i<=z; ++i){
    for(float j=-z; j<=z; ++j){
      float4 t = A(u+to_float2(i,j));
      float4 m = B(u+to_float2(i,j));
      float2 c = (swi2(m,x,y)-to_float2(i,j))*blur;
      float z = t.z*_expf(-dot(c,c));
      lz   += z*length(swi2(m,x,y));
      //swi2(a,x,y) += z*swi2(m,x,y);
      a.x += z*m.x;
      a.y += z*m.y;
      
      a.z  += z*m.z;
      tz   += z;
    }}
    if(tz==0.0f){tz = 1.0f;}
    float l = 1.0f/length(swi2(a,x,y));  if(isinf(l)){l = 0.0f;}
    //swi2(a,x,y) *= l*lz/tz;
    a.x *= l*lz/tz;
    a.y *= l*lz/tz;
    
    if(iMouse.z>0.0f)
    {
        float2 m = 16.0f*(u-swi2(iMouse,x,y))/iResolution.y;
        a += to_float4(0,0,1,1)*0.1f*_expf(-dot(m,m));
    }
    if(iFrame==0)
    {
        float2 m = 7.0f*(u-iResolution*0.5f)/iResolution.y;
        a = to_float4(0,0,1,1)*_expf(-dot(m,m));
    }
    fragColor = a;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0


__KERNEL__ void VelocityConservationFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f; 

    float2 u = fragCoord/iResolution;
    fragColor = to_float4_s(_tex2DVecN(iChannel0,u.x,u.y,15).z);

  SetFragmentShaderComputedColor(fragColor);
}
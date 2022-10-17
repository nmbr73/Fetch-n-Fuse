
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
 
#define iterations 14
#define formuparam2 0.79
 
#define volsteps 5
#define stepsize 0.290

#ifdef ORG 
  #define zoom 0.900
  #define tile   0.850
  #define speed2  0.001
   
  #define brightness 0.003
  #define darkmatter 0.300
  #define distfading 0.560
  #define saturation 0.900


  #define transverseSpeed zoom*0.01
  #define cloud 0.02f 
#endif
 
__DEVICE__ float triangle(float x, float a) { 
  float output2 = 2.0f*_fabs(  2.0f*  ( (x/a) - _floor( (x/a) + 0.5f) ) ) - 1.0f;
  return output2;
}
 
__DEVICE__ float field(in float3 p, float iTime) {  
  float strength = 7.0f + 0.03f * _logf(1.e-6 + fract(_sinf(iTime) * 4373.11f));
  float accum = 0.0f;
  float prev = 0.0f;
  float tw = 0.0f;  

  for (int i = 0; i < 6; ++i) {
    float mag = dot(p, p);
    p = abs_f3(p) / mag + to_float3(-0.5f, -0.8f + 0.1f*_sinf(iTime*0.7f + 2.0f), -1.1f+0.3f*_cosf(iTime*0.3f));
    float w = _expf(-(float)(i) / 7.0f);
    accum += w * _expf(-strength * _powf(_fabs(mag - prev), 2.3f));
    tw += w;
    prev = mag;
  }
  return _fmaxf(0.0f, 5.0f * accum / tw - 0.7f);
}

__KERNEL__ void SpaceyBoiJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float4 iMouse)
{
   
  CONNECT_SLIDER0(Seethe, -1000.0f, 1000.0f, 0.0f); 
  
  
  CONNECT_SLIDER1(zoom, -1.0f, 5.0f, 0.900f);
  CONNECT_SLIDER2(tile, -1.0f, 5.0f, 0.850f);
  CONNECT_SLIDER3(speed2, -1.0f, 1.0f, 0.001f);
   
  CONNECT_SLIDER4(brightness, -1.0f, 1.0f, 0.003f);
  CONNECT_SLIDER5(darkmatter, -1.0f, 5.0f, 0.300f);
  CONNECT_SLIDER6(distfading, -1.0f, 5.0f, 0.560f);
  CONNECT_SLIDER7(saturation, -1.0f, 5.0f, 0.900f);


  CONNECT_SLIDER8(TransverseSpeed, -1.0f, 1.0f, 0.01f);
  CONNECT_SLIDER9(cloud, -1.0f, 1.0f, 0.02f); 
  
  CONNECT_SLIDER10(Depth, -1.0f, 30.0f, 12.0f); 
  
  float transverseSpeed = zoom*TransverseSpeed;
  
   
  float time = iTime+Seethe; 

  float2 mouse = swi2(iMouse,x,y)/iResolution-0.5f;  

  float2 uv2 = 2.0f * fragCoord / to_float2_s(512) - 1.0f;
  float2 uvs = uv2 * to_float2_s(512)  / 512.0f;
  
  float time2 = iTime;               
  float speed = speed2;
  //speed = 0.0001f * _cosf(time2*0.00002f + 3.1415926f/8.0f);          
  //speed = 0.0f;  
  float formuparam = formuparam2;
  
  //get coords and direction  
  float2 uv = uvs;           
  //mouse rotation
  float a_xz = 0.9f;
  float a_yz = -0.6f;
  float a_xy = 0.9f + iTime*0.08f;  
  
  mat2 rot_xz = to_mat2(_cosf(a_xz),_sinf(a_xz),-_sinf(a_xz),_cosf(a_xz));  
  mat2 rot_yz = to_mat2(_cosf(a_yz),_sinf(a_yz),-_sinf(a_yz),_cosf(a_yz));    
  mat2 rot_xy = to_mat2(_cosf(a_xy),_sinf(a_xy),-_sinf(a_xy),_cosf(a_xy));
  
  float v2 =1.0f;  
  float3 dir=to_float3_aw(uv*zoom+mouse,1.0f); 
  float3 from=to_float3(0.0f, 0.0f,0.0f);                               
  from.x -= 5.0f*(1.0f-1.0f);
  from.y -= 5.0f*(1.0f-0.7f);
         
               
  float3 forward = to_float3(0.0f,0.0f,1.0f);   
  from.x += transverseSpeed*(1.0f)*_cosf(0.01f*iTime) + 0.001f*iTime;
  from.y += transverseSpeed*(1.0f)*_sinf(0.01f*iTime) +0.001f*iTime;
  from.z += 0.003f*time;  
  
  swi2S(dir,x,y, mul_f2_mat2(swi2(dir,x,y),rot_xy));
  swi2S(forward,x,y, mul_f2_mat2(swi2(forward,x,y) , rot_xy));
  swi2S(dir,x,z, mul_f2_mat2(swi2(dir,x,z) , rot_xz));
  swi2S(forward,x,z, mul_f2_mat2(swi2(forward,x,z) , rot_xz));  
  swi2S(dir,y,z, mul_f2_mat2(swi2(dir,y,z) , rot_yz));
  swi2S(forward,y,z, mul_f2_mat2(swi2(forward,y,z) , rot_yz));
  
  swi2S(from,x,y, mul_f2_mat2(swi2(from,x,y) , mul_f_mat2(-1.0f,rot_xy)));
  swi2S(from,x,z, mul_f2_mat2(swi2(from,x,z) , rot_xz));
  swi2S(from,y,z, mul_f2_mat2(swi2(from,y,z) , rot_yz));
   
  
  //zoom
  float zooom = (time2-3311.0f)*speed;
  from += forward* zooom;
  float sampleShift = mod_f( zooom, stepsize );
   
  float zoffset = -sampleShift;
  sampleShift /= stepsize; // make from 0 to 1
  
  //volumetric rendering
  float s=0.24f;
  float s3 = s + stepsize/2.0f;
  float3 v=to_float3_s(0.0f);
  float t3 = 0.0f;  
  
  float3 backCol2 = to_float3_s(0.0f);
  for (int r=0; r<volsteps; r++) {
    float3 p2=from+(s+zoffset)*dir;// + to_float3(0.0f,0.0f,zoffset);
    float3 p3=from+(s3+zoffset)*dir;// + to_float3(0.0f,0.0f,zoffset);
    
    p2 = abs_f3(to_float3_s(tile)-mod_f3f3(p2,to_float3_s(tile*2.0f))); // tiling fold
    p3 = abs_f3(to_float3_s(tile)-mod_f3f3(p3,to_float3_s(tile*2.0f))); // tiling fold    
    
    if( cloud > 0.0f)
      t3 = field(p3, iTime);
    
    
    float pa,a=pa=0.0f;
    for (int i=0; i<iterations; i++) {
      p2 = abs_f3(p2)/dot(p2,p2)-formuparam; // the magic formula
      //p=_fabs(p)/_fmaxf(dot(p,p),0.005f)-formuparam; // another interesting way to reduce noise
      float D = _fabs(length(p2)-pa); // absolute sum of average change
      a += i > 7 ? _fminf( Depth, D) : D; // Size 12.0f
      pa = length(p2);
    }
    
    
    float dm = _fmaxf(0.0f, darkmatter-a*a*0.001f); //dark matter
    
    a *= a*a; // add contrast
    
    // brightens stuff up a bit
    float s1 = s+zoffset;
    // need closed form expression for this, now that we shift samples
    float fade = _powf(distfading,_fmaxf(0.0f,(float)(r)-sampleShift));
    
    if (r>3) fade *= 1.0f-dm; // dark matter, don't render near
float zzzzzzzzzzzzzz;    
    //t3 += fade;    
    v += fade;
    //backCol2 -= fade;

    // fade out samples as they approach the camera
    if( r == 0 )
      fade *= (1.0f - (sampleShift));
    // fade in samples as they approach from the distance
    if( r == volsteps-1 )
      fade *= sampleShift;
    v+=to_float3(s1,s1*s1,s1*s1*s1*s1)*a*brightness*fade; // coloring based on distance
    
    backCol2 += _mix(0.4f, 1.0f, v2) * to_float3(1.8f * t3 * t3 * t3, 1.4f * t3 * t3, t3) * fade;

    
    s+=stepsize;
    s3 += stepsize;    
  }//фор
           
  v = _mix(to_float3_s(length(v)),v,saturation); //color adjust  

  float4 forCol2 = to_float4_aw(v*0.01f,1.0f);  
  
  if( cloud > 0.0f)
    backCol2 *= cloud;
  
  backCol2.z *= -3.8f;
  backCol2.x *= 0.05f;  
  
  backCol2.z = 1.5f*_mix(backCol2.y, backCol2.z, 0.1f);
  backCol2.y = -0.0f;
  swi2S(backCol2,z,y,  _mix(swi2(backCol2,y,z), swi2(backCol2,z,y), 0.39f*(_cosf(1.00f) + 1.0f)));
  fragColor = forCol2 + to_float4_aw(backCol2, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
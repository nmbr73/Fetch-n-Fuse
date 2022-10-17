
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//from https://www.shadertoy.com/view/4sKyRK
__DEVICE__ float distanceToBottleCurve(float2 point) {
    return point.y-0.1f*_sinf(point.x*2.5f + 0.6f) + 0.05f*_sinf(5.0f*point.x) + 0.04f*_sinf(7.5f*point.x);
}
__DEVICE__ bool texturee(float2 uv) {
    float ang = _atan2f(uv.y, uv.x);
    float len = _floor(length(uv)*10.0f);
    bool val = len == 2.0f || len == 6.0f || len == 9.0f;
    if (len == 3.0f || len == 4.0f || len == 5.0f || len == 8.0f || len == 10.0f) {
        val = distanceToBottleCurve(to_float2(ang+len,0.0f))*7.99f > _cosf(len*7.99f);
    }
    return val;
}

__KERNEL__ void EggstructionFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution)
{
    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution-0.5f;
    //I would make a better revision logo but I am too tired :(
    fragColor = to_float4(1.0f,91.0f,188.0f,1.0f)/255.0f;
    float rad1 = _fabs(_floor(_atan2f(uv.x,uv.y)/2.0f)*0.04f);
    if (texturee(uv*2.5f)) {
        fragColor = to_float4(255.0f,214.0f,0.0f,1.0f)/255.0f;
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: EggTex' to iChannel1


#define ro(r) to_mat2(_cosf(r),_sinf(r),-_sinf(r),_cosf(r))

__DEVICE__ float box(float3 p, float3 d) {
  p = abs_f3(p)-d;
  return length(_fmaxf(p,to_float3_s(0.0f)))+_fminf(0.0f,_fmaxf(p.x,_fmaxf(p.y,p.z)));
}

__DEVICE__ float beam(float3 p) {
  swi2S(p,x,y, swi2(p,x,y) - _fmaxf(0.0f,dot(normalize(to_float2_s(1.0f)),swi2(p,x,y)))*2.0f*normalize(to_float2_s(1)));
  swi2S(p,x,y, swi2(p,x,y) - _fmaxf(0.0f,dot(normalize(to_float2(1,-1)),swi2(p,x,y)))*2.0f*normalize(to_float2(1,-1)));
  p.x += 0.2f;
  float3 p2 = p;
  p2.y = _fabs(p2.y)-0.2f;
  p.z = asin(_sinf(p.z*10.0f))/10.0f;
  swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , ro(radians(45.0f))));
  return _fminf(length(swi2(p2,x,y)),length(swi2(p,x,y)))-0.02f;
}

__DEVICE__ float zip(float k, float iTime) {
  k += iTime;
  float id = _floor(k)+0.5f;
  k -= id;
  k = smoothstep(-0.1f,0.1f,k)+smoothstep(-0.2f,0.2f,k)-smoothstep(-0.3f,0.3f,k);
  k+=id;
  return k;
}

struct Egg{
  float3 pos;
  float off1;
  float mul;
  float off2;
  float2 stretch;
};


__DEVICE__ float scene(float3 p, float iTime, inout float *o1,inout float *o2,inout float *o3,inout float3 *oop, Egg egg, float timeshift) {
  
  float zzzzzzzzzzzzzzzzzzzz;
  swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , ro(asin(_sinf(zip(0.4f,iTime)/2.0f))*0.7f)));
  *o1 = beam(p);
  *o2 = beam(swi3(p,x,z,y));
  *o3 = box(p,to_float3_s(0.2f))-0.1f;
  *o3 = _fminf(*o3,box(p-to_float3(0,1,0),to_float3(0.2f,0.2f,0.4f))-0.1f);
  *o3 = _fminf(*o3,box(p-to_float3(0,-5,0),to_float3(0.2f,0.05f,0.2f))-0.1f);
  *o3 = _fminf(*o3,box(p-to_float3(0,0,0.6f),to_float3(0.2f,0.2f,0.05f))-0.1f);
  *o2 = _fmaxf(*o2,p.y-0.79f);
  *o2 = _fmaxf(*o2,-p.y-5.0f);
  *o1 = _fmaxf(*o1,p.z-0.48f);
  p.z+=0.4f;
  float op = p.z;
  p.y += 2.5f + asin(_sinf(zip(timeshift,iTime))); //2.5 AuslegerSchlitten
  *o3  = _fminf(*o3, box(p,to_float3(0.2f,0.3f,0.1f))-0.05f);
  
  float3 p2 = p;
  p2.y=_fabs(p.y)-0.1f;
  p.z+=0.4f + _fmaxf(0.0f,asin(_sinf(iTime)))*8.0f;
  *o3 = _fminf(*o3, box(p,to_float3(0.2f,0.3f,0.1f))-0.05f);
  *o3 = _fminf(*o3,_fmaxf(_fmaxf(op,-p.z),length(swi2(p2,x,y))-0.02f));
  p.z+=0.4f;
  swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y) , ro(iTime/3.0f)));
  *oop = p;
  p.z*=0.8f;
  //float e = length(p)-0.25f+smoothstep(-0.5f,0.5f,p.z)*0.4f-0.2f;
  float e = length(p-egg.pos)-egg.off1+smoothstep(-0.5f+egg.stretch.x,0.5f+egg.stretch.y,p.z)*egg.mul-egg.off2;
  if (asin(_sinf(iTime/2.0f - 3.14159f/4.0f)) < 0.0f) e = 1e5;
  return _fminf(_fminf(*o3,e),_fminf(*o1,*o2));
}

 __DEVICE__ inline mat3 mat3_sub_mat3( mat3 A, mat3 B) {  
  mat3 C;  

  C.r0 = to_float3(A.r0.x - B.r0.x, A.r0.y - B.r0.y,A.r0.z - B.r0.z);  
  C.r1 = to_float3(A.r1.x - B.r1.x, A.r1.y - B.r1.y,A.r1.z - B.r1.z); 
  C.r2 = to_float3(A.r2.x - B.r2.x, A.r2.y - B.r2.y,A.r2.z - B.r2.z);

  return C;  
  }

__DEVICE__ float3 norm(float3 p, float iTime, inout float *o1,inout float *o2,inout float *o3,inout float3 *oop, Egg egg, float timeshift) {
  mat3 k = mat3_sub_mat3(to_mat3_f3(p,p,p) , to_mat3(0.001f,0.0f,0.0f, 0.0f,0.001f,0.0f, 0.0f,0.0f,0.001f ));
  return normalize(scene(p,iTime,o1,o2,o3,oop,egg,timeshift)
       -to_float3( scene(k.r0,iTime,o1,o2,o3,oop,egg,timeshift),
                   scene(k.r1,iTime,o1,o2,o3,oop,egg,timeshift),
                   scene(k.r2,iTime,o1,o2,o3,oop,egg,timeshift) ));
}


__KERNEL__ void EggstructionFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(EggTex, 0);

  CONNECT_COLOR0(Color1, 0.7f, 0.2f, 0.1f, 1.0f);
  CONNECT_COLOR1(Color2, 0.05f, 0.05f, 0.05f, 1.0f);
  CONNECT_COLOR2(Color3, 0.7f, 0.6f, 0.05f, 1.0f);
  CONNECT_COLOR3(Color4, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);

  CONNECT_POINT0(EggPos, 0.0f, 0.0f );
  CONNECT_SLIDER1(EggPosZ, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER2(EggOff1, -10.0f, 10.0f, 0.25f);
  CONNECT_SLIDER3(EggOff2, -10.0f, 10.0f, 0.2f);
  CONNECT_SLIDER4(EggMul, -10.0f, 10.0f, 0.4f);
  CONNECT_POINT1(Stretch, 0.0f, 0.0f );
  
  CONNECT_POINT1(TexXY, 0.0f, 0.0f );
  CONNECT_SLIDER5(TexZ, -10.0f, 10.0f, 0.0f);
  CONNECT_SLIDER6(TexScale, -10.0f, 10.0f, 0.0f);
  
  CONNECT_SLIDER7(Brightness1, -10.0f, 12.0f, 7.0f);
  CONNECT_SLIDER8(Brightness2, -10.0f, 10.0f, 2.0f);
  
  CONNECT_SLIDER9(timeshift, -50.0f, 50.0f, 0.0f);
  
  CONNECT_SLIDER10(Dist, -10.0f, 10.0f, 1.0f);
  

  CONNECT_BUTTON0(TexModus, 0, XY,  XZ, YZ, NN1, NN2);

  Egg egg = { to_float3_aw(EggPos,EggPosZ), EggOff1, EggOff2, EggMul, Stretch };

  mat2 dummy1;
  mat3 dummy2;

  float o1 = 0.0f, o2 = 0.0f, o3 = 0.0f;
  float3 oop;

  // Normalized pixel coordinates (from 0 to 1)
  float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
  
  float ratio = iResolution.y/iResolution.x;

  float3 cam_mPos = to_float3_s(0.0f);

  if (iMouse.z > 0.0f)
  {    
    float fAngle = (iMouse.x / iResolution.x - 0.0f) * radians(360.0f);
    float fElevation = (iMouse.y / iResolution.y - 0.5f) * radians(90.0f);
    float fDist = Dist;//1.0f; 
    //cam_mPos = to_float3(_sinf(fAngle) * fDist * _cosf(fElevation), _sinf(fElevation) * fDist, _cosf(fAngle) * fDist * _cosf(fElevation));
    cam_mPos = to_float3_aw(swi2(iMouse,x,y)/iResolution,Dist);
  }
  

  float3 cam = normalize(to_float3(1.0f+_sinf(zip(0.3f,iTime)/2.0f)*0.5f,uv.x,uv.y)+cam_mPos);
  float v = _sinf(zip(0.7f,iTime))*0.3f;
  float3 init = to_float3(-6,-2.0f+v,-1.0f+_cosf(zip(2.0f,iTime)*2.0f)*0.25f);
  
  swi2S(cam,x,z, mul_f2_mat2(swi2(cam,x,z) , ro(0.3f)));
  swi2S(init,x,z, mul_f2_mat2(swi2(init,x,z) ,ro(0.3f)));
  
  float3 p = init;
  bool hit = false;
  float dist;
  bool trig = false;
  for(int i = 0; i<50&&!hit;i++){ 
    dist = scene(p,iTime,&o1,&o2,&o3,&oop,egg,timeshift);
    p+=cam*dist;
    hit = dist*dist<1e-6;
    if(!trig)trig=dist<0.03f;
    if(dist>1e4)break;
  }
  float3 lp = oop;
  bool io1 = dist==o1;
  bool io2 = dist==o2;
  bool io3 = dist==o3;
  float3 n = norm(p,iTime,&o1,&o2,&o3,&oop,egg,timeshift);
  float3 r = reflect(cam,n);
  float fact = length(sin_f3(r*3.0f)*0.5f+0.5f)/_sqrtf(3.0f);
  //float3 matcol = swi3(texture(iChannel0,clamp(swi2(lp,y,z)*1.8f+to_float2(0,0.3f)+0.5f,0.0f,1.0f)),x,y,z); // Das Ei swi3(Color4,x,y,z);//
  float4 matcol = texture(iChannel0,clamp(swi2(lp,y,z)*1.8f+to_float2(0,0.3f)+0.5f,0.0f,1.0f)); // Das Ei swi3(Color4,x,y,z);//
  
  if(EggTex)
  {
    float2 tuv = 1.0f-to_float2(lp.x*ratio, lp.y) * TexScale+TexXY;
    
    if(TexModus == 2)
      tuv = to_float2(lp.x*ratio, lp.z) * TexScale+TexXY - 0.5f; // -0.5
    
    if(TexModus == 3)
      tuv = to_float2(lp.y*ratio, lp.z) * TexScale+TexXY - 0.5f;
    
    matcol = texture(iChannel1, tuv);
    
    //lp.y *= ratio;
    //matcol = texture(iChannel1, 1.0f-swi2(lp,y,z)*TexScale+TexXY+0.5f);
  }
  
  if(io1) matcol = Color1;//to_float3(0.7f,0.2f,0.1f);
  if(io3) matcol = Color2;//to_float3_s(0.05f);
  if(io2) matcol = Color3;//to_float3(0.7f,0.6f,0.05f);
  float3 col = swi3(matcol,x,y,z) + _powf(fact,Brightness1)*Brightness2;
  
  fragColor = to_float4_aw( hit ? sqrt_f3(col):to_float3_s(trig?0.0f:uv.y*0.5f+0.5f), matcol.w);
  
  SetFragmentShaderComputedColor(fragColor);
}

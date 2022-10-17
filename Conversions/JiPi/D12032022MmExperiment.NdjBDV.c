
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Material Maker Experiment 12-03-2022
// By PauloFalcao
//
// Made in material maker
//
// MaterialMaker is a node based shader maker
// where nodes have functions as inputs
// so it's possible to do raymarching, fractals,
// and more pretty things
//
// Material Maker
//   https://rodzilla.itch.io/material-maker
//
// My Raymarching library for Material Maker
//   https://github.com/paulofalcao/MaterialMakerRayMarching
// 


#define SEED_VARIATION 0.0f

__DEVICE__ float wave3d_square(float x) {
  return (fract(x) < 0.5f) ? 0.0f : 1.0f;
}

__DEVICE__ float mix3d_xor(float x, float y, float z) {
  float xy = _fminf(x+y, 2.0f-x-y);
  return _fminf(xy+z, 2.0f-xy-z);
}

__DEVICE__ float3 MFSDF_Obj_Maker_rotate3d(float3 p, float3 a) {
  float3 rv;
  float c;
  float s;
  c = _cosf(a.x);
  s = _sinf(a.x);
  rv.x = p.x;
  rv.y = p.y*c+p.z*s;
  rv.z = -p.y*s+p.z*c;
  c = _cosf(a.y);
  s = _sinf(a.y);
  p.x = rv.x*c+rv.z*s;
  p.y = rv.y;
  p.z = -rv.x*s+rv.z*c;
  c = _cosf(a.z);
  s = _sinf(a.z);
  rv.x = p.x*c+p.y*s;
  rv.y = -p.x*s+p.y*c;
  rv.z = p.z;
  return rv;
}

__DEVICE__ float mfsdf3d_smooth_union_f(float a,float b,float k){
  float h = _fmaxf( k-_fabs(a-b), 0.0f )/k;
  return _fminf(a,b)-h*h*k*0.25f;
}

__DEVICE__ float4 mfsdf3d_smooth_union(float4 a, float4 b, float k) {
  float e=0.001f;
  k=_fmaxf(k,e);
  float h=mfsdf3d_smooth_union_f(a.w,b.w,k);
  float2 n=normalize(to_float2(mfsdf3d_smooth_union_f(a.w+e,b.w,k)-mfsdf3d_smooth_union_f(a.w-e,b.w,k),
                               mfsdf3d_smooth_union_f(a.w,b.w+e,k)-mfsdf3d_smooth_union_f(a.w,b.w-e,k)));
  return to_float4_aw(_mix(swi3(a,x,y,z),swi3(b,x,y,z),_atan2f(_fabs(n.y),_fabs(n.x))/(3.14159265359f/2.0f)),h);
}

__DEVICE__ float mfsdf3d_smooth_subtraction_f(float a,float b,float k){
  float h = _fmaxf( k-_fabs(-a-b), 0.0f )/k;
  return _fmaxf(-a,b)+h*h*k*0.25f;
}

__DEVICE__ float4 mfsdf3d_smooth_subtraction(float4 a, float4 b, float k) {
  float e=0.001f;
  k=_fmaxf(k,e);
  float h=mfsdf3d_smooth_subtraction_f(a.w,b.w,k);
  float2 n=normalize(to_float2(mfsdf3d_smooth_subtraction_f(a.w+e,b.w,k)-mfsdf3d_smooth_subtraction_f(a.w-e,b.w,k),
                               mfsdf3d_smooth_subtraction_f(a.w,b.w+e,k)-mfsdf3d_smooth_subtraction_f(a.w,b.w-e,k)));
    return to_float4_aw(_mix(swi3(a,x,y,z),swi3(b,x,y,z),_atan2f(_fabs(n.y),_fabs(n.x))/(3.14159265359f/2.0f)),h);
}

__DEVICE__ float mfsdf3d_smooth_intersection_f(float a,float b,float k){
  float h = _fmaxf( k-_fabs(a-b), 0.0f )/k;
  return _fmaxf(a,b)+h*h*k*0.25f;
}

__DEVICE__ float4 mfsdf3d_smooth_intersection(float4 a, float4 b, float k) {
  float e=0.001f;
  k=_fmaxf(k,e);
  float h=mfsdf3d_smooth_intersection_f(a.w,b.w,k);
  float2 n=normalize(to_float2(mfsdf3d_smooth_intersection_f(a.w+e,b.w,k)-mfsdf3d_smooth_intersection_f(a.w-e,b.w,k),
                               mfsdf3d_smooth_intersection_f(a.w,b.w+e,k)-mfsdf3d_smooth_intersection_f(a.w,b.w-e,k)));
    return to_float4_aw(_mix(swi3(a,x,y,z),swi3(b,x,y,z),_atan2f(_fabs(n.y),_fabs(n.x))/(3.14159265359f/2.0f)),h);
}

// https://www.shadertoy.com/view/XsX3zB by Nikita Miropolskiy
// MIT License

// discontinuous pseudorandom constly distributed in [-0.5f, +0.5]^3 */
__DEVICE__ float3 XsX3zB_oct_random3(float3 c) {
  float j = 4096.0f*_sinf(dot(c,to_float3(17.0f, 59.4f, 15.0f)));
  float3 r;
  r.z = fract(512.0f*j);
  j *= 0.125f;
  r.x = fract(512.0f*j);
  j *= 0.125f;
  r.y = fract(512.0f*j);
  return r-0.5f;
}



// 3d simplex noise
__DEVICE__ float XsX3zB_oct_simplex3d(float3 p) {
   // skew constants for 3d simplex functions
   const float XsX3zB_oct_F3 =  0.3333333f;
   const float XsX3zB_oct_G3 =  0.1666667f;
   
   // 1.0f find current tetrahedron T and it's four vertices
   // s, s+i1, s+i2, s+1.0f - absolute skewed (integer) coordinates of T vertices
   // x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices
   
   // calculate s and x
   float3 s = _floor(p + dot(p, to_float3_s(XsX3zB_oct_F3)));
   float3 x = p - s + dot(s, to_float3_s(XsX3zB_oct_G3));
   
   // calculate i1 and i2
   float3 e = step(to_float3_s(0.0f), x - swi3(x,y,z,x));
   float3 i1 = e*(1.0f - swi3(e,z,x,y));
   float3 i2 = 1.0f - swi3(e,z,x,y)*(1.0f - e);
     
   // x1, x2, x3
   float3 x1 = x - i1 + XsX3zB_oct_G3;
   float3 x2 = x - i2 + 2.0f*XsX3zB_oct_G3;
   float3 x3 = x - 1.0f + 3.0f*XsX3zB_oct_G3;
   
   // 2.0f find four surflets and store them in d
   float4 w, d;
   
   // calculate surflet weights
   w.x = dot(x, x);
   w.y = dot(x1, x1);
   w.z = dot(x2, x2);
   w.w = dot(x3, x3);
   
   // w fades from 0.6f at the center of the surflet to 0.0f at the margin
   w = _fmaxf(0.6f - w, 0.0f);
   
   // calculate surflet components
   d.x = dot(XsX3zB_oct_random3(s), x);
   d.y = dot(XsX3zB_oct_random3(s + i1), x1);
   d.z = dot(XsX3zB_oct_random3(s + i2), x2);
   d.w = dot(XsX3zB_oct_random3(s + 1.0f), x3);
   
   // multiply d by w^4
   w *= w;
   w *= w;
   d *= w;
   
   // 3.0f return the sum of the four surflets
   return dot(d, to_float4_s(52.0f));
}

__DEVICE__ float3 v4v4_rotate(float3 p, float3 a) {
  float3 rv;
  float c;
  float s;
  c = _cosf(a.x);
  s = _sinf(a.x);
  rv.x = p.x;
  rv.y = p.y*c+p.z*s;
  rv.z = -p.y*s+p.z*c;
  c = _cosf(a.y);
  s = _sinf(a.y);
  p.x = rv.x*c+rv.z*s;
  p.y = rv.y;
  p.z = -rv.x*s+rv.z*c;
  c = _cosf(a.z);
  s = _sinf(a.z);
  rv.x = p.x*c+p.y*s;
  rv.y = -p.x*s+p.y*c;
  rv.z = p.z;
  return rv;
}

#define PI 3.14159265359f

__DEVICE__ float2 equirectangularMap(float3 dir) {
  float2 longlat = to_float2(_atan2f(dir.y,dir.x),_acosf(dir.z));
  return longlat/to_float2(2.0f*PI,PI);
}


//Simple HDRI START

//Hash without Sine Dave_Hoskins
//https://www.shadertoy.com/view/4djSRW 
__DEVICE__ float Simple360HDR_hash12(float2 p)
{
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float Simple360HDR_noise(float2 v){
  float2 v1=_floor(v);
  float2 v2=smoothstep(0.0f,1.0f,fract_f2(v));
  float n00=Simple360HDR_hash12(v1);
  float n01=Simple360HDR_hash12(v1+to_float2(0,1));
  float n10=Simple360HDR_hash12(v1+to_float2(1,0));
  float n11=Simple360HDR_hash12(v1+to_float2(1,1));
  return _mix(mix(n00,n01,v2.y),_mix(n10,n11,v2.y),v2.x);
}

__DEVICE__ float Simple360HDR_noiseOct(float2 p){
  return
    Simple360HDR_noise(p)*0.5f+
    Simple360HDR_noise(p*2.0f+13.0f)*0.25f+
    Simple360HDR_noise(p*4.0f+23.0f)*0.15f+
    Simple360HDR_noise(p*8.0f+33.0f)*0.10f+
    Simple360HDR_noise(p*16.0f+43.0f)*0.05f;
}

__DEVICE__ float3 Simple360HDR_skyColor(float3 p){
  float3 s1=to_float3(0.2f,0.5f,1.0f);
  float3 s2=to_float3(0.1f,0.2f,0.4f)*1.5f;
  float3 v=(Simple360HDR_noiseOct(swi2(p,x,z)*0.1f)-0.5f)*to_float3_s(1.0f);
  float d=length(p);
  return _mix(s2+v,s1+v*(12.0f/_fmaxf(d,20.0f)),clamp(d*0.1f,0.0f,1.0f));
}

__DEVICE__ float3 Simple360HDR_floorColor(float3 p){
  float3 v=(Simple360HDR_noiseOct(swi2(p,x,z)*0.1f)*0.5f+0.25f)*to_float3(0.7f,0.5f,0.4f);
  return v;
}

__DEVICE__ float3 Simple360HDR_renderHDR360(float3 rd, float3 sun){
  float3 col;
  float3 p;
  float3 c;
  if (rd.y>0.0f) {
        p=rd*(5.0f/rd.y);
        c=Simple360HDR_skyColor(p);
    } else {
        p=rd*(-10.0f/rd.y);
        c=Simple360HDR_floorColor(p);
    c=_mix(c,to_float3(0.5f,0.7f,1.0f),clamp(1.0f-_sqrtf(-rd.y)*3.0f,0.0f,1.0f));
  }
  float3 skycolor=to_float3(0.1f,0.45f,0.68f);
  float d=length(p);
  
  float ds=clamp(dot(sun,rd),0.0f,1.0f);
  float3 sunc=(ds>0.9997?to_float3_s(2.0f):to_float3_s(0.0f))+_powf(ds,512.0f)*4.0f+_powf(ds,128.0f)*to_float3_s(0.5f)+_powf(ds,4.0f)*to_float3_s(0.5f);
    if (rd.y>0.0f){
    c+=to_float3_s(0.3f)*_powf(1.0f-_fabs(rd.y),3.0f)*0.7f;
  } 
    return c+sunc;
}

__DEVICE__ float3 Simple360HDR_make360hdri(float2 p, float3 sun){
    float xPI=3.14159265359f;
    float2 thetaphi = ((p * 2.0f) - to_float2_s(1.0f)) * to_float2(xPI,xPI/2.0f); 
    float3 rayDirection = to_float3_aw(_cosf(thetaphi.y) * _cosf(thetaphi.x), _sinf(thetaphi.y), _cosf(thetaphi.y) * _sinf(thetaphi.x));
    return Simple360HDR_renderHDR360(rayDirection,sun);
}

//Simple HDRI END
const float p_o8705_CamX = 0.872000000f;
const float p_o8705_CamY = 1.094000000f;
const float p_o8705_CamZ = 3.000000000f;
const float p_o8705_LookAtX = 0.000000000f;
const float p_o8705_LookAtY = 0.000000000f;
const float p_o8705_LookAtZ = 0.000000000f;
const float p_o8705_CamD = 1.500000000f;
const float p_o8705_CamZoom = 2.347000000f;
const float p_o8705_SunX = 2.500000000f;
const float p_o8705_SunY = 2.500000000f;
const float p_o8705_SunZ = 2.500000000f;
const float p_o8705_AmbLight = 0.250000000f;
const float p_o8705_AmbOcclusion = 1.000000000f;
const float p_o8705_Shadow = 1.000000000f;
const float p_o8705_Gamma = 1.300000000f;
const float p_o8718_xyz = 0.451000000f;
const float p_o8718_x = 1.000000000f;
const float p_o8718_y = 1.000000000f;
const float p_o8718_z = 1.000000000f;
const float p_o8738_k = 0.446000000f;
const float p_o8725_k = 1.000000000f;
const float p_o8714_BaseColor_r = 1.000000000f;
const float p_o8714_BaseColor_g = 0.656250000f;
const float p_o8714_BaseColor_b = 0.000000000f;
const float p_o8714_BaseColor_a = 1.000000000f;
const float p_o8714_Specular = 0.961000000f;
const float p_o8714_Roughness = 0.000000000f;


__DEVICE__ float o8762_fct(float3 uv, float _seed_variation_) {
  return mix3d_xor(wave3d_square(uv.x), wave3d_square(uv.y), wave3d_square(uv.z));
}

__DEVICE__ float3 o8714_input_BaseColor_tex3d(float4 p, float _seed_variation_) {
    float3 o8762_0_1_tex3d = to_float3_aw(o8762_fct((p).xyz, _seed_variation_));
    return o8762_0_1_tex3d;
}

__DEVICE__ float o8714_input_Metallic_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}

__DEVICE__ float o8714_input_Specular_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}

__DEVICE__ float o8714_input_Roughness_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}

__DEVICE__ float3 o8714_input_Emission_tex3d(float4 p, float _seed_variation_) {
    return to_float3(1.0f,1.0f,1.0f);
}

__DEVICE__ float3 o8714_input_Normal_tex3d(float4 p, float _seed_variation_) {
    return to_float3(0.0f,1.0f,0.0f);
}

__DEVICE__ float o8714_input_Alpha_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}
__DEVICE__ float o8714_input_AmbientOcclusion_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}

const float p_o8719_d = 3.140000000f;
const float p_o8713_r = 0.610000000f;

__DEVICE__ float o8719_input_in(float2 uv, float _seed_variation_) {
    float o8713_0_1_sdf2d = length((uv)-to_float2((_cosf(_seed_variation_*6.0f)*0.4f)+0.5f, (_sinf(_seed_variation_*5.0f)*0.4f)+0.5f))-p_o8713_r;
    return o8713_0_1_sdf2d;
}

__DEVICE__ float o8714_input_sdf3d(float3 p, float _seed_variation_) {
    float2 o8719_0_q = to_float2(length((p).xy)-p_o8719_d+0.5f, (p).z+0.5f);
    float o8719_0_1_sdf3d = o8719_input_in(o8719_0_q,_atan2f((p).x,(p).y));
    return o8719_0_1_sdf3d;
}


__DEVICE__ float4 PBRObjectMaker_o8714(float4 uv, float _seed_variation_) {
  float sdf=o8714_input_sdf3d(swi3(uv,x,y,z), _seed_variation_);
  //5 - Roughness
  if (uv.w>4.5f) {
    return to_float4(p_o8714_Roughness*o8714_input_Roughness_tex3d(to_float4(swi3(uv,x,y,z),5.0f), _seed_variation_),0.0f,0.0f,sdf);
  } else
  //4 - Specular
  if (uv.w>3.5f) {
    return to_float4(p_o8714_Specular*o8714_input_Specular_tex3d(to_float4(swi3(uv,x,y,z),4.0f), _seed_variation_),0.0f,0.0f,sdf);
  } else
  //1 - BaseColor
  if (uv.w>0.5f){
    return to_float4(to_float4(p_o8714_BaseColor_r, p_o8714_BaseColor_g, p_o8714_BaseColor_b, p_o8714_BaseColor_a).rgb*o8714_input_BaseColor_tex3d(to_float4(swi3(uv,x,y,z),1.0f), _seed_variation_),sdf);
  } else
  //0 - SDF
  {
    return to_float4_aw(to_float3(0),sdf);
  }
}

const float p_o8726_BaseColor_r = 0.000000000f;
const float p_o8726_BaseColor_g = 0.424825996f;
const float p_o8726_BaseColor_b = 1.000000000f;
const float p_o8726_BaseColor_a = 1.000000000f;
const float p_o8726_Specular = 0.969000000f;
const float p_o8726_Roughness = 0.000000000f;


__DEVICE__ float3 o8726_input_BaseColor_tex3d(float4 p, float _seed_variation_) {
    return to_float3_s(1.0f);
}

__DEVICE__ float o8726_input_Specular_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}

__DEVICE__ float o8726_input_Roughness_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}

const float p_o8737_d = 4.730000000f;
const float p_o8730_r = 0.500000000f;

__DEVICE__ float o8737_input_in(float2 uv, float _seed_variation_) {
    float o8730_0_1_sdf2d = length((uv)-to_float2((_cosf(_seed_variation_*9.0f)*0.6f)+0.5f, (_sinf(_seed_variation_*7.0f)*0.4f)+0.5f))-p_o8730_r;
    return o8730_0_1_sdf2d;
}

__DEVICE__ float o8726_input_sdf3d(float3 p, float _seed_variation_) {
    float2 o8737_0_q = to_float2(length((p).xy)-p_o8737_d+0.5f, (p).z+0.5f);
    float o8737_0_1_sdf3d = o8737_input_in(o8737_0_q,_atan2f((p).x,(p).y));
    return o8737_0_1_sdf3d;
}

__DEVICE__ float4 PBRObjectMaker_o8726(float4 uv, float _seed_variation_) {
   // uv.xyz=MFSDF_Obj_Maker_rotate3d(swi3(uv,x,y,z)-to_float3(p_o8726_TranlateX,p_o8726_TranlateY,p_o8726_TranlateZ),to_float3(p_o8726_RotateX,p_o8726_RotateY,p_o8726_RotateZ)*6.28318530718f)/p_o8726_scale;
  float sdf=o8726_input_sdf3d(swi3(uv,x,y,z), _seed_variation_);
  //5 - Roughness
  if (uv.w>4.5f) {
    return to_float4(p_o8726_Roughness*o8726_input_Roughness_tex3d(to_float4(swi3(uv,x,y,z),5.0f), _seed_variation_),0.0f,0.0f,sdf);
  } else
  //4 - Specular
  if (uv.w>3.5f) {
    return to_float4(p_o8726_Specular*o8726_input_Specular_tex3d(to_float4(swi3(uv,x,y,z),4.0f), _seed_variation_),0.0f,0.0f,sdf);
  } else
  //1 - BaseColor
  if (uv.w>0.5f){
    return to_float4(to_float4(p_o8726_BaseColor_r, p_o8726_BaseColor_g, p_o8726_BaseColor_b, p_o8726_BaseColor_a).rgb*o8726_input_BaseColor_tex3d(to_float4(swi3(uv,x,y,z),1.0f), _seed_variation_),sdf);
  } else
  //0 - SDF
  {
    return to_float4_aw(to_float3(0),sdf);
  }
}

const float p_o8733_BaseColor_r = 1.000000000f;
const float p_o8733_BaseColor_g = 1.000000000f;
const float p_o8733_BaseColor_b = 1.000000000f;
const float p_o8733_BaseColor_a = 1.000000000f;
const float p_o8733_Specular = 1.000000000f;
const float p_o8733_Roughness = 0.000000000f;
const float p_o8733_scale = 1.000000000f;
const float p_o8733_TranlateX = 0.000000000f;
const float p_o8733_TranlateY = 0.000000000f;
const float p_o8733_TranlateZ = 0.000000000f;
const float p_o8733_RotateX = 0.000000000f;
const float p_o8733_RotateY = 0.000000000f;
const float p_o8733_RotateZ = 0.000000000f;
const float p_o8749_g_0_pos = 0.409091000f;
const float p_o8749_g_0_r = 1.000000000f;
const float p_o8749_g_0_g = 0.000000000f;
const float p_o8749_g_0_b = 0.000000000f;
const float p_o8749_g_0_a = 1.000000000f;
const float p_o8749_g_1_pos = 0.518182000f;
const float p_o8749_g_1_r = 1.000000000f;
const float p_o8749_g_1_g = 1.000000000f;
const float p_o8749_g_1_b = 1.000000000f;
const float p_o8749_g_1_a = 1.000000000f;

__DEVICE__ float4 o8749_g_gradient_fct(float x) {
  if (x < p_o8749_g_0_pos) {
    return to_float4(p_o8749_g_0_r,p_o8749_g_0_g,p_o8749_g_0_b,p_o8749_g_0_a);
  } else if (x < p_o8749_g_1_pos) {
    return _mix(to_float4(p_o8749_g_0_r,p_o8749_g_0_g,p_o8749_g_0_b,p_o8749_g_0_a), to_float4(p_o8749_g_1_r,p_o8749_g_1_g,p_o8749_g_1_b,p_o8749_g_1_a), ((x-p_o8749_g_0_pos)/(p_o8749_g_1_pos-p_o8749_g_0_pos)));
  }
  return to_float4(p_o8749_g_1_r,p_o8749_g_1_g,p_o8749_g_1_b,p_o8749_g_1_a);
}

const float p_o8744_scale = 3.768000000f;
const float p_o8744_scale_x = 1.000000000f;
const float p_o8744_scale_y = 1.000000000f;
const float p_o8744_scale_z = 1.000000000f;
const float p_o8744_transx = 0.000000000f;
const float p_o8744_transy = 0.000000000f;
const float p_o8744_transz = 0.000000000f;
const float p_o8744_persistence = 0.500000000f;
const float p_o8744_brightness = 0.000000000f;
const float p_o8744_contrast = 2.255000000f;

__DEVICE__ float o8744_fbm(float3 coord, float persistence, float _seed_variation_) {
  float normalize_factor = 0.0f;
  float value = 0.0f;
  float scale = 1.0f;
  float size = 1.0f;
  for (int i = 0; i < 2; i++) {
    value += XsX3zB_oct_simplex3d(coord*size) * scale;
    normalize_factor += scale;
    size *= 2.0f;
    scale *= persistence;
  }
  return value / normalize_factor;
}

__DEVICE__ float o8744_bc(float f,float contrast, float brightness, float _seed_variation_) {
  return f*contrast+brightness+0.5f-contrast*0.5f;
}

__DEVICE__ float3 o8733_input_BaseColor_tex3d(float4 p, float _seed_variation_) {
    float3 o8744_0_out = to_float3(o8744_bc(o8744_fbm((p).xyz*to_float3(p_o8744_scale_x,p_o8744_scale_y,p_o8744_scale_z)*0.5f*p_o8744_scale+to_float3(p_o8744_transx,p_o8744_transy,p_o8744_transz),p_o8744_persistence, _seed_variation_)*0.5f+0.5f,p_o8744_contrast,p_o8744_brightness, _seed_variation_));float3 o8744_0_1_tex3d = o8744_0_out;
    float3 o8749_0_1_tex3d = o8749_g_gradient_fct(dot(o8744_0_1_tex3d, to_float3_s(1.0f))/3.0f).rgb;
    return o8749_0_1_tex3d;
}

__DEVICE__ float o8733_input_Specular_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}

__DEVICE__ float o8733_input_Roughness_tex3d(float4 p, float _seed_variation_) {
    return 1.0f;
}

const float p_o8739_Distort = 0.107000000f;
const float p_o8739_Correction = 0.000000000f;
const float p_o8739_Bound = 9.411000000f;
const float p_o8732_d = 4.320000000f;
const float p_o8731_r = 0.610000000f;

__DEVICE__ float o8732_input_in(float2 uv, float _seed_variation_) {
    float o8731_0_1_sdf2d = length((uv)-to_float2((_cosf(_seed_variation_*4.0f)*0.6f)+0.5f, (_sinf(_seed_variation_*4.0f)*0.4f)+0.5f))-p_o8731_r;
    return o8731_0_1_sdf2d;
}

__DEVICE__ float o8739_input_sdf(float3 p, float _seed_variation_) {
    float2 o8732_0_q = to_float2(length((p).xy)-p_o8732_d+0.5f, (p).z+0.5f);
    float o8732_0_1_sdf3d = o8732_input_in(o8732_0_q,_atan2f((p).x,(p).y));
    return o8732_0_1_sdf3d;
}

__DEVICE__ float3 o8739_input_tex3d(float4 p, float _seed_variation_) {
    float3 o8744_0_out = to_float3(o8744_bc(o8744_fbm((p).xyz*to_float3(p_o8744_scale_x,p_o8744_scale_y,p_o8744_scale_z)*0.5f*p_o8744_scale+to_float3(p_o8744_transx,p_o8744_transy,p_o8744_transz),p_o8744_persistence, _seed_variation_)*0.5f+0.5f,p_o8744_contrast,p_o8744_brightness, _seed_variation_));float3 o8744_0_1_tex3d = o8744_0_out;
    return o8744_0_1_tex3d;
}

//tetrahedron normal by PauloFalcao
//https://www.shadertoy.com/view/XstGDS
__DEVICE__ float3 normal_o8739(float3 p, float _seed_variation_) {  
  const float3 e=to_float3(0.001f,-0.001f,0.0f);
  float v1=o8739_input_sdf(p+swi3(e,x,y,y), _seed_variation_);
  float v2=o8739_input_sdf(p+swi3(e,y,y,x), _seed_variation_);
  float v3=o8739_input_sdf(p+swi3(e,y,x,y), _seed_variation_);
  float v4=o8739_input_sdf(p+swi3(e,x,x,x), _seed_variation_);
  return normalize(to_float3(v4+v1-v3-v2,v3+v4-v1-v2,v2+v4-v3-v1));
}

__DEVICE__ float distortByNormal_o8739(float3 uv, float _seed_variation_) {
    float d=o8739_input_sdf(uv, _seed_variation_);
  if (d<=_fabs(p_o8739_Distort*(p_o8739_Bound+1.0f))+0.01f){
    float3 n=normal_o8739(uv, _seed_variation_);
    float3 s=o8739_input_tex3d(to_float4_aw(uv,0.0f), _seed_variation_);
    return o8739_input_sdf(uv-(n*s*p_o8739_Distort), _seed_variation_);
  } else return d;
}

__DEVICE__ float o8733_input_sdf3d(float3 p, float _seed_variation_) {
    float o8739_0_1_sdf3d = distortByNormal_o8739((p), _seed_variation_)/(1.0f+p_o8739_Distort*p_o8739_Correction);
    return o8739_0_1_sdf3d;
}


__DEVICE__ float4 PBRObjectMaker_o8733(float4 uv, float _seed_variation_) {
    uv.xyz=MFSDF_Obj_Maker_rotate3d(swi3(uv,x,y,z)-to_float3(p_o8733_TranlateX,p_o8733_TranlateY,p_o8733_TranlateZ),to_float3(p_o8733_RotateX,p_o8733_RotateY,p_o8733_RotateZ)*6.28318530718f)/p_o8733_scale;
  float sdf=o8733_input_sdf3d(swi3(uv,x,y,z), _seed_variation_)*p_o8733_scale;
  //5 - Roughness
  if (uv.w>4.5f) {
    return to_float4(p_o8733_Roughness*o8733_input_Roughness_tex3d(to_float4(swi3(uv,x,y,z),5.0f), _seed_variation_),0.0f,0.0f,sdf);
  } else
  //4 - Specular
  if (uv.w>3.5f) {
    return to_float4(p_o8733_Specular*o8733_input_Specular_tex3d(to_float4(swi3(uv,x,y,z),4.0f), _seed_variation_),0.0f,0.0f,sdf);
  } else
  //1 - BaseColor
  if (uv.w>0.5f){
    return to_float4(to_float4(p_o8733_BaseColor_r, p_o8733_BaseColor_g, p_o8733_BaseColor_b, p_o8733_BaseColor_a).rgb*o8733_input_BaseColor_tex3d(to_float4(swi3(uv,x,y,z),1.0f), _seed_variation_),sdf);
  } else
  //0 - SDF
  {
    return to_float4_aw(to_float3(0),sdf);
  }
}

__DEVICE__ float4 o8705_input_mfsdf(float4 p, float _seed_variation_) {

    float3 r=v4v4_rotate(swi3(p,x,y,z), -to_float3(iTime*31.0f, iTime*33.0f, iTime*35.0f)*0.01745329251f);
    if (iMouse.z>0.5f){
        r=to_float3(swi3(p,x,y,z));
    }
    float4 rot=to_float4_aw(to_float4(r, p.w).xyz/to_float3(p_o8718_x, p_o8718_y, p_o8718_z)/p_o8718_xyz,to_float4_aw(r,p.w).w);

    float4 o8714_0_1_v4v4 = PBRObjectMaker_o8714(rot, _seed_variation_);
    float4 o8726_0_1_v4v4 = PBRObjectMaker_o8726(rot, _seed_variation_);
    float4 o8725_0_1_v4v4 = mfsdf3d_smooth_union(o8714_0_1_v4v4, o8726_0_1_v4v4,p_o8725_k);
    float4 o8733_0_1_v4v4 = PBRObjectMaker_o8733(rot, _seed_variation_);
    float4 o8738_0_1_v4v4 = mfsdf3d_smooth_union(o8725_0_1_v4v4, o8733_0_1_v4v4,p_o8738_k);
    float4 o_o8718_0=o8738_0_1_v4v4;float4 o8718_0_1_v4v4 = to_float4(swi3(o_o8718_0,x,y,z),o_o8718_0.w*_fminf(min(p_o8718_x, p_o8718_y), p_o8718_z)*p_o8718_xyz);
    float4 o8720_0_1_v4v4 = o8718_0_1_v4v4;

    return o8720_0_1_v4v4;
}
__DEVICE__ float3 o8705_input_hdri(float2 uv, float _seed_variation_) {

return Simple360HDR_make360hdri(to_float2((uv).x,-(uv).y+1.0f),normalize(to_float3(-p_o8705_SunX,p_o8705_SunY,-p_o8705_SunZ)));
}

//tetrahedron normal by PauloFalcao
//https://www.shadertoy.com/view/XstGDS
__DEVICE__ float3 normal_o8705(float3 p, float _seed_variation_) {  
  const float3 e=to_float3(0.001f,-0.001f,0.0f);
  float v1=o8705_input_mfsdf(to_float4(p+swi3(e,x,y,y),0.0f), _seed_variation_).w;
  float v2=o8705_input_mfsdf(to_float4(p+swi3(e,y,y,x),0.0f), _seed_variation_).w;
  float v3=o8705_input_mfsdf(to_float4(p+swi3(e,y,x,y),0.0f), _seed_variation_).w;
  float v4=o8705_input_mfsdf(to_float4(p+swi3(e,x,x,x),0.0f), _seed_variation_).w;
  return normalize(to_float3(v4+v1-v3-v2,v3+v4-v1-v2,v2+v4-v3-v1));
}

__DEVICE__ void march_o8705(inout float d,inout float3 p,float dS, float3 ro, float3 rd, float _seed_variation_) {
    for (int i=0; i < 500; i++) {
      p = ro + rd*d;
        dS = o8705_input_mfsdf(to_float4_aw(p,0.0f), _seed_variation_).w;
        d += dS;
        if (d > 50.0f || _fabs(dS) < 0.0001f) break;
    }
}

//from https://www.shadertoy.com/view/lsKcDD
__DEVICE__ float calcAO_o8705( in float3 pos, in float3 nor , float _seed_variation_) {
  float occ = 0.0f;
    float sca = 1.0f;
    for( int i=0; i<5; i++ ){
        float h = 0.001f + 0.25f*float(i)/4.0f;
        float d = o8705_input_mfsdf(to_float4_aw( pos + h*nor ,0.0f), _seed_variation_).w;
        occ += (h-d)*sca;
        sca *= 0.98f;
    }
    return clamp( 1.0f - 1.6f*occ, 0.0f, 1.0f );    
}

//from https://www.shadertoy.com/view/lsKcDD
__DEVICE__ float calcSoftshadow_o8705( in float3 ro, in float3 rd, in float mint, in float tmax, float _seed_variation_) {
  float res = 1.0f;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    for( int i=0; i<32; i++ ){
    float h = o8705_input_mfsdf(to_float4_aw( ro + rd*t ,0.0f), _seed_variation_).w;
        res = _fminf( res, 10.0f*h/t );
        t += h;
        if( res<0.0001f || t>tmax ) break;  
    }
    return clamp( res, 0.0f, 1.0f );
}

__DEVICE__ float3 raymarch_o8705(float2 uv, float _seed_variation_) {
    uv-=0.5f;

    float mx=iMouse.x/iResolution.x*PI*2.0f;
    float my=iMouse.y/iResolution.y*PI/2.01f;
    
    float3 lookat=to_float3(p_o8705_LookAtX,p_o8705_LookAtY,p_o8705_LookAtZ);
    
    float3 cam;
    if (iMouse.z<0.1f){
        cam=to_float3_aw((_sinf(iTime*0.1f)*2.0f),(_sinf(iTime*0.13f)*1.0f),(_sinf(iTime*0.17f)*2.0f))*p_o8705_CamZoom;
    } else {
        cam=lookat+to_float3_aw(_cosf(my)*_cosf(mx),_sinf(my),_cosf(my)*_sinf(mx))*6.0f;
    }
    
  float3 ray=normalize(lookat-cam);
  float3 cX=normalize(cross(to_float3(0.0f,1.0f,0.0f),ray));
  float3 cY=normalize(cross(cX,ray));
  float3 rd = normalize(ray*p_o8705_CamD+cX*uv.x+cY*uv.y);
  float3 ro = cam;
  
  float d=0.0f;
  float3 p=to_float3(0);
  float dS=0.0f;
  march_o8705(d,p,dS,ro,rd, _seed_variation_);
  
    float3 color=to_float3_s(0.0f);
  float3 objColor=o8705_input_mfsdf(to_float4_aw(p,1.0f), _seed_variation_).xyz;   // 1 - BaseColor (r,g,b,sdf) linear (0-1) 
  float objSpecular=o8705_input_mfsdf(to_float4_aw(p,4.0f), _seed_variation_).x*0.2f;  // 4 - Specular  (v,0,0,sdf)
  float objRoughness=o8705_input_mfsdf(to_float4_aw(p,5.0f), _seed_variation_).x; // 5 - Roughness (v,0,0,sdf)
  float3 light=normalize(to_float3(p_o8705_SunX,p_o8705_SunY,p_o8705_SunZ));
  if (d<50.0f) {
      float3 n=normal_o8705(p, _seed_variation_);
    float l=clamp(dot(-light,-n),0.0f,1.0f);
    float3 ref=normalize(reflect(rd,-n));
    float r=clamp(dot(ref,light),0.0f,1.0f);
    float cAO=_mix(1.0f,calcAO_o8705(p,n, _seed_variation_),p_o8705_AmbOcclusion);
    float shadow=_mix(1.0f,calcSoftshadow_o8705(p,light,0.05f,5.0f, _seed_variation_),p_o8705_Shadow);
    color=_fminf(to_float3_aw(_fmaxf(shadow,p_o8705_AmbLight)),_fmaxf(l,p_o8705_AmbLight))*_fmaxf(cAO,p_o8705_AmbLight)*objColor+4.0f*_powf(r,_powf(256.0f,(1.0f-objRoughness)))*objSpecular;
    //reflection
    d=0.01f;
    march_o8705(d,p,dS,p,ref, _seed_variation_);
    float3 objColorRef=to_float3_aw(0);
    if (d<50.0f) {
      objColorRef=o8705_input_mfsdf(to_float4(p,1.0f), _seed_variation_).xyz;
      n=normal_o8705(p, _seed_variation_);
      l=clamp(dot(-light,-n),0.0f,1.0f);
      objColorRef=_fmaxf(l,p_o8705_AmbLight)*objColorRef;
    } else {
      objColorRef=o8705_input_hdri(equirectangularMap(swi3(ref,x,z,y)), _seed_variation_).xyz;
    }
    color=_mix(color,objColorRef,objSpecular);
  } else {
    color=o8705_input_hdri(equirectangularMap(swi3(rd,x,z,y)), _seed_variation_).xyz;
  }
  return _powf(color,to_float3_aw(1.0f/p_o8705_Gamma));
}

__KERNEL__ void D12032022MmExperimentFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    float minSize = _fminf(iResolution.x, iResolution.y);
    float _seed_variation_ = SEED_VARIATION;
    float2 UV = to_float2(0.0f, 1.0f) + to_float2(1.0f, -1.0f) * (fragCoord-0.5f*(iResolution-to_float2(minSize)))/minSize;
    float3 o8705_0_1_rgb = raymarch_o8705((UV), _seed_variation_);
    fragColor = to_float4_aw(_powf(o8705_0_1_rgb,to_float3(1.0f/1.2f)), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
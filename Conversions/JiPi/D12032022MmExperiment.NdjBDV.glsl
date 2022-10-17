

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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


#define SEED_VARIATION 0.0

float wave3d_square(float x) {
	return (fract(x) < 0.5) ? 0.0 : 1.0;
}

float mix3d_xor(float x, float y, float z) {
	float xy = min(x+y, 2.0-x-y);
	return min(xy+z, 2.0-xy-z);
}

vec3 MFSDF_Obj_Maker_rotate3d(vec3 p, vec3 a) {
	vec3 rv;
	float c;
	float s;
	c = cos(a.x);
	s = sin(a.x);
	rv.x = p.x;
	rv.y = p.y*c+p.z*s;
	rv.z = -p.y*s+p.z*c;
	c = cos(a.y);
	s = sin(a.y);
	p.x = rv.x*c+rv.z*s;
	p.y = rv.y;
	p.z = -rv.x*s+rv.z*c;
	c = cos(a.z);
	s = sin(a.z);
	rv.x = p.x*c+p.y*s;
	rv.y = -p.x*s+p.y*c;
	rv.z = p.z;
	return rv;
}

float mfsdf3d_smooth_union_f(float a,float b,float k){
	float h = max( k-abs(a-b), 0.0 )/k;
	return min(a,b)-h*h*k*0.25;
}

vec4 mfsdf3d_smooth_union(vec4 a, vec4 b, float k) {
    float e=0.001;
    k=max(k,e);
    float h=mfsdf3d_smooth_union_f(a.w,b.w,k);
	vec2 n=normalize(vec2(mfsdf3d_smooth_union_f(a.w+e,b.w,k)-mfsdf3d_smooth_union_f(a.w-e,b.w,k),
                          mfsdf3d_smooth_union_f(a.w,b.w+e,k)-mfsdf3d_smooth_union_f(a.w,b.w-e,k)));
    return vec4(mix(a.xyz,b.xyz,atan(abs(n.y),abs(n.x))/(3.14159265359/2.0)),h);
}

float mfsdf3d_smooth_subtraction_f(float a,float b,float k){
	float h = max( k-abs(-a-b), 0.0 )/k;
	return max(-a,b)+h*h*k*0.25;
}

vec4 mfsdf3d_smooth_subtraction(vec4 a, vec4 b, float k) {
    float e=0.001;
    k=max(k,e);
	float h=mfsdf3d_smooth_subtraction_f(a.w,b.w,k);
	vec2 n=normalize(vec2(mfsdf3d_smooth_subtraction_f(a.w+e,b.w,k)-mfsdf3d_smooth_subtraction_f(a.w-e,b.w,k),
                          mfsdf3d_smooth_subtraction_f(a.w,b.w+e,k)-mfsdf3d_smooth_subtraction_f(a.w,b.w-e,k)));
    return vec4(mix(a.xyz,b.xyz,atan(abs(n.y),abs(n.x))/(3.14159265359/2.0)),h);
}

float mfsdf3d_smooth_intersection_f(float a,float b,float k){
	float h = max( k-abs(a-b), 0.0 )/k;
	return max(a,b)+h*h*k*0.25;
}

vec4 mfsdf3d_smooth_intersection(vec4 a, vec4 b, float k) {
    float e=0.001;
    k=max(k,e);
	float h=mfsdf3d_smooth_intersection_f(a.w,b.w,k);
	vec2 n=normalize(vec2(mfsdf3d_smooth_intersection_f(a.w+e,b.w,k)-mfsdf3d_smooth_intersection_f(a.w-e,b.w,k),
                          mfsdf3d_smooth_intersection_f(a.w,b.w+e,k)-mfsdf3d_smooth_intersection_f(a.w,b.w-e,k)));
    return vec4(mix(a.xyz,b.xyz,atan(abs(n.y),abs(n.x))/(3.14159265359/2.0)),h);
}

// https://www.shadertoy.com/view/XsX3zB by Nikita Miropolskiy
// MIT License

// discontinuous pseudorandom constly distributed in [-0.5, +0.5]^3 */
vec3 XsX3zB_oct_random3(vec3 c) {
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

// skew constants for 3d simplex functions
const float XsX3zB_oct_F3 =  0.3333333;
const float XsX3zB_oct_G3 =  0.1666667;

// 3d simplex noise
float XsX3zB_oct_simplex3d(vec3 p) {
	 // 1. find current tetrahedron T and it's four vertices
	 // s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices
	 // x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices
	 
	 // calculate s and x
	 vec3 s = floor(p + dot(p, vec3(XsX3zB_oct_F3)));
	 vec3 x = p - s + dot(s, vec3(XsX3zB_oct_G3));
	 
	 // calculate i1 and i2
	 vec3 e = step(vec3(0.0), x - x.yzx);
	 vec3 i1 = e*(1.0 - e.zxy);
	 vec3 i2 = 1.0 - e.zxy*(1.0 - e);
	 	
	 // x1, x2, x3
	 vec3 x1 = x - i1 + XsX3zB_oct_G3;
	 vec3 x2 = x - i2 + 2.0*XsX3zB_oct_G3;
	 vec3 x3 = x - 1.0 + 3.0*XsX3zB_oct_G3;
	 
	 // 2. find four surflets and store them in d
	 vec4 w, d;
	 
	 // calculate surflet weights
	 w.x = dot(x, x);
	 w.y = dot(x1, x1);
	 w.z = dot(x2, x2);
	 w.w = dot(x3, x3);
	 
	 // w fades from 0.6 at the center of the surflet to 0.0 at the margin
	 w = max(0.6 - w, 0.0);
	 
	 // calculate surflet components
	 d.x = dot(XsX3zB_oct_random3(s), x);
	 d.y = dot(XsX3zB_oct_random3(s + i1), x1);
	 d.z = dot(XsX3zB_oct_random3(s + i2), x2);
	 d.w = dot(XsX3zB_oct_random3(s + 1.0), x3);
	 
	 // multiply d by w^4
	 w *= w;
	 w *= w;
	 d *= w;
	 
	 // 3. return the sum of the four surflets
	 return dot(d, vec4(52.0));
}

vec3 v4v4_rotate(vec3 p, vec3 a) {
	vec3 rv;
	float c;
	float s;
	c = cos(a.x);
	s = sin(a.x);
	rv.x = p.x;
	rv.y = p.y*c+p.z*s;
	rv.z = -p.y*s+p.z*c;
	c = cos(a.y);
	s = sin(a.y);
	p.x = rv.x*c+rv.z*s;
	p.y = rv.y;
	p.z = -rv.x*s+rv.z*c;
	c = cos(a.z);
	s = sin(a.z);
	rv.x = p.x*c+p.y*s;
	rv.y = -p.x*s+p.y*c;
	rv.z = p.z;
	return rv;
}
const float PI=3.14159265359;

vec2 equirectangularMap(vec3 dir) {
	vec2 longlat = vec2(atan(dir.y,dir.x),acos(dir.z));
 	return longlat/vec2(2.0*PI,PI);
}


//Simple HDRI START

//Hash without Sine Dave_Hoskins
//https://www.shadertoy.com/view/4djSRW 
float Simple360HDR_hash12(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float Simple360HDR_noise(vec2 v){
  vec2 v1=floor(v);
  vec2 v2=smoothstep(0.0,1.0,fract(v));
  float n00=Simple360HDR_hash12(v1);
  float n01=Simple360HDR_hash12(v1+vec2(0,1));
  float n10=Simple360HDR_hash12(v1+vec2(1,0));
  float n11=Simple360HDR_hash12(v1+vec2(1,1));
  return mix(mix(n00,n01,v2.y),mix(n10,n11,v2.y),v2.x);
}

float Simple360HDR_noiseOct(vec2 p){
  return
    Simple360HDR_noise(p)*0.5+
    Simple360HDR_noise(p*2.0+13.0)*0.25+
    Simple360HDR_noise(p*4.0+23.0)*0.15+
    Simple360HDR_noise(p*8.0+33.0)*0.10+
    Simple360HDR_noise(p*16.0+43.0)*0.05;
}

vec3 Simple360HDR_skyColor(vec3 p){
	vec3 s1=vec3(0.2,0.5,1.0);
	vec3 s2=vec3(0.1,0.2,0.4)*1.5;
    vec3 v=(Simple360HDR_noiseOct(p.xz*0.1)-0.5)*vec3(1.0);
	float d=length(p);
    return mix(s2+v,s1+v*(12.0/max(d,20.0)),clamp(d*0.1,0.0,1.0));
}

vec3 Simple360HDR_floorColor(vec3 p){
    vec3 v=(Simple360HDR_noiseOct(p.xz*0.1)*0.5+0.25)*vec3(0.7,0.5,0.4);
    return v;
}

vec3 Simple360HDR_renderHDR360(vec3 rd, vec3 sun){
    vec3 col;
	vec3 p;
	vec3 c;
	if (rd.y>0.0) {
        p=rd*(5.0/rd.y);
        c=Simple360HDR_skyColor(p);
    } else {
        p=rd*(-10.0/rd.y);
        c=Simple360HDR_floorColor(p);
		c=mix(c,vec3(0.5,0.7,1.0),clamp(1.0-sqrt(-rd.y)*3.0,0.0,1.0));
	}
	vec3 skycolor=vec3(0.1,0.45,0.68);
	float d=length(p);
	
	float ds=clamp(dot(sun,rd),0.0,1.0);
	vec3 sunc=(ds>0.9997?vec3(2.0):vec3(0.0))+pow(ds,512.0)*4.0+pow(ds,128.0)*vec3(0.5)+pow(ds,4.0)*vec3(0.5);
    if (rd.y>0.0){
		c+=vec3(0.3)*pow(1.0-abs(rd.y),3.0)*0.7;
	} 
    return c+sunc;
}

vec3 Simple360HDR_make360hdri(vec2 p, vec3 sun){
    float xPI=3.14159265359;
    vec2 thetaphi = ((p * 2.0) - vec2(1.0)) * vec2(xPI,xPI/2.0); 
    vec3 rayDirection = vec3(cos(thetaphi.y) * cos(thetaphi.x), sin(thetaphi.y), cos(thetaphi.y) * sin(thetaphi.x));
    return Simple360HDR_renderHDR360(rayDirection,sun);
}

//Simple HDRI END
const float p_o8705_CamX = 0.872000000;
const float p_o8705_CamY = 1.094000000;
const float p_o8705_CamZ = 3.000000000;
const float p_o8705_LookAtX = 0.000000000;
const float p_o8705_LookAtY = 0.000000000;
const float p_o8705_LookAtZ = 0.000000000;
const float p_o8705_CamD = 1.500000000;
const float p_o8705_CamZoom = 2.347000000;
const float p_o8705_SunX = 2.500000000;
const float p_o8705_SunY = 2.500000000;
const float p_o8705_SunZ = 2.500000000;
const float p_o8705_AmbLight = 0.250000000;
const float p_o8705_AmbOcclusion = 1.000000000;
const float p_o8705_Shadow = 1.000000000;
const float p_o8705_Gamma = 1.300000000;
const float p_o8718_xyz = 0.451000000;
const float p_o8718_x = 1.000000000;
const float p_o8718_y = 1.000000000;
const float p_o8718_z = 1.000000000;
const float p_o8738_k = 0.446000000;
const float p_o8725_k = 1.000000000;
const float p_o8714_BaseColor_r = 1.000000000;
const float p_o8714_BaseColor_g = 0.656250000;
const float p_o8714_BaseColor_b = 0.000000000;
const float p_o8714_BaseColor_a = 1.000000000;
const float p_o8714_Specular = 0.961000000;
const float p_o8714_Roughness = 0.000000000;


float o8762_fct(vec3 uv, float _seed_variation_) {
	return mix3d_xor(wave3d_square(uv.x), wave3d_square(uv.y), wave3d_square(uv.z));
}

vec3 o8714_input_BaseColor_tex3d(vec4 p, float _seed_variation_) {
    vec3 o8762_0_1_tex3d = vec3(o8762_fct((p).xyz, _seed_variation_));
    return o8762_0_1_tex3d;
}

float o8714_input_Metallic_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}

float o8714_input_Specular_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}

float o8714_input_Roughness_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}

vec3 o8714_input_Emission_tex3d(vec4 p, float _seed_variation_) {
    return vec3(1.0,1.0,1.0);
}

vec3 o8714_input_Normal_tex3d(vec4 p, float _seed_variation_) {
    return vec3(0.0,1.0,0.0);
}

float o8714_input_Alpha_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}
float o8714_input_AmbientOcclusion_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}

const float p_o8719_d = 3.140000000;
const float p_o8713_r = 0.610000000;

float o8719_input_in(vec2 uv, float _seed_variation_) {
    float o8713_0_1_sdf2d = length((uv)-vec2((cos(_seed_variation_*6.0)*0.4)+0.5, (sin(_seed_variation_*5.0)*0.4)+0.5))-p_o8713_r;
    return o8713_0_1_sdf2d;
}

float o8714_input_sdf3d(vec3 p, float _seed_variation_) {
    vec2 o8719_0_q = vec2(length((p).xy)-p_o8719_d+0.5, (p).z+0.5);
    float o8719_0_1_sdf3d = o8719_input_in(o8719_0_q,atan((p).x,(p).y));
    return o8719_0_1_sdf3d;
}


vec4 PBRObjectMaker_o8714(vec4 uv, float _seed_variation_) {
	float sdf=o8714_input_sdf3d(uv.xyz, _seed_variation_);
	//5 - Roughness
	if (uv.w>4.5) {
		return vec4(p_o8714_Roughness*o8714_input_Roughness_tex3d(vec4(uv.xyz,5.0), _seed_variation_),0.0,0.0,sdf);
	} else
	//4 - Specular
	if (uv.w>3.5) {
		return vec4(p_o8714_Specular*o8714_input_Specular_tex3d(vec4(uv.xyz,4.0), _seed_variation_),0.0,0.0,sdf);
	} else
	//1 - BaseColor
	if (uv.w>0.5){
		return vec4(vec4(p_o8714_BaseColor_r, p_o8714_BaseColor_g, p_o8714_BaseColor_b, p_o8714_BaseColor_a).rgb*o8714_input_BaseColor_tex3d(vec4(uv.xyz,1.0), _seed_variation_),sdf);
	} else
	//0 - SDF
	{
		return vec4(vec3(0),sdf);
	}
}

const float p_o8726_BaseColor_r = 0.000000000;
const float p_o8726_BaseColor_g = 0.424825996;
const float p_o8726_BaseColor_b = 1.000000000;
const float p_o8726_BaseColor_a = 1.000000000;
const float p_o8726_Specular = 0.969000000;
const float p_o8726_Roughness = 0.000000000;


vec3 o8726_input_BaseColor_tex3d(vec4 p, float _seed_variation_) {
    return vec3(1.0);
}

float o8726_input_Specular_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}

float o8726_input_Roughness_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}

const float p_o8737_d = 4.730000000;
const float p_o8730_r = 0.500000000;

float o8737_input_in(vec2 uv, float _seed_variation_) {
    float o8730_0_1_sdf2d = length((uv)-vec2((cos(_seed_variation_*9.0)*0.6)+0.5, (sin(_seed_variation_*7.0)*0.4)+0.5))-p_o8730_r;
    return o8730_0_1_sdf2d;
}

float o8726_input_sdf3d(vec3 p, float _seed_variation_) {
    vec2 o8737_0_q = vec2(length((p).xy)-p_o8737_d+0.5, (p).z+0.5);
    float o8737_0_1_sdf3d = o8737_input_in(o8737_0_q,atan((p).x,(p).y));
    return o8737_0_1_sdf3d;
}

vec4 PBRObjectMaker_o8726(vec4 uv, float _seed_variation_) {
   // uv.xyz=MFSDF_Obj_Maker_rotate3d(uv.xyz-vec3(p_o8726_TranlateX,p_o8726_TranlateY,p_o8726_TranlateZ),vec3(p_o8726_RotateX,p_o8726_RotateY,p_o8726_RotateZ)*6.28318530718)/p_o8726_scale;
	float sdf=o8726_input_sdf3d(uv.xyz, _seed_variation_);
	//5 - Roughness
	if (uv.w>4.5) {
		return vec4(p_o8726_Roughness*o8726_input_Roughness_tex3d(vec4(uv.xyz,5.0), _seed_variation_),0.0,0.0,sdf);
	} else
	//4 - Specular
	if (uv.w>3.5) {
		return vec4(p_o8726_Specular*o8726_input_Specular_tex3d(vec4(uv.xyz,4.0), _seed_variation_),0.0,0.0,sdf);
	} else
	//1 - BaseColor
	if (uv.w>0.5){
		return vec4(vec4(p_o8726_BaseColor_r, p_o8726_BaseColor_g, p_o8726_BaseColor_b, p_o8726_BaseColor_a).rgb*o8726_input_BaseColor_tex3d(vec4(uv.xyz,1.0), _seed_variation_),sdf);
	} else
	//0 - SDF
	{
		return vec4(vec3(0),sdf);
	}
}

const float p_o8733_BaseColor_r = 1.000000000;
const float p_o8733_BaseColor_g = 1.000000000;
const float p_o8733_BaseColor_b = 1.000000000;
const float p_o8733_BaseColor_a = 1.000000000;
const float p_o8733_Specular = 1.000000000;
const float p_o8733_Roughness = 0.000000000;
const float p_o8733_scale = 1.000000000;
const float p_o8733_TranlateX = 0.000000000;
const float p_o8733_TranlateY = 0.000000000;
const float p_o8733_TranlateZ = 0.000000000;
const float p_o8733_RotateX = 0.000000000;
const float p_o8733_RotateY = 0.000000000;
const float p_o8733_RotateZ = 0.000000000;
const float p_o8749_g_0_pos = 0.409091000;
const float p_o8749_g_0_r = 1.000000000;
const float p_o8749_g_0_g = 0.000000000;
const float p_o8749_g_0_b = 0.000000000;
const float p_o8749_g_0_a = 1.000000000;
const float p_o8749_g_1_pos = 0.518182000;
const float p_o8749_g_1_r = 1.000000000;
const float p_o8749_g_1_g = 1.000000000;
const float p_o8749_g_1_b = 1.000000000;
const float p_o8749_g_1_a = 1.000000000;

vec4 o8749_g_gradient_fct(float x) {
  if (x < p_o8749_g_0_pos) {
    return vec4(p_o8749_g_0_r,p_o8749_g_0_g,p_o8749_g_0_b,p_o8749_g_0_a);
  } else if (x < p_o8749_g_1_pos) {
    return mix(vec4(p_o8749_g_0_r,p_o8749_g_0_g,p_o8749_g_0_b,p_o8749_g_0_a), vec4(p_o8749_g_1_r,p_o8749_g_1_g,p_o8749_g_1_b,p_o8749_g_1_a), ((x-p_o8749_g_0_pos)/(p_o8749_g_1_pos-p_o8749_g_0_pos)));
  }
  return vec4(p_o8749_g_1_r,p_o8749_g_1_g,p_o8749_g_1_b,p_o8749_g_1_a);
}

const float p_o8744_scale = 3.768000000;
const float p_o8744_scale_x = 1.000000000;
const float p_o8744_scale_y = 1.000000000;
const float p_o8744_scale_z = 1.000000000;
const float p_o8744_transx = 0.000000000;
const float p_o8744_transy = 0.000000000;
const float p_o8744_transz = 0.000000000;
const float p_o8744_persistence = 0.500000000;
const float p_o8744_brightness = 0.000000000;
const float p_o8744_contrast = 2.255000000;

float o8744_fbm(vec3 coord, float persistence, float _seed_variation_) {
	float normalize_factor = 0.0;
	float value = 0.0;
	float scale = 1.0;
	float size = 1.0;
	for (int i = 0; i < 2; i++) {
		value += XsX3zB_oct_simplex3d(coord*size) * scale;
		normalize_factor += scale;
		size *= 2.0;
		scale *= persistence;
	}
	return value / normalize_factor;
}

float o8744_bc(float f,float contrast, float brightness, float _seed_variation_) {
	return f*contrast+brightness+0.5-contrast*0.5;
}

vec3 o8733_input_BaseColor_tex3d(vec4 p, float _seed_variation_) {
    vec3 o8744_0_out = vec3(o8744_bc(o8744_fbm((p).xyz*vec3(p_o8744_scale_x,p_o8744_scale_y,p_o8744_scale_z)*0.5*p_o8744_scale+vec3(p_o8744_transx,p_o8744_transy,p_o8744_transz),p_o8744_persistence, _seed_variation_)*0.5+0.5,p_o8744_contrast,p_o8744_brightness, _seed_variation_));vec3 o8744_0_1_tex3d = o8744_0_out;
    vec3 o8749_0_1_tex3d = o8749_g_gradient_fct(dot(o8744_0_1_tex3d, vec3(1.0))/3.0).rgb;
    return o8749_0_1_tex3d;
}

float o8733_input_Specular_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}

float o8733_input_Roughness_tex3d(vec4 p, float _seed_variation_) {
    return 1.0;
}

const float p_o8739_Distort = 0.107000000;
const float p_o8739_Correction = 0.000000000;
const float p_o8739_Bound = 9.411000000;
const float p_o8732_d = 4.320000000;
const float p_o8731_r = 0.610000000;

float o8732_input_in(vec2 uv, float _seed_variation_) {
    float o8731_0_1_sdf2d = length((uv)-vec2((cos(_seed_variation_*4.0)*0.6)+0.5, (sin(_seed_variation_*4.0)*0.4)+0.5))-p_o8731_r;
    return o8731_0_1_sdf2d;
}

float o8739_input_sdf(vec3 p, float _seed_variation_) {
    vec2 o8732_0_q = vec2(length((p).xy)-p_o8732_d+0.5, (p).z+0.5);
    float o8732_0_1_sdf3d = o8732_input_in(o8732_0_q,atan((p).x,(p).y));
    return o8732_0_1_sdf3d;
}

vec3 o8739_input_tex3d(vec4 p, float _seed_variation_) {
    vec3 o8744_0_out = vec3(o8744_bc(o8744_fbm((p).xyz*vec3(p_o8744_scale_x,p_o8744_scale_y,p_o8744_scale_z)*0.5*p_o8744_scale+vec3(p_o8744_transx,p_o8744_transy,p_o8744_transz),p_o8744_persistence, _seed_variation_)*0.5+0.5,p_o8744_contrast,p_o8744_brightness, _seed_variation_));vec3 o8744_0_1_tex3d = o8744_0_out;
    return o8744_0_1_tex3d;
}

//tetrahedron normal by PauloFalcao
//https://www.shadertoy.com/view/XstGDS
vec3 normal_o8739(vec3 p, float _seed_variation_) {  
  const vec3 e=vec3(0.001,-0.001,0.0);
  float v1=o8739_input_sdf(p+e.xyy, _seed_variation_);
  float v2=o8739_input_sdf(p+e.yyx, _seed_variation_);
  float v3=o8739_input_sdf(p+e.yxy, _seed_variation_);
  float v4=o8739_input_sdf(p+e.xxx, _seed_variation_);
  return normalize(vec3(v4+v1-v3-v2,v3+v4-v1-v2,v2+v4-v3-v1));
}

float distortByNormal_o8739(vec3 uv, float _seed_variation_) {
    float d=o8739_input_sdf(uv, _seed_variation_);
	if (d<=abs(p_o8739_Distort*(p_o8739_Bound+1.0))+0.01){
		vec3 n=normal_o8739(uv, _seed_variation_);
		vec3 s=o8739_input_tex3d(vec4(uv,0.0), _seed_variation_);
		return o8739_input_sdf(uv-(n*s*p_o8739_Distort), _seed_variation_);
	} else return d;
}

float o8733_input_sdf3d(vec3 p, float _seed_variation_) {
    float o8739_0_1_sdf3d = distortByNormal_o8739((p), _seed_variation_)/(1.0+p_o8739_Distort*p_o8739_Correction);
    return o8739_0_1_sdf3d;
}


vec4 PBRObjectMaker_o8733(vec4 uv, float _seed_variation_) {
    uv.xyz=MFSDF_Obj_Maker_rotate3d(uv.xyz-vec3(p_o8733_TranlateX,p_o8733_TranlateY,p_o8733_TranlateZ),vec3(p_o8733_RotateX,p_o8733_RotateY,p_o8733_RotateZ)*6.28318530718)/p_o8733_scale;
	float sdf=o8733_input_sdf3d(uv.xyz, _seed_variation_)*p_o8733_scale;
	//5 - Roughness
	if (uv.w>4.5) {
		return vec4(p_o8733_Roughness*o8733_input_Roughness_tex3d(vec4(uv.xyz,5.0), _seed_variation_),0.0,0.0,sdf);
	} else
	//4 - Specular
	if (uv.w>3.5) {
		return vec4(p_o8733_Specular*o8733_input_Specular_tex3d(vec4(uv.xyz,4.0), _seed_variation_),0.0,0.0,sdf);
	} else
	//1 - BaseColor
	if (uv.w>0.5){
		return vec4(vec4(p_o8733_BaseColor_r, p_o8733_BaseColor_g, p_o8733_BaseColor_b, p_o8733_BaseColor_a).rgb*o8733_input_BaseColor_tex3d(vec4(uv.xyz,1.0), _seed_variation_),sdf);
	} else
	//0 - SDF
	{
		return vec4(vec3(0),sdf);
	}
}

vec4 o8705_input_mfsdf(vec4 p, float _seed_variation_) {

    vec3 r=v4v4_rotate(p.xyz, -vec3(iTime*31.0, iTime*33.0, iTime*35.0)*0.01745329251);
    if (iMouse.z>0.5){
        r=vec3(p.xyz);
    }
    vec4 rot=vec4(vec4(r, p.w).xyz/vec3(p_o8718_x, p_o8718_y, p_o8718_z)/p_o8718_xyz,vec4(r,p.w).w);

    vec4 o8714_0_1_v4v4 = PBRObjectMaker_o8714(rot, _seed_variation_);
    vec4 o8726_0_1_v4v4 = PBRObjectMaker_o8726(rot, _seed_variation_);
    vec4 o8725_0_1_v4v4 = mfsdf3d_smooth_union(o8714_0_1_v4v4, o8726_0_1_v4v4,p_o8725_k);
    vec4 o8733_0_1_v4v4 = PBRObjectMaker_o8733(rot, _seed_variation_);
    vec4 o8738_0_1_v4v4 = mfsdf3d_smooth_union(o8725_0_1_v4v4, o8733_0_1_v4v4,p_o8738_k);
    vec4 o_o8718_0=o8738_0_1_v4v4;vec4 o8718_0_1_v4v4 = vec4(o_o8718_0.xyz,o_o8718_0.w*min(min(p_o8718_x, p_o8718_y), p_o8718_z)*p_o8718_xyz);
    vec4 o8720_0_1_v4v4 = o8718_0_1_v4v4;

    return o8720_0_1_v4v4;
}
vec3 o8705_input_hdri(vec2 uv, float _seed_variation_) {

return Simple360HDR_make360hdri(vec2((uv).x,-(uv).y+1.0),normalize(vec3(-p_o8705_SunX,p_o8705_SunY,-p_o8705_SunZ)));
}

//tetrahedron normal by PauloFalcao
//https://www.shadertoy.com/view/XstGDS
vec3 normal_o8705(vec3 p, float _seed_variation_) {  
  const vec3 e=vec3(0.001,-0.001,0.0);
  float v1=o8705_input_mfsdf(vec4(p+e.xyy,0.0), _seed_variation_).w;
  float v2=o8705_input_mfsdf(vec4(p+e.yyx,0.0), _seed_variation_).w;
  float v3=o8705_input_mfsdf(vec4(p+e.yxy,0.0), _seed_variation_).w;
  float v4=o8705_input_mfsdf(vec4(p+e.xxx,0.0), _seed_variation_).w;
  return normalize(vec3(v4+v1-v3-v2,v3+v4-v1-v2,v2+v4-v3-v1));
}

void march_o8705(inout float d,inout vec3 p,float dS, vec3 ro, vec3 rd, float _seed_variation_) {
    for (int i=0; i < 500; i++) {
    	p = ro + rd*d;
        dS = o8705_input_mfsdf(vec4(p,0.0), _seed_variation_).w;
        d += dS;
        if (d > 50.0 || abs(dS) < 0.0001) break;
    }
}

//from https://www.shadertoy.com/view/lsKcDD
float calcAO_o8705( in vec3 pos, in vec3 nor , float _seed_variation_) {
	float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ ){
        float h = 0.001 + 0.25*float(i)/4.0;
        float d = o8705_input_mfsdf(vec4( pos + h*nor ,0.0), _seed_variation_).w;
        occ += (h-d)*sca;
        sca *= 0.98;
    }
    return clamp( 1.0 - 1.6*occ, 0.0, 1.0 );    
}

//from https://www.shadertoy.com/view/lsKcDD
float calcSoftshadow_o8705( in vec3 ro, in vec3 rd, in float mint, in float tmax, float _seed_variation_) {
	float res = 1.0;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration
    for( int i=0; i<32; i++ ){
		float h = o8705_input_mfsdf(vec4( ro + rd*t ,0.0), _seed_variation_).w;
        res = min( res, 10.0*h/t );
        t += h;
        if( res<0.0001 || t>tmax ) break;  
    }
    return clamp( res, 0.0, 1.0 );
}

vec3 raymarch_o8705(vec2 uv, float _seed_variation_) {
    uv-=0.5;

    float mx=iMouse.x/iResolution.x*PI*2.0;
    float my=iMouse.y/iResolution.y*PI/2.01;
    
    vec3 lookat=vec3(p_o8705_LookAtX,p_o8705_LookAtY,p_o8705_LookAtZ);
    
    vec3 cam;
    if (iMouse.z<0.1){
        cam=vec3((sin(iTime*0.1)*2.0),(sin(iTime*0.13)*1.0),(sin(iTime*0.17)*2.0))*p_o8705_CamZoom;
    } else {
        cam=lookat+vec3(cos(my)*cos(mx),sin(my),cos(my)*sin(mx))*6.0;
    }
    
	vec3 ray=normalize(lookat-cam);
	vec3 cX=normalize(cross(vec3(0.0,1.0,0.0),ray));
	vec3 cY=normalize(cross(cX,ray));
	vec3 rd = normalize(ray*p_o8705_CamD+cX*uv.x+cY*uv.y);
	vec3 ro = cam;
	
	float d=0.;
	vec3 p=vec3(0);
	float dS=0.0;
	march_o8705(d,p,dS,ro,rd, _seed_variation_);
	
    vec3 color=vec3(0.0);
	vec3 objColor=o8705_input_mfsdf(vec4(p,1.0), _seed_variation_).xyz;   // 1 - BaseColor (r,g,b,sdf) linear (0-1) 
	float objSpecular=o8705_input_mfsdf(vec4(p,4.0), _seed_variation_).x*0.2;  // 4 - Specular  (v,0,0,sdf)
	float objRoughness=o8705_input_mfsdf(vec4(p,5.0), _seed_variation_).x; // 5 - Roughness (v,0,0,sdf)
	vec3 light=normalize(vec3(p_o8705_SunX,p_o8705_SunY,p_o8705_SunZ));
	if (d<50.0) {
	    vec3 n=normal_o8705(p, _seed_variation_);
		float l=clamp(dot(-light,-n),0.0,1.0);
		vec3 ref=normalize(reflect(rd,-n));
		float r=clamp(dot(ref,light),0.0,1.0);
		float cAO=mix(1.0,calcAO_o8705(p,n, _seed_variation_),p_o8705_AmbOcclusion);
		float shadow=mix(1.0,calcSoftshadow_o8705(p,light,0.05,5.0, _seed_variation_),p_o8705_Shadow);
		color=min(vec3(max(shadow,p_o8705_AmbLight)),max(l,p_o8705_AmbLight))*max(cAO,p_o8705_AmbLight)*objColor+4.0*pow(r,pow(256.0,(1.0-objRoughness)))*objSpecular;
		//reflection
		d=0.01;
		march_o8705(d,p,dS,p,ref, _seed_variation_);
		vec3 objColorRef=vec3(0);
		if (d<50.0) {
			objColorRef=o8705_input_mfsdf(vec4(p,1.0), _seed_variation_).xyz;
			n=normal_o8705(p, _seed_variation_);
			l=clamp(dot(-light,-n),0.0,1.0);
			objColorRef=max(l,p_o8705_AmbLight)*objColorRef;
		} else {
			objColorRef=o8705_input_hdri(equirectangularMap(ref.xzy), _seed_variation_).xyz;
		}
		color=mix(color,objColorRef,objSpecular);
	} else {
		color=o8705_input_hdri(equirectangularMap(rd.xzy), _seed_variation_).xyz;
	}
	return pow(color,vec3(1.0/p_o8705_Gamma));
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    float minSize = min(iResolution.x, iResolution.y);
    float _seed_variation_ = SEED_VARIATION;
    vec2 UV = vec2(0.0, 1.0) + vec2(1.0, -1.0) * (fragCoord-0.5*(iResolution.xy-vec2(minSize)))/minSize;
    vec3 o8705_0_1_rgb = raymarch_o8705((UV), _seed_variation_);
    fragColor = vec4(pow(o8705_0_1_rgb,vec3(1.0/1.2)), 1.0);
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Audio' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// CC0: Electric Eel Universe
//  Saturday tinkering with an old failed shader
//  Turned out a bit better today

#define PI              3.141592654f
#define TAU             (2.0f*PI)
#define TIME            iTime
#define RESOLUTION      iResolution
#define ROT(a)          to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a))

// License: WTFPL, author: sam hocevar, found: https://stackoverflow.com/a/17897228/418488
//const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
__DEVICE__ float3 hsv2rgb(float3 c) {
  const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);	
	
  float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(hsv2rgb_K,x,y,z)) * 6.0f - swi3(hsv2rgb_K,w,w,w));
  return c.z * _mix(swi3(hsv2rgb_K,x,x,x), clamp(p - swi3(hsv2rgb_K,x,x,x), 0.0f, 1.0f), c.y);
}
// License: WTFPL, author: sam hocevar, found: https://stackoverflow.com/a/17897228/418488
//  Macro version of above to enable compile-time constants
#define HSV2RGB(c)  (c.z * _mix(swi3(hsv2rgb_K,x,x,x), clamp(abs_f3(fract_f3(swi3(c,x,x,x) + swi3(hsv2rgb_K,x,y,z)) * 6.0f - swi3(hsv2rgb_K,w,w,w)) - swi3(hsv2rgb_K,x,x,x), 0.0f, 1.0f), c.y))

// License: Unknown, author: Unknown, found: don't remember
__DEVICE__ float hash(float co) {
  return fract(_sinf(co*12.9898f) * 13758.5453f);
}

// License: MIT OR CC-BY-NC-4.0f, author: mercury, found: https://mercury.sexy/hg_sdf/
__DEVICE__ float mod1(inout float *p, float size) {
  float halfsize = size*0.5f;
  float c = _floor((*p + halfsize)/size);
  *p = mod_f(*p + halfsize, size) - halfsize;
  return c;
}

// License: MIT, author: Inigo Quilez, found: https://www.iquilezles.org/www/articles/intersectors/intersectors.htm
__DEVICE__ float2 rayCylinder(float3 ro, float3 rd, float3 cb, float3 ca, float cr) {
  float3  oc = ro - cb;
  float card = dot(ca,rd);
  float caoc = dot(ca,oc);
  float a = 1.0f - card*card;
  float b = dot( oc, rd) - caoc*card;
  float c = dot( oc, oc) - caoc*caoc - cr*cr;
  float h = b*b - a*c;
  if( h<0.0f ) return to_float2_s(-1.0f); //no intersection
  h = _sqrtf(h);
  return to_float2(-b-h,-b+h)/a;
}

// License: Unknown, author: Unknown, found: shadertoy somewhere, don't remember where
__DEVICE__ float dfcos(float x) {
  return _sqrtf(x*x+1.0f)*0.8f-1.8f;
}

// License: Unknown, author: Unknown, found: shadertoy somewhere, don't remember where
__DEVICE__ float dfcos_f2(float2 p, float freq) {
  float x = p.x;
  float y = p.y;
  x *= freq;
    
  float x1 = _fabs(mod_f(x+PI,TAU)-PI);
  float x2 = _fabs(mod_f(x   ,TAU)-PI);
    
  float a = 0.18f*freq;
    
  x1 /= _fmaxf( y*a+1.0f-a,1.0f);
  x2 /= _fmaxf(-y*a+1.0f-a,1.0f);
  return (_mix(-dfcos(x2)-1.0f,dfcos(x1)+1.0f,clamp(y*0.5f+0.5f,0.0f,1.0f)))/_fmaxf(freq*0.8f,1.0f)+_fmaxf(_fabs(y)-1.0f,0.0f)*sign_f(y);
}

__DEVICE__ float3 skyColor(float3 ro, float3 rd, bool Neonlightstunnel, __TEXTURE2D__ iChannel0) {
  const float4 hsv2rgb_K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);	
	
  const float3 l = normalize(to_float3(0.0f, 0.0f, -1.0f));
  //const float3 baseCol = HSV2RGB(to_float3(0.6f, 0.95f, 0.0025f));
  
  float3 baseCol;
  
  if(Neonlightstunnel)
  {
	float vu = texture( iChannel0, to_float2(0.0f, 0.01f)).x ;
    baseCol = HSV2RGB(to_float3(vu*1.83f, 0.95f, 0.0025f));   // cycle hue
    baseCol *= vu;                                            // adapt size
  }
  else
  {
	baseCol = HSV2RGB(to_float3(0.6f, 0.95f, 0.0025f));
  }
  
  return baseCol/(1.00001f+dot(rd, l));
}

__DEVICE__ float3 color(float3 ww, float3 uu, float3 vv, float3 ro, float2 p, float iTime, bool Neonlightstunnel, __TEXTURE2D__ iChannel0) {
  const float rdd = 2.0f;
  const float mm = 4.0f;
  const float rep = 27.0f;

  float3 rd = normalize(-p.x*uu + p.y*vv + rdd*ww);
  
  float3 skyCol = skyColor(ro, rd, Neonlightstunnel, iChannel0);

  swi2S(rd,y,x, mul_f2_mat2(swi2(rd,y,x) , ROT(0.1f*TIME)));

  float3 col = skyCol;

  // I read somewhere that if you call atan in a shader you got no business writing shader code.
  //  I even call it in a loop :)
  float a = _atan2f(rd.y, rd.x);
  for(float i = 0.0f; i < mm; ++i) {
    float ma = a;
    float sz = rep+i*6.0f;
    float slices = TAU/sz; 
    float na = mod1(&ma, slices);

    float h1 = hash(na+13.0f*i+123.4f);
    float h2 = fract(h1*3677.0f);
    float h3 = fract(h1*8677.0f);

    float tr = _mix(0.5f, 3.0f, h1);
    float2 tc = rayCylinder(ro, rd, ro, to_float3(0.0f, 0.0f, 1.0f), tr);
    float3 tcp = ro + tc.y*rd;
	
	if (i == 2.0f && Neonlightstunnel) tcp *= 2.0f;// added: smaller blocks
	
    float2 tcp2 = to_float2(tcp.z, _atan2f(tcp.y, tcp.x));
  
    float zz = _mix(0.025f, 0.05f, _sqrtf(h1))*rep/sz;
    
	float tmp = tcp2.y;
	float tnpy = mod1(&tmp, slices);
	tcp2.y = tmp;
	
    float fo = smoothstep(0.5f*slices, 0.25f*slices, _fabs(tcp2.y));
    
	float3 bcol;
	
	if(Neonlightstunnel)
	{
		tcp2.x += -h2*TIME * 3.0f;

		tcp2.y *= tr*PI/4.0f;  // width of the eels
    
		float vu2 = (i == 3.0f) ? 0.8f - ((texture( iChannel0, to_float2(0.0f, 0.8f)).x ))*0.7f : 1.0f;
		tcp2.y*= vu2;
		
		tcp2/=zz;
		float d = dfcos_f2(tcp2, 2.0f*zz);   // wiggle freq
		//    float d = tcp2.y;              // straight rays instead of wiggles
		d = _fabs(d);
		
		
		float vu3 = (i == 2.0f) ? 0.3f - ((texture( iChannel0, to_float2(0.0f, 0.3f)).x ))*0.5f : 1.0f;
		d *= zz*vu3;// "smaller" -> blur more

		bcol = (1.0f+cos_f3(to_float3(0.0f, 1.0f, 2.0f)+TAU*h3+0.5f*h2*h2*tcp.z))*0.00005f;
		bcol /= _fmaxf(d*d, 0.000f+5E-7*tc.y*tc.y);
		
		
		bcol *= _expf(-0.04f*tc.y*tc.y);    
		//    bcol *= smoothstep(-0.5f, 1.0f, _sinf(_mix(0.125f, 1.0f, h2)*tcp.z) );
		bcol *= smoothstep(-1.0f, 2.0f, _sinf(_mix(0.125f, 1.0f, h2)*tcp.z) );
	 
		float dz = (i == 1.0f) ? texture( iChannel0, to_float2(0.0f, 0.1f)).x * 5.0f : 0.0f; // displace "eel"
		bcol *= smoothstep(-0.5f, 1.0f, _sinf(_mix(0.125f, 1.0f, h2*6.0f)*(tcp.z + dz)));

		bcol *= fo;// not much difference
	}
	else
	{
		tcp2.x += -h2*TIME;
		tcp2.y *= tr*PI/3.0f;
		
		tcp2/=zz;
		float d = dfcos_f2(tcp2, 2.0f*zz);
		//    float d = tcp2.y;
		d = _fabs(d);
		d *= zz;

		bcol = (1.0f+cos_f3(to_float3(0.0f, 1.0f, 2.0f)+TAU*h3+0.5f*h2*h2*tcp.z))*0.00005f;
		bcol /= _fmaxf(d*d, 0.000f+5E-7f*tc.y*tc.y);
		bcol *= _expf(-0.04f*tc.y*tc.y);
		bcol *= smoothstep(-0.5f, 1.0f, _sinf(_mix(0.125f, 1.0f, h2)*tcp.z));
		bcol *= fo;
	}

    col += bcol;
  }

  return col;
}

// License: Unknown, author: nmz (twitter: @stormoid), found: https://www.shadertoy.com/view/NdfyRM
__DEVICE__ float3 sRGB(float3 t) {
  return _mix(1.055f*pow_f3(t, to_float3_s(1.0f/2.4f)) - 0.055f, 12.92f*t, step(t, to_float3_s(0.0031308f)));
}

// License: Unknown, author: Matt Taylor (https://github.com/64), found: https://64.github.io/tonemapping/
__DEVICE__ float3 aces_approx(float3 v) {
  v = _fmaxf(v, to_float3_s(0.0f));
  v *= 0.6f;
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;
  return clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);
}

__DEVICE__ float3 effect(float2 p, float2 pp, float iTime, bool Neonlightstunnel, float Zoom, float2 iMouse, __TEXTURE2D__ iChannel0) {
  float tm = 1.5f*TIME+12.3f;
  float3 ro   = to_float3(Neonlightstunnel?1.0f:0.0f, 0.0f, tm);
  float3 dro  = normalize(to_float3(1.0f, 0.0f, 3.0f) + to_float3(iMouse.x,iMouse.y,Zoom));
  
  if(Neonlightstunnel)   tm*=5.0f; // added: move "sun" faster
  
  swi2S(dro,x,z, mul_f2_mat2(swi2(dro,x,z) , ROT(0.2f*_sinf(0.05f*tm))));
  swi2S(dro,y,z, mul_f2_mat2(swi2(dro,y,z) , ROT(0.2f*_sinf(0.05f*tm*_sqrtf(0.5f)))));
  const float3 up = to_float3(0.0f,1.0f,0.0f);
  float3 ww = normalize(dro);
  float3 uu = normalize(cross(up, ww));
  float3 vv = (cross(ww, uu));
  float3 col = color(ww, uu, vv, ro, p, iTime, Neonlightstunnel, iChannel0);
  col -= 0.125f*swi3(to_float3(0.0f, 1.0f, 2.0f),y,z,x)*length(pp);
  col = aces_approx(col);
  col = sRGB(col);
  return col;
}

__KERNEL__ void ElectricEelUniverseFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
	
  CONNECT_CHECKBOX0(Neonlightstunnel, 0);
  CONNECT_SLIDER0(Zoom, -50.0f, 50.0f, 0.0f);  
	
  mat2 dummy;

  float2 q = fragCoord/swi2(RESOLUTION,x,y);
  float2 p = -1.0f + 2.0f * q;
  float2 pp = p;
  p.x *= RESOLUTION.x/RESOLUTION.y;

  float3 col = effect(p, pp, iTime, Neonlightstunnel, Zoom, (swi2(iMouse,x,y)/iResolution)-0.5f, iChannel0);
  fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float hash13( const in float3 p ) {
  float h = dot(p,to_float3(127.1f,311.7f,758.5453123f));  
  return fract(_sinf(h)*43758.5453123f);
}


// [iq] https://www.shadertoy.com/view/llGSzw
__DEVICE__ float3 hash3( uint n ) 
{
  // integer hash copied from Hugo Elias
  n = (n << 13U) ^ n;
  n = n * (n * n * 15731U + 789221U) + 1376312589U;
  uint3 k = n * make_uint3(n,n*16807U,n*48271U);
  
  return to_float3( (float)(k.x & 0x7fffffffU),(float)(k.y & 0x7fffffffU),(float)(k.z & 0x7fffffffU))/(float)(0x7fffffff);
}

__DEVICE__ float hash1( uint n ) 
{
  // integer hash copied from Hugo Elias
  n = (n << 13U) ^ n;
  n = n * (n * n * 15731U + 789221U) + 1376312589U;
  return (float)( n & 0x7fffffffU)/(float)(0x7fffffff);
}


// --------------------------------------------------------
// [iq] https://iquilezles.org/articles/distfunctions
// --------------------------------------------------------

__DEVICE__ float opExtrussion( in float3 p, in float sdf, in float h) {
  float2 w = to_float2(sdf, _fabs(p.z) - h);
  return _fminf(_fmaxf(w.x,w.y),0.0f) + length(_fmaxf(w,to_float2_s(0.0f)));
}

__DEVICE__ float sdCircle( in float2 p, in float2 w) {
  float d = length(p)- w.x;
  return _fmaxf(d, -w.y-d);
}

__DEVICE__ float sdRoundedX( in float2 p, in float w, in float r ) {
  p = abs_f2(p);
  return length(p-_fminf(p.x+p.y,w)*0.5f) - r;
}

__DEVICE__ float sdVerticalCapsule( float3 p, float h, float r ) {
  p.y -= clamp( p.y, 0.0f, h );
  return length( p ) - r;
}

__DEVICE__ float sdBox( in float2 p, in float2 b ) {
    float2 d = abs_f2(p)-b;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(max(d.x,d.y),0.0f);
}

__DEVICE__ float sdBox2( in float2 p, in float2 b ) {
  float d = sdBox(p, to_float2_s(0.8f*b.x))-0.01f;
  return _fmaxf(d, -b.y-d);
}

__DEVICE__ float sdBox3( float3 p, float3 b ) {
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Abstract 2' to iChannel1
// Connect Buffer A 'Texture: Font 1' to iChannel2


// Add second rotation to [iq]  https://www.shadertoy.com/view/3ld3DM
// See also               [dr2] https://www.shadertoy.com/view/3l3GD7

// -------------------------------
// Choose shapes
// -------------------------------

//#define SD2D(uv,w) sdBox(uv, w)
#define SD2D(uv,w) sdRoundedX(uv, w.x, w.y)
//#define SD2D(uv,w) sdCircle(uv, w)
//#define SD2D(uv,w) sdBox2(uv, w)

//#define SD3D(uv,w) sdBox3(uv, to_float3(w.x,h,w.x))
#define SD3D(uv,w) 0.8f*sdMessage3D(-1.0f*swi3(uv,y,z,x)+to_float3(l+0.24f,0,0), txt,0.5f,w.x,iChannel2)

// -------------------------------
// Render options
// -------------------------------

//#define WITH_TEXTURE
//#define WITH_BUMPMAP
//#define WITH_GRIB
//#define WITH_AA

// -------------------------------
#define GROUND 0
#define BIDULE 1
// -------------------------------

__DEVICE__ mat2 rot(float a) {
  float ca = _cosf(a), sa = _sinf(a);
  return to_mat2( ca, -sa, sa, ca );
}


// --------------------------------------------------------
// Inspired by [iq] https://www.shadertoy.com/view/3ld3DM
// --------------------------------------------------------

__DEVICE__ float3 opCurveSpace( in float3 p, in float h, in float3 r, out float2 *q, out float *ra) {
    float s = sign_f(r.x);
    if (s*r.x<0.001f) r.x = 0.001f;
    if (_fabs(r.y)<0.001f) r.y = 0.001f;
    if (_fabs(r.z)<0.001f) r.z = 0.001f;
    float2 sc = to_float2(_sinf(r.x),_cosf(r.x)); // could de precalculated
    mat2 rot2 = rot(r.y);            // could de precalculated
    *ra = 0.5f*h/r.x;                // Distance
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , rot2));          // Apply 2nd rotation
    p.x -= *ra;                      // Recenter
    *q = swi2(p,x,y) - 2.0f*sc*_fmaxf(0.0f,dot(sc,swi2(p,x,y)));  // Reflect
    float3 uvw = to_float3(*ra-length(*q)*s,         // New space coordinates 
                           *ra*_atan2f(s*p.y,-s*p.x),
                           p.z);
    swi2S(uvw,z,x, mul_f2_mat2(swi2(uvw,z,x) , rot(r.y+r.z*(uvw.y/h))));        // Inverse 2nd rotation
    return uvw;
}


// -- Text ------------------------------------------------
// Adapted from [FabriceNeyret2] https://www.shadertoy.com/view/llyXRW
// --------------------------------------------------------


__DEVICE__ float sdFont(in float2 p, in int c, __TEXTURE2D__ iChannel2) {
    float2 uv = (p + to_float2((float)(c%16), (float)(15-c/16)) + 0.5f)/16.0f;
    return _fmaxf(_fmaxf(_fabs(p.x) - 0.25f, _fmaxf(p.y - 0.35f, -0.38f - p.y)), texture(iChannel2, uv).w - 127.0f/255.0f);
}

__DEVICE__ float sdMessage2D(in float2 p, in int txt[6], __TEXTURE2D__ iChannel2) { 
    float d = 99.0f, w = 0.45f; // letter width  
    for (int id = 0; id<6; id++){
      d = _fminf(d, sdFont(p, txt[id],iChannel2));   
      p.x -= w; 
    }
    return d-0.015f; //0.015
}

__DEVICE__ float sdMessage3D(in float3 p, in int txt[6], in float scale, in float h, __TEXTURE2D__ iChannel2) { 
    return opExtrussion(p, sdMessage2D(swi2(p,x,y)/scale, txt, iChannel2)*scale, h);
}

// Based on [iq] https://www.shadertoy.com/view/3ld3DM
__DEVICE__ float4 sdJoint3D( in float3 p, in float l, in float3 rot, in float2 w,in int txt[6], __TEXTURE2D__ iChannel2) {
    float2 q; float ra;
    float3 uvw = opCurveSpace(p, l, rot, &q, &ra);

#ifdef SD3D   
    float d = SD3D(uvw, w);
#else // 
    // 2D Profile
    float ww = 1.2f*_fmaxf(w.x,w.y);
    float dTop = length(to_float2(q.x+ra-clamp(q.x+ra,-ww,ww), q.y))*sign_f(-q.y);
   
    // Profile  
    float d = SD2D(swi2(uvw,x,z), w);
  d = _fmaxf(dTop, d);
#endif
    return to_float4(d, uvw.x,uvw.y,uvw.z );
}

__DEVICE__ float4 sdJoint3DSphere( in float3 p, in float h, in float3 rot, in float w) {   
    float2 q; float ra; // only use in 2D
    float3 uvw = opCurveSpace(p, h, rot, &q, &ra);
    float d = sdVerticalCapsule(uvw, h, w);
   return to_float4(d, uvw.x,uvw.y,uvw.z );
}

// --------------------------------------------------------
//   The Scene
// --------------------------------------------------------

__DEVICE__ float4 map4( in float3 pos, float iTime , __TEXTURE2D__ iChannel2, float4 Txt1, float4 Txt2) {
  
    //int[] gtxt = int[] (83,80,65,67,69);
    int gtxt1[] = {32,74,105,80,105,32};
    int gtxt2[] = {110,109,98,114,55,51};

  
  
  
    float a = 1.0f*_sinf(iTime*1.5f), b = 1.3f*_sinf(iTime*2.5f);
    //vec4 d1 = sdJoint3DSphere(pos-to_float3(0.0f,0.0f, 0.4f), 0.8f, to_float3(b,a,a*b), 0.2f ),
//    float4 d1 = sdJoint3D(pos-to_float3(0.0f,0.0f, 0.4f), 1.1f, to_float3(b,a,a*b),to_float2(0.2f,0.04f), gtxt1,iChannel2 ),
//           d2 = sdJoint3D(pos-to_float3(0.0f,0.0f,-0.4f), 1.1f, to_float3(a,b,1.5f*_sinf(a+iTime*2.1f)), to_float2(0.2f,0.04f), gtxt2,iChannel2 );
    
    float4 d1 = sdJoint3D(pos-to_float3(0.0f,0.0f, 0.4f)+swi3(Txt1,x,y,z), 1.1f, to_float3(b,a,a*b),to_float2(0.2f+Txt1.w,0.04f), gtxt1,iChannel2 ),
           d2 = sdJoint3D(pos-to_float3(0.0f,0.0f,-0.4f)+swi3(Txt2,x,y,z), 1.1f, to_float3(a,b,1.5f*_sinf(a+iTime*2.1f)), to_float2(0.2f+Txt2.w,0.04f), gtxt2,iChannel2 );


    
    d1.w += 0.4f;
    d2.w -= 0.4f;
    return d1.x<d2.x ? d1 : d2; // Without ground
}

__DEVICE__ float map(in float3 p, float iTime, __TEXTURE2D__ iChannel2, float4 Txt1, float4 Txt2) {
    return _fminf(map4(p,iTime,iChannel2,Txt1,Txt2).x, p.y); // Distance with ground
}

// --------------------------------------
// Shading Tools
// --------------------------------------

__DEVICE__ float3 normal(in float3 p, in float3 ray, in float t, float iTime, float2 iResolution, __TEXTURE2D__ iChannel2, float4 Txt1, float4 Txt2) {
  float pitch = 0.4f * t / iResolution.x;
  float2 d = to_float2(-1,1) * pitch;
  float3 p0 = p+swi3(d,x,x,x), p1 = p+swi3(d,x,y,y), p2 = p+swi3(d,y,x,y), p3 = p+swi3(d,y,y,x);
  float f0 = map(p0,iTime,iChannel2,Txt1,Txt2), f1 = map(p1,iTime,iChannel2,Txt1,Txt2), f2 = map(p2,iTime,iChannel2,Txt1,Txt2), f3 = map(p3,iTime,iChannel2,Txt1,Txt2);
  float3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - p*(f0+f1+f2+f3);
  
  return normalize(grad - _fmaxf(0.0f, dot(grad,ray))*ray);
}

__DEVICE__ float SoftShadow(in float3 ro, in float3 rd, float iTime, __TEXTURE2D__ iChannel2, float4 Txt1, float4 Txt2) {
    float r = 1.0f, h, t = 0.005f+hash13(ro)*0.02f, dt = 0.01f;
    for(int i=0; i<48; i++ ) {
        h = map4(ro + rd*t,iTime,iChannel2,Txt1,Txt2).x;
        r = _fminf(r, 3.0f*h/t);
        t += dt;
        dt += 0.0015f;
        if (h<1e-4) break;
    }
    return clamp(r, 0.0f, 1.0f);
}

__DEVICE__ float CalcAO(in float3 p, in float3 n, float iTime, __TEXTURE2D__ iChannel2, float4 Txt1, float4 Txt2) {
    float d, h=0.01f, a=0.0f, s=1.0f;
    for(int i=0; i<4; i++) {
        d = map(n * h + p,iTime,iChannel2,Txt1,Txt2);
        a += (h-d)*s;
        s *= 0.8f;
        h += 0.03f;
    }
    return clamp(1.0f-4.0f*a, 0.0f, 1.0f);
}

__DEVICE__ float isGridLine(float3 p, float3 v) {
    float3 k = smoothstep(to_float3_s(0.2f),to_float3_s(0.8f),abs_f3(mod_f3f3(p+v*0.5f, v)-v*0.5f)/0.01f);
    return k.x * k.y * k.z;
}

// See https://iquilezles.org/articles/palettes for more information
__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d ) {
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

//#ifdef WITH_BUMPMAP
  //----------------------------------
  // Texture 3D
  //----------------------------------
  // Need to be in UVW space !
  __DEVICE__ float3 normalUVW(in float3 p, in float3 n, in float t, float iTime, float2 iResolution, __TEXTURE2D__ iChannel2, float4 Txt1, float4 Txt2) {   
    float pitch = 0.4f * t / iResolution.x;
    
    return normalize(swi3(map4(p+n*pitch,iTime,iChannel2,Txt1,Txt2),y,z,w) - swi3(map4(p,iTime,iChannel2,Txt1,Txt2),y,z,w));
  }

  // Tri-Planar blending function. Based on an old Nvidia writeup:
  // GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
  __DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){  
      n = _fmaxf(n*n, to_float3_s(0.001f));
      n /= n.x + n.y + n.z;  
    return swi3((texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z),x,y,z);
  }

  // Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
  // make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
  __DEVICE__ float3 doBumpMap( __TEXTURE2D__ tx, in float3 p, in float3 n, in float3 nUVW, float bf){   
      const float2 e = to_float2(0.001f, 0);
      // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
      mat3 m = to_mat3_f3( tex3D(tx, p - swi3(e,x,y,y), nUVW), tex3D(tx, p - swi3(e,y,x,y), nUVW), tex3D(tx, p - swi3(e,y,y,x), nUVW));
      float3 g = mul_f3_mat3(to_float3(0.299f, 0.587f, 0.114f),m); // Converting to greyscale.
      g = (g - dot(tex3D(tx,  p , nUVW), to_float3(0.299f, 0.587f, 0.114f)) )/e.x; 
      g -= nUVW*dot(nUVW, g);
      return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
  }
//#endif

//----------------------------------
// Shading
//----------------------------------

__DEVICE__ float3 render(in float3 ro, in float3 rd, in float res, in float3 pos, in float3 n, in float3 cobj, in float3 light, float iTime, __TEXTURE2D__ iChannel2, float4 Txt1, float4 Txt2) {
    float 
         ao = CalcAO(pos, n,iTime,iChannel2,Txt1,Txt2),
         sh = SoftShadow( pos, light,iTime,iChannel2,Txt1,Txt2),
         amb = clamp(0.5f+0.5f*n.y, 0.0f, 1.0f),
         dif = sh*clamp(dot( n, light ), 0.0f, 1.0f),
         pp = clamp(dot(reflect(-light,n), -rd),0.0f,1.0f),
         fre = (0.7f+0.3f*dif)* ao*_powf( clamp(1.0f+dot(n,rd),0.0f,1.0f), 2.0f);
    float3 brdf = ao*0.5f*(amb)+ sh*ao*1.0f*dif*to_float3(1.0f,0.9f,0.7f)*to_float3(1.0f,0.25f,0.05f),
         sp = sh*5.0f*_powf(pp,9.0f)*to_float3(1.0f, 0.6f, 0.2f),
        col = cobj*(brdf + sp) + fre*(0.5f*cobj+0.5f);
    return _mix(0.1f*to_float3(1.0f,1.0f,0.8f),col,2.0f*dot(n,-rd));
}

// --------------------------------------
// Main
// --------------------------------------

__DEVICE__ float4 mainImage2(float4 fragColor, in float2 fragCoord, float2 iResolution, float iTime, float4 iMouse, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3,
                             float3 View, float4 Txt1, float4 Txt2, bool Switches[4], float2 GrdScale  ) {

    float2 r = iResolution, 
           m = swi2(iMouse,x,y) / r,
           q = fragCoord/swi2(r,x,y), pix = q+q-1.0f;
           pix.x *= r.x/r.y;

    float anim = 0.1f*iTime,
          tTensionCol = smoothstep(0.8f,1.2f,anim),
          aCam = 10.0f + 4.0f*anim + 8.0f*m.x;
    
    // Camera
    float3 ro = 1.5f*to_float3(_cosf(aCam), 1.2f, 0.2f + _sinf(aCam))+View,
         w = normalize(to_float3(0,0.3f,0) - ro), 
         u = normalize(cross(w, to_float3(0,1,0))), 
         v = cross(u, w),
         rd = normalize(pix.x * u + pix.y * v + w+w);
      
      
    // Ground intersection (faster than ray marching)
    float tg = -ro.y/rd.y; 
    float tmax = _fminf(tg,5.5f);
  // Ray marching
    int obj = GROUND;
    float h = 0.1f, t = 0.01f*hash13(swi3(q,x,y,x));
    for(int i=0;i<200;i++) { 
        if (h<5.e-5 || t>tmax) break;
        t += h = map4(ro + rd*t,iTime,iChannel2,Txt1,Txt2).x;
    }
    if (h<5.e-5) {
        obj = BIDULE;
    } else {
        t = tg;
    }
    
  // Light
    float3 lightPos = to_float3(0.0f,1.2f, 0.7f);
            
    // Calculate color on point
    float3 pos = ro + t * rd;
    float3 n = obj == GROUND ? to_float3(0,1,0) : normal(pos,rd,t,iTime,iResolution,iChannel2,Txt1,Txt2);
    float3 uvw = obj == GROUND ? pos : swi3(map4(pos,iTime,iChannel2,Txt1,Txt2),y,z,w);     
    float3 cobj = to_float3_s(1.0f);
    float grib = 1.0f;
//#ifdef WITH_TEXTURE
if(Switches[0])
{
    float k = hash13(_floor((uvw+0.15f)/0.3f));
    cobj = pal(k, to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,1.0f,0.5f),to_float3(0.8f,0.90f,0.30f));
    cobj = _mix(to_float3_s(1), sqrt_f3(cobj), step(0.5f,k));
}
//#endif      
if(Switches[3])
{
    float3 tex = swi3(texture(iChannel3, swi2(uvw,x,z)/GrdScale),x,y,z);
    cobj = obj == GROUND ? tex : cobj;
}
//#ifdef WITH_GRIB
if(Switches[2])
{
    grib = isGridLine(uvw+0.15f, to_float3_s(0.3f));
    cobj = _mix(to_float3_s(0.1f),cobj,grib);
}
//#endif
//#ifdef WITH_BUMPMAP
if(Switches[1])
{
    float3 nuvw = obj == GROUND ? n : normalUVW(pos,n,t,iTime,iResolution,iChannel2,Txt1,Txt2);
    n = grib < 0.7f ? n : doBumpMap(iChannel1, uvw, n, nuvw, 0.003f);
    // keep in visible side
    n = normalize(n - _fmaxf(0.0f,dot(n,rd))*rd);
}
//#endif   

    // Shading
    float3 c = render(ro, rd, t, pos, n, cobj, normalize(lightPos-pos),iTime,iChannel2,Txt1,Txt2);

//#ifndef WITH_TEXTURE
if(Switches[0]==0)
{ 
 // Add light
    float3 col = _mix(to_float3(0.0f,0.25f,1.0f), to_float3(1.0f,0.25f,0.0f), smoothstep(-0.1f,0.1f,_cosf(0.5f*iTime)));
    c += 0.1f*(0.03f+col/_powf(0.1f+length(pos-to_float3(0,0.4f,0)),1.5f));
}
//#endif        

  fragColor = to_float4_aw(pow_f3(clamp(c,0.0f,1.0f), to_float3_s(0.43f)), t);  
  
  //fragColor = texture(iChannel2, fragCoord/iResolution);
  
  return fragColor;
}


// -- Anti aliasing ----------------------------------


__KERNEL__ void SpaceCurvatureJipiFuse__Buffer_A(float4 O, float2 U, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(WITH_AA, 0);
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f);
    CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);
    
    CONNECT_POINT1(Txt1XY, 0.0f, 0.0f);
    CONNECT_SLIDER1(Txt1Z, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Txt1W, -10.0f, 10.0f, 0.0f);
    
    CONNECT_POINT2(Txt2XY, 0.0f, 0.0f);
    CONNECT_SLIDER3(Txt2Z, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Txt2W, -10.0f, 10.0f, 0.0f);
    
    CONNECT_CHECKBOX2(WITH_TEXTURE, 1);
    CONNECT_CHECKBOX3(WITH_GRIB, 1);
    CONNECT_CHECKBOX4(WITH_BUMPMAP, 1);
    CONNECT_CHECKBOX5(WITH_GROUNDTEXTURE, 0);
    CONNECT_SLIDER5(Grd_Scale, -10.0f, 10.0f, 1.0f);
    
    float2 GrdScale = to_float2( iResolution.x/iResolution.y,1.0f) * Grd_Scale;
    
    bool Switches[4] = {WITH_TEXTURE,WITH_BUMPMAP,WITH_GRIB,WITH_GROUNDTEXTURE};
    float3 View = to_float3_aw(ViewXY, ViewZ);
    
    float4 Txt1 = to_float4(Txt1XY.x,Txt1XY.y,Txt1Z,Txt1W);
    float4 Txt2 = to_float4(Txt2XY.x,Txt2XY.y,Txt2Z,Txt2W);
    
    O = to_float4_s(0.0f);

    if(WITH_AA == 1)
    {      
      float4 T=O;                                     
      for (int k=0; k<4; k++, O+=T)               
          T = mainImage2(T, U+0.25f*to_float2(k%2-1,k/2-1),iResolution,iTime,iMouse,iChannel1,iChannel2,iChannel3, View,Txt1,Txt2,Switches,GrdScale);  
      O /= 4.0f;
    }  
    else
    {
      O = mainImage2(O, U,iResolution,iTime,iMouse,iChannel1,iChannel2,iChannel3, View,Txt1,Txt2,Switches,GrdScale);  
    }
    
    SetFragmentShaderComputedColor(O);
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Created by sebastien durand - 01/2019
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// *****************************************************************************
// Add 2 rotations to [iq]  https://www.shadertoy.com/view/3ld3DM
// See also           [dr2] https://www.shadertoy.com/view/3l3GD7
// *****************************************************************************

// Buf B: Calculate distance to scene
// Image: DOF post processing


//#define WITH_DOF
#define WITH_CONE_TEST

      
  __DEVICE__ bool inCone(float3 p, float3 o, float3 n, float side,float cosAngle) {
    return side*dot(normalize(o-p), n) >= cosAngle;
  }

  //--------------------------------------------------------------------------
  // eiffie's code for calculating the aperture size for a given distance...
  __DEVICE__ float coc(float t, float2 iResolution) {
    return _fmaxf(t*0.08f, (2.0f/iResolution.y) * (1.0f+t));
  }

  __DEVICE__ float3 RD(const float2 q, float2 iResolution) {
      return normalize(to_float3((2.0f* q.x - 1.0f) * iResolution.x/iResolution.y,  (2.0f* q.y - 1.0f), 2.0f));
  }

  __DEVICE__ float3 dof(__TEXTURE2D__ tex, float2 uv, float fdist, float2 iResolution) {
      
      
      const float aperture = 2.0f;

      const float cosAngle = _cosf(radians(aperture/2.0f));
      const float GA = 2.399f;  // golden angle = 2pi/(1+phi)
      const mat2 rot = to_mat2(_cosf(GA),_sinf(GA),-_sinf(GA),_cosf(GA));
      
      
      
      const float amount = 1.0f;
      float4 colMain = _tex2DVecN(tex,uv.x,uv.y,15);

      fdist = _fminf(30.0f, fdist);
      float rad = _fminf(0.3f, coc(_fabs(colMain.w-fdist),iResolution)),//0.3f; // TODO calculate this for Max distance on picture
            r = 6.0f;
      
      float3 cn = RD(uv,iResolution),    // Cone axis    
           co = cn*fdist,  // Cone origin
           sum = to_float3_s(0.0f),  
           bokeh = to_float3_s(1),
           acc = to_float3_s(0),
           pixPos;
      float2 pixScreen,
           pixel = 1.0f/iResolution,        
           angle = to_float2(0, rad);
      float4 pixCol;
      
      bool isInCone = false;
      for (int j=0;j<32;j++) {  
          r += 1.0f/r;
          angle = mul_f2_mat2(angle,rot);
          pixScreen = uv + pixel*(r-1.0f)*angle; // Neighbourg Pixel
          pixCol = _tex2DVecN(tex,pixScreen.x,pixScreen.y,15);    // Color of pixel (w is depth)      
          pixPos = pixCol.w * RD(pixScreen,iResolution);   // Position of 3D point in camera base
  #ifdef WITH_CONE_TEST
          if (inCone(pixPos, co, cn, sign_f(fdist - pixCol.w),cosAngle)) 
  #endif            
          {   // true if the point is effectivelly in the cone
              bokeh = pow_f3(swi3(pixCol,x,y,z), to_float3_s(9.0f)) * amount + 0.1f;
              acc += swi3(pixCol,x,y,z) * bokeh;      
              sum += bokeh;
              isInCone = true;
          }
    }
          
     return (!isInCone) ? swi3(colMain,x,y,z) : // Enable to deal with problem of precision when at thin begining of the cone
         swi3(acc,x,y,z)/sum;
  }


  __KERNEL__ void SpaceCurvatureJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
  {
      CONNECT_CHECKBOX1(WITH_DOF, 1);

      float3 c;

      if(WITH_DOF == 1)
      { 

        float2 r = iResolution, m = swi2(iMouse,x,y) / r,
               q = fragCoord/r;
        
        // Animation
        float anim = 0.1f*iTime,
             aCam = 10.0f + 4.0f*anim + 8.0f*m.x;

        // Camera
        float3 ro = 1.5f*to_float3(_cosf(aCam), 1.2f, 0.2f + _sinf(aCam));
          
        // DOF
        float fdist = length(ro-to_float3(0,0.3f,0));
        c = dof(iChannel0,q,fdist,iResolution); 
        
        // Vigneting
        c *= _powf(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.32f); 

      }
      else
      {        
        float2 uv = fragCoord / iResolution;
        c = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
        c *= _powf(16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y), 0.5f); // Vigneting
        //fragColor = c; //*0.01f; 
      }        
      //fragColor = texture(iChannel2, U/iResolution);
      fragColor = to_float4_aw(c,1.0f);
      
      SetFragmentShaderComputedColor(fragColor);
  }

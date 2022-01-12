
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Preset: Keyboard' to iChannel0


// rainbow spaghetti by mattz
//
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// Some code from iq's raymarching primer: https://www.shadertoy.com/view/Xds3zN

// const float i3 = 0.5773502691896258f;


//const float trad = 0.06f;
#define trad 0.06f

#define VEL 0.2f*to_float3(1.0f, 1.0f, 0.0f)





__DEVICE__ float hash(in float3 x) {
  return fract(87.3f*dot(x, to_float3(0.1f, 0.9f, 0.7f)));
}

__DEVICE__ float line(in float3 p0, in float3 p1, in float3 p) {

  const float lrad = 0.015f;

  float3 dp0 = p-p0;
  float3 d10 = p1-p0;

  float u = clamp(dot(dp0, d10)/dot(d10, d10), -5.0f, 5.0f);
  return distance(_mix(p0, p1, u), p)-0.5f*lrad;

}

__DEVICE__ float2 opU(float2 a, float2 b) {
  return a.x < b.x ? a : b;
}

__DEVICE__ float hueOf(float3 pos) {
  return _cosf( 2.0f*dot(2.0f*pos, to_float3(0.3f, 0.7f, 0.4f)) ) * 0.49f + 0.5f;
}

__DEVICE__ float3 round2(in float3 x, in float3 a) {
  return 2.0f * _floor( 0.5f * (x + 1.0f - a) ) + a;
}

__DEVICE__ float4 pdist(float3 p, float3 q) {
  float3 pq = p-q;
  return to_float4_aw(q, dot(pq,pq));
}

__DEVICE__ float4 pselect(float4 a, float4 b) {
  return a.w < b.w ? a : b;
}

__DEVICE__ float torus(in float3 a, in float3 b, in float3 pos) {
  const float r = 0.40824829046386302f;

  pos -= 0.5f*(a+b);
  float3 n = normalize(b-a);
  return distance(pos, r*normalize(pos - n*dot(n, pos))) - trad;
}

__DEVICE__ mat4 permute(float3 e, float3 f, float3 g, float3 h, float p) {
const float i = 0.3333333333333333f;
const float j = 0.6666666666666666f;

  return (p < i ? mat4(to_float4_aw(e,1.0f), to_float4_aw(f,1.0f), to_float4_aw(g, 1.0f), to_float4_aw(h, 1.0f)) :
      (p < j ? mat4(to_float4_aw(e,1.0f), to_float4_aw(g,1.0f), to_float4_aw(f, 1.0f), to_float4_aw(h, 1.0f)) :
       mat4(to_float4_aw(e,1.0f), to_float4_aw(h,1.0f), to_float4_aw(f, 1.0f), to_float4_aw(g, 1.0f))));
}

__DEVICE__ float3 randomBasis(float p) {
const float i = 0.3333333333333333f;
const float j = 0.6666666666666666f;
  return (p < i ? to_float3(1.0f, 0.0f, 0.0f) :
      p < j ? to_float3(0.0f, 1.0f, 0.0f) :
      to_float3(0.0f, 0.0f, 1.0f));
}

__DEVICE__ float3 randomPerp(float3 v, float p) {
  return (v.x>0.0f ? (p < 0.5f ? to_float3(0.0f, 1.0f, 0.0f) : to_float3(0.0f, 0.0f, 1.0f)) :
      v.y>0.0f ? (p < 0.5f ? to_float3(1.0f, 0.0f, 0.0f) : to_float3(0.0f, 0.0f, 1.0f)) :
      (p < 0.5f ? to_float3(1.0f, 0.0f, 0.0f) : to_float3(0.0f, 1.0f, 0.0f)));
}


__DEVICE__ float2 map(in float3 pos,float iTime, float foo) {

  const float wrap = 64.0f;
  const float i3 = 0.5773502691896258f;


  float3 orig = pos;

  pos = mod_f(pos + mod_f(iTime*VEL, wrap), wrap);

  // a, b, c, d are octahedron centers
  // d, e, f, g are tetrahedron vertices
  float3 a = round2(pos, to_float3_s(1.0f));
  float3 h = round2(pos, to_float3_s(0.0f));

  float3 b = to_float3(a.x, h.y, h.z);
  float3 c = to_float3(h.x, a.y, h.z);
  float3 d = to_float3(h.x, h.y, a.z);

  float3 e = to_float3(h.x, a.y, a.z);
  float3 f = to_float3(a.x, h.y, a.z);
  float3 g = to_float3(a.x, a.y, h.z);

  // o is the closest octahedron center
  float3 o = pselect(pselect(pdist(pos, a), pdist(pos, b)),
           pselect(pdist(pos, c), pdist(pos, d))).xyz;

  // t is the closest tetrahedron center
  float3 t = _floor(pos) + 0.5f;

  // normal points towards o
  // so bd is positive inside octahedron, negative inside tetrahedron
  float bd = dot(pos - swi3(o,x,y,z), (swi3(o,x,y,z)-swi3(t,x,y,z))*2.0f*i3) + i3;

  mat4 m = permute(e,f,g,h,hash(mod_f(t, wrap)));

  float t1 = torus(m[0].xyz, m[1].xyz, pos);
  float t2 = torus(m[2].xyz, m[3].xyz, pos);

  float p = hash(mod_f(o, wrap));
  float3 b1 = randomBasis(fract(85.17f*p));
  float3 b2 = randomPerp(b1, fract(63.61f*p+4.2f));
  float3 b3 = randomPerp(b1, fract(43.79f*p+8.3f));

  float3 po = pos-o;

  float o1 = torus( b1,  b2, po);
  float o2 = torus( b1, -b2, po);
  float o3 = torus(-b1,  b3, po);
  float o4 = torus(-b1, -b3, po);

  float2 noodle = to_float2(_fminf(max(bd, _fminf(t1,t2)),
               _fmaxf(-bd, _fminf(min(o1, o2), _fminf(o3, o4)))),
             hueOf(orig+0.5f*VEL*iTime));

  if (foo > 0.0f) {

    float dline = line(e, f, pos);
    dline = _fminf(dline, line(e, g, pos));
    dline = _fminf(dline, line(e, h, pos));
    dline = _fminf(dline, line(f, g, pos));
    dline = _fminf(dline, line(f, h, pos));
    dline = _fminf(dline, line(g, h, pos));

    float2 grid = to_float2(dline, 2.0f);

    noodle.x += 0.1f*trad;
    noodle.y = hash(mod_f(bd < 0.0f ? t : o, wrap));
    return opU(grid, noodle);

  } else {

    return noodle;

  }

}

__DEVICE__ float3 hue(float h) {

  float3 c = mod_f(h*6.0f + to_float3(2, 0, 4), 6.0f);
  return h > 1.0f ? to_float3_s(0.5f) : clamp(_fminf(c, -c+4.0f), 0.0f, 1.0f);
}

__DEVICE__ float2 castRay( in float3 ro, in float3 rd, in float maxd, float iTime, float foo )
{
  const int rayiter = 60;

  float precis = 0.0001f;
    float h=precis*2.0f;
    float t = 0.0f;
    float m = -1.0f;
    for( int i=0; i<rayiter; i++ )
    {
        if( _fabs(h)<precis||t>maxd ) continue;//break;
        t += h;
      float2 res = map( ro+rd*t ,iTime,foo);
        h = res.x;
      m = res.y;
    }

    return to_float2( t, m );
}

__DEVICE__ float3 calcNormal( in float3 pos, float iTime, float foo )
{
  float3 eps = to_float3( 0.0001f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(pos+swi3(eps,x,y,y),iTime,foo).x - map(pos-swi3(eps,x,y,y),iTime,foo).x,
      map(pos+swi3(eps,y,x,y),iTime,foo).x - map(pos-swi3(eps,y,x,y),iTime,foo).x,
      map(pos+swi3(eps,y,y,x),iTime,foo).x - map(pos-swi3(eps,y,y,x),iTime,foo).x );
  return normalize(nor);
}

__DEVICE__ float3 shade( in float3 ro, in float3 rd, float iTime,float foo ) {
  const float fogv = 0.025f;
  const float dmax = 20.0f;
  float3 L = normalize(to_float3(0.1f, 1.0f, 0.5f));

  float2 tm = castRay(ro, rd, dmax,iTime,foo);
  if (tm.y >= 0.0f) {
    float3 n = calcNormal(ro + tm.x * rd,iTime,foo);
    float fog = _expf(-tm.x*tm.x*fogv);
    float3 color = hue(tm.y) * 0.55f + 0.45f;
    float3 diffamb = (0.5f*dot(n,L)+0.5f) * color;
    float3 R = 2.0f*n*dot(n,L)-L;
    float spec = 0.2f*_powf(clamp(-dot(R, rd), 0.0f, 1.0f), 6.0f);
    return fog * (diffamb + spec);
  } else {
    return to_float3_s(1.0f);
  }
}

__KERNEL__ void RainbowSpaghettiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  const float3 axis = to_float3(1.0f, 1.0f, 0.0f);//to_float3(1.0f, 1.0f, 1.0f);
  const float3 tgt = to_float3(1.0f, 1.7f, 1.1f);//to_float3(-0.0f, 0.3f, -0.15f);
  const float3 cpos = tgt + axis;

  // const float3 vel = 0.2f*axis;

  const float KEY_G = 71.5f/256.0f;
  //float foo = texture(iChannel0, to_float2(KEY_G, 0.75f)).x
  float foo = _tex2DVecN(iChannel0, KEY_G, 0.75f, 15).x;


  const float yscl = 720.0f;
  const float f = 900.0f;

  float2 uv = (fragCoord - 0.5f*iResolution) * yscl / iResolution.y;

  float3 up = to_float3(0.0f, 1.0f, 0.0f);

  float3 rz = normalize(tgt - cpos);
  float3 rx = normalize(cross(rz,up));
  float3 ry = cross(rx,rz);

  float thetax = 0.0f;
  float thetay = 0.0f;

  if (_fmaxf(iMouse.x, iMouse.y) > 20.0f) {
    thetax = (iMouse.y - 0.5f*iResolution.y) * 3.14f/iResolution.y;
    thetay = (iMouse.x - 0.5f*iResolution.x) * -6.28f/iResolution.x;
  }

  float cx = _cosf(thetax);
  float sx = _sinf(thetax);
  float cy = _cosf(thetay);
  float sy = _sinf(thetay);

  mat3 Rx = mat3(1.0f, 0.0f, 0.0f,
           0.0f, cx, sx,
           0.0f, -sx, cx);

  mat3 Ry = mat3(cy, 0.0f, -sy,
           0.0f, 1.0f, 0.0f,
           sy, 0.0f, cy);

  mat3 R = mat3(rx,ry,rz);
  mat3 Rt = mat3(rx.x, ry.x, rz.x,
           rx.y, ry.y, rz.y,
           rx.z, ry.z, rz.z);

  float3 rd = R*Rx*Ry*normalize(to_float3_aw(uv, f));

  float3 ro = tgt + R*Rx*Ry*Rt*(cpos-tgt);

  fragColor = to_float4_aw(shade(ro, rd,iTime,foo), 1.0f);



  SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: St Peters Basilica Blurred_0' to iChannel0
// Connect Image 'Texture: Bild1' to iChannel1
// Connect Image 'Texture: Bild2' to iChannel2
// Connect Image 'Texture: Bild3' to iChannel3

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

const float dmax = 1000.0f;
const int rayiter = 60;

const float wrap = 64.0f;

//float3 L = normalize(to_float3(0.1f, 1.0f, 0.5f));

//const float3 tgt = to_float3_s(0.0f);
//const float3 cpos = to_float3(3.0f, -1.0f, 7.0f);

//float2 miss = to_float2(1e5, -1.0f);


__DEVICE__ float2 opU(float2 a, float2 b) {
  return a.x < b.x ? a : b;
}


__DEVICE__ float sdRect(in float3 pos, in float2 rmin, in float2 rmax) {
    float3 pc = to_float3_aw(clamp(swi2(pos,x,y), rmin, rmax), 0);
    return distance_f3(pos, pc);                   
}

__DEVICE__ float sdDisc(in float3 pos, in float r) {
    float l = length(swi2(pos,x,y));
    float3 pc = to_float3_aw(_fminf(l, r)*swi2(pos,x,y)/l, 0);
    return distance_f3(pos, pc);
}

__DEVICE__ float sdHeart(in float3 pos, in float r, in float d) {
    
    pos.x = _fabs(pos.x);
    swi2S(pos,x,y, _sqrtf(2.0f)*0.5f*mul_mat2_f2(to_mat2(1.0f,-1.0f,1.0f,1.0f),swi2(pos,x,y)));
        
    float ds = sdRect(pos, to_float2_s(-r+d), to_float2(r,r-d));
    float dc = sdDisc(pos-to_float3(r, 0, 0),r-d);
    
  return _fminf(ds, dc)-d;

}

__DEVICE__ float sdPlane(in float3 pos, float t) {
    return pos.x*_cosf(t) + pos.y*_sinf(t);
}
    

__DEVICE__ float2 map(in float3 pos, float iTime) {
  
    const float r = 1.5f;
    const float d = 0.9f;
    const float x = 0.05f;
    

    pos.y += 0.4f;
    
    float t = 2.0f*(iTime - 0.625f*_sinf(2.0f*iTime));
    
    float2 rval = to_float2(1e6, -1);
    
    for (float i=0.0f; i<6.0f; i++) {              
        
        float h1 = sdHeart(pos, r-(2.0f*i)*x, d-(2.0f*i)*x);
        float h2 = sdHeart(pos, r-(2.0f*i+1.0f)*x, d-(2.0f*i+1.0f)*x);
        float p = i<4.0f?sdPlane(pos, t) : -1e6;
        rval = opU(rval, to_float2(_fmaxf(_fmaxf(h1, -h2), p), 0.99f-0.08f*i));
        t *= -1.25f;

    }
    
    return rval;
}

__DEVICE__ float3 hue(float h) {
  
  float3 c = mod_f(h*6.0f + to_float3(2, 0, 4), 6.0f);
  return h >= 1.0f ? to_float3_s(h-1.0f) : clamp(_fminf(c, -c+4.0f), 0.0f, 1.0f);
}

__DEVICE__ float2 castRay( in float3 ro, in float3 rd, in float maxd, float iTime )
{
  float precis = 0.001f;
    float h=precis*2.0f;
    float t = 0.0f;
    float m = -1.0f;
    for( int i=0; i<rayiter; i++ )
    {
        if( _fabs(h)<precis||t>maxd ) continue;//break;
        t += h;
      float2 res = map( ro+rd*t, iTime );
        h = res.x;
      m = res.y;
    }
  if (t > maxd) { m = -1.0f; }
  return to_float2( t, m );
}

__DEVICE__ float3 calcNormal( in float3 pos, float iTime )
{
  float3 eps = to_float3( 0.001f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(pos+swi3(eps,x,y,y),iTime).x - map(pos-swi3(eps,x,y,y),iTime).x,
      map(pos+swi3(eps,y,x,y),iTime).x - map(pos-swi3(eps,y,x,y),iTime).x,
      map(pos+swi3(eps,y,y,x),iTime).x - map(pos-swi3(eps,y,y,x),iTime).x );
  return normalize(nor);
}

__DEVICE__ float3 shade( in float3 ro, in float3 rd, float iTime, bool TexOn, float Diffamb, float _R, float Spec, float Decube, float3 tmy, float ratio, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, __TEXTURE2D__ iChannel2, __TEXTURE2D__ iChannel3 ){
    
    float3 L = normalize(to_float3(0.1f, 1.0f, 0.5f));
  
    float3 c = to_float3_s(0.0f);
    float a = 1.0f;
    bool hit = true;
    
    for (int i=0; i<2; ++i) {
        
        if (hit) {

            float2 tm = castRay(ro, rd, dmax,iTime);
            float3 b;
            
            if (tm.y >= 0.0f) {
                float3 n = calcNormal(ro + tm.x * rd,iTime);
                float3 color = hue(tm.y) * 0.55f + 0.45f;
                
                if(TexOn)
                {
                  
                  float2 tuv = to_float2(rd.x*ratio,rd.y);
                  
                  
                  if (tm.y < 0.92+tmy.x) color = swi3(texture(iChannel1, tuv),x,y,z);
                  if (tm.y < 0.90+tmy.y) color = swi3(texture(iChannel2, tuv),x,y,z);
                  if (tm.y < 0.75+tmy.z) color = swi3(texture(iChannel3, tuv),x,y,z);
                }
                float3 diffamb = (Diffamb*dot(n,L)+0.2f) * color;
                float3 R = _R*n*dot(n,L)-L;
                float spec = Spec*_powf(clamp(-dot(R, rd), 0.0f, 1.0f), 20.0f);
                b = diffamb + spec;
                ro = ro + tm.x * rd;
                rd = reflect(rd, n);
                ro += 1e-4*rd;
                hit = true;
            } else {
                b = i>0 ? swi3(decube_f3(iChannel0,rd),x,y,z)*Decube : to_float3_s(1.0f);
                hit = false;
            }

            c = _mix(c, b, a);
            a *= 0.3f;
        }
    }
    
    return c;
}


__KERNEL__ void HeartU2015Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

  CONNECT_CHECKBOX0(TexOn, 0);
  CONNECT_COLOR0(Color, 0.0f, 0.0f, 0.0f, 1.0f);
  CONNECT_SLIDER0(yscl, -1.0f, 1000.0f, 720.0f);
  CONNECT_SLIDER1(f, -1.0f, 1500.0f, 900.0f);
  
  CONNECT_SLIDER2(Diffamb, -1.0f, 2.0f, 0.8f);
  CONNECT_SLIDER3(_R, -1.0f, 1500.0f, 2.0f);
  CONNECT_SLIDER4(Spec, -1.0f, 1500.0f, 0.5f);
  CONNECT_SLIDER5(Decube, -1.0f, 1500.0f, 2.5f);
  
  CONNECT_SLIDER6(tmy1, -1.0f, 1.0f, 0.0f);
  CONNECT_SLIDER7(tmy2, -1.0f, 1.0f, 0.0f);
  CONNECT_SLIDER8(tmy3, -1.0f, 1.0f, 0.0f);

  float3 tmy = to_float3(tmy1,tmy2,tmy3);
    
  const float3 tgt = to_float3_s(0.0f);
  const float3 cpos = to_float3(3.0f, -1.0f, 7.0f);

  float3 L = normalize(to_float3(0.1f, 1.0f, 0.5f));

  //const float yscl = 720.0f;
  //const float f = 900.0f;
  
  float ratio = iResolution.y/iResolution.x;
  
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
  
  mat3 Rx = to_mat3(1.0f, 0.0f, 0.0f, 
                    0.0f, cx, sx,
                    0.0f, -sx, cx);
  
  mat3 Ry = to_mat3(cy, 0.0f, -sy,
                    0.0f, 1.0f, 0.0f,
                    sy, 0.0f, cy);

  mat3 R = to_mat3_f3(rx,ry,rz);
  mat3 Rt = to_mat3(rx.x, ry.x, rz.x,
                    rx.y, ry.y, rz.y,
                    rx.z, ry.z, rz.z);

  float3 rd = mul_mat3_f3(mul_mat3_mat3(mul_mat3_mat3(R,Rx),Ry),normalize(to_float3_aw(uv, f)));
  
  float3 ro = tgt + mul_mat3_f3(mul_mat3_mat3(mul_mat3_mat3(mul_mat3_mat3(R,Rx),Ry),Rt),(cpos-tgt));
    
  L = mul_mat3_f3(mul_mat3_mat3(mul_mat3_mat3(mul_mat3_mat3(R,Rx),Ry),Rt),L);

  fragColor = to_float4_aw(shade(ro, rd, iTime, TexOn, Diffamb, _R, Spec, Decube, tmy, ratio, iChannel0, iChannel1, iChannel2, iChannel3), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
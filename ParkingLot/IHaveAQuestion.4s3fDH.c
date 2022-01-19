
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Cubemap: Forest' to iChannel0


#define pi 3.1415927
#define phi 1.61803398875
__DEVICE__ mat3 rot (float3 u) {
    float3 s = _sinf(u), c = _cosf(u);
    mat3 x = mat3(1,0,0,     0,c.x,s.x,     0,-s.x,c.x);
    mat3 y = mat3(c.y,0,s.y,   0,1,0,       -s.y,0,c.y);
    mat3 z = mat3(s.z,c.z,0,  -c.z,s.z,0,    0,0,1);
    return x*y*z;}
__DEVICE__ void tri (float3 p, float3 d, mat3 tr, inout float4* col, inout float* depth, inout float3* norm, __TEXTURE2D__ iChannel0) {
    float3 n = normalize(cross(tr[1]-tr[0],tr[2]-tr[0]));
    if (dot(n,-d)<0.0f) n *= -1.0f;
    mat3 nn = mat3(
      normalize(cross(n,tr[2]-tr[1])),
      normalize(cross(n,tr[0]-tr[2])),
      normalize(cross(n,tr[1]-tr[0]))
    );
    float3 w = p - tr[0];
    float x = -dot(w,n)/dot(d,n);
    if (x < 0.0f) return;
    float3 i = p + d*x;
    float3 ipol = to_float3(dot(nn[0],i-tr[1]),dot(nn[1],i-tr[2]),dot(nn[2],i-tr[0]))/to_float3(dot(nn[0],tr[0]-tr[1]),dot(nn[1],tr[1]-tr[2]),dot(nn[2],tr[2]-tr[0]));
    float4 c = to_float4_s(1.0f);
    c.w *= step(0.0f,ipol.x);
    c.w *= step(0.0f,ipol.y);
    c.w *= step(0.0f,ipol.z);
    if (c.w==0.0f) return;
    if (x < *depth) {
        float3 u = normalize(reflect(p-i,n));
//      swi3(c,x,y,z) = _tex2DVecN(iChannel0,u.x,u.y,15).xyz*_fabs(u)-1.0f+smoothstep(0.02f,0.025f,ipol.x)*smoothstep(0.02f,0.025f,ipol.y)*smoothstep(0.02f,0.025f,ipol.z);
        u = _tex2DVecN(iChannel0,u.x,u.y,15).xyz*_fabs(u)-1.0f+smoothstep(0.02f,0.025f,ipol.x)*smoothstep(0.02f,0.025f,ipol.y)*smoothstep(0.02f,0.025f,ipol.z);
        col->x=u.x;
        col->y=u.y;
        col->z=u.z;
        *depth = x;
        *norm = n;
    }
}
__DEVICE__ void sph (float3 p, float3 d, float4 cr, inout float4* col, inout float* depth, inout float3* norm, __TEXTURE2D__ iChannel0) {
  float3 w = p-swi3(cr,x,y,z);
    float B = 2.0f*dot(w,d);
    float C = dot(w,w)-cr.w*cr.w;
    float dl = B*B-4.0f*C;
    if (dl < 0.0f) return;
    float x = 0.5f*(-B-_sqrtf(dl));
    if (x < 0.0f) return;
    float3 i = p + d*x;
    //float4 c = to_float4_s(1.0f);
    if (x < *depth) {
        *norm = normalize(i-swi3(cr,x,y,z));
        float3 r = normalize(reflect(p-i,*norm));
      //swi3(c,x,y,z) = _fabs(r)*_tex2DVecN(iChannel0,r.x,r.y,15).xyz;
        r = _fabs(r)*_tex2DVecN(iChannel0,r.x,r.y,15).xyz;
        col->x=r.x;
        col->y=r.y;
        col->z=r.z;
       *depth = x;

    }
}

__KERNEL__ void IHaveAQuestionFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = ((fragCoord)/iResolution*2.0f-1.0f)*iResolution/swi2(iResolution,y,y);
  mat3 m = rot(to_float3(1.0f,2.0f,3.0f)*0.1f*iTime);
    float3 p = m*to_float3(0.0f,0.0f,-2.0f);
    float3 d = m*normalize(to_float3_aw(uv,2.0f));

    float4 col = to_float4(0.0f,0.0f,0.0f,0.0f);

    for (float i = 0.0f; i < 10.0f; i+=1.0f) {
      float depth = 1e3;
      float3 norm=to_float3_s(0.0f);
      float4 c = to_float4_s(0.0f);
      float t = iTime/pi;
      for (int i = 0; i < 4; i++) {
          tri(p,d,
              rot(pi*float(i)*to_float3(1.0f,0.0f,0.0f)*2.0f/4.0f)*mat3(
                  4.0f,0.0f,0.0f,
                  0.0f,4.0f,0.0f,
                  0.0f,0.0f,4.0f
              )
          ,&c,&depth,&norm,iChannel0);
        tri(p,d,
              rot(pi*float(i)*to_float3(1.0f,0.0f,0.0f)*2.0f/4.0f)*mat3(
                  -4.0f,0.0f,0.0f,
                  0.0f,-4.0f,0.0f,
                  0.0f,0.0f,-4.0f
              )
          ,&c,&depth,&norm,iChannel0);
          tri(p,d,
              rot(to_float3(t,0,0)+pi*float(i)*to_float3(1.0f,0.0f,0.0f)*2.0f/4.0f)*mat3(
                  1.0f,0.0f,0.0f,
                  0.0f,1.0f,0.0f,
                  0.0f,0.0f,1.0f
              )
          ,&c,&depth,&norm,iChannel0);
          tri(p,d,
              rot(to_float3(t,0,0)+pi*float(i)*to_float3(1.0f,0.0f,0.0f)*2.0f/4.0f)*mat3(
                  -1.0f,0.0f,0.0f,
                  0.0f,-1.0f,0.0f,
                  0.0f,0.0f,-1.0f
              )
          ,&c,&depth,&norm,iChannel0);

          sph (p,d,to_float4(_sinf(t+float(i)),_cosf(t+float(i)),_sinf(phi*t+float(i*i)),0.2f), &c, &depth, &norm,iChannel0);
      }


      p = p + d*depth;
      d = reflect(d,norm);
      p += 0.0001f*d;
      col += 0.6f*c/(1.0f+0.3f*i);
  }


    fragColor = col;


  SetFragmentShaderComputedColor(fragColor);
}
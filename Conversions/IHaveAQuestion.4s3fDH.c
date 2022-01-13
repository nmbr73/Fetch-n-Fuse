
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect '/media/a/94284d43be78f00eb6b298e6d78656a1b34e2b91b34940d02f1ca8b22310e8a0.png' to iChannel0


#define pi 3.1415927
#define phi 1.61803398875
__DEVICE__ mat3 rot (float3 u) {
    float3 s = _sinf(u), c = _cosf(u);
    mat3 x = mat3(1,0,0,     0,c.x,s.x,     0,-s.x,c.x);
    mat3 y = mat3(c.y,0,s.y,   0,1,0,       -s.y,0,c.y);
    mat3 z = mat3(s.z,c.z,0,  -c.z,s.z,0,    0,0,1);
    return x*y*z;}
__DEVICE__ void tri (float3 p, float3 d, mat3 tr, inout float4 col, inout float depth, inout float3 norm) {
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
    float3 ipol = to_float3_aw(dot(nn[0],i-tr[1]),dot(nn[1],i-tr[2]),dot(nn[2],i-tr[0]))/to_float3_aw(dot(nn[0],tr[0]-tr[1]),dot(nn[1],tr[1]-tr[2]),dot(nn[2],tr[2]-tr[0]));
    float4 c = to_float4_aw(1);
    c.w *= step(0.0f,ipol.x);
    c.w *= step(0.0f,ipol.y);
    c.w *= step(0.0f,ipol.z);
    if (c.w==0.0f) return;
    if (x < depth) {
        float3 u = normalize(reflect(p-i,n));
        swi3(c,x,y,z) = _tex2DVecN(iChannel0,u.x,u.y,15).xyz*_fabs(u)-1.0f+smoothstep(0.02f,0.025f,ipol.x)*smoothstep(0.02f,0.025f,ipol.y)*smoothstep(0.02f,0.025f,ipol.z);
        col=c;
        depth = x;
        norm = n;
    }
}
__DEVICE__ void sph (float3 p, float3 d, float4 cr, inout float4 col, inout float depth, inout float3 norm) {
  float3 w = p-swi3(cr,x,y,z);
    float B = 2.0f*dot(w,d);
    float C = dot(w,w)-cr.w*cr.w;
    float dl = B*B-4.0f*C;
    if (dl < 0.0f) return;
    float x = 0.5f*(-B-_sqrtf(dl));
    if (x < 0.0f) return;
    float3 i = p + d*x;
    float4 c = to_float4(1);
    if (x < depth) {
        norm = normalize(i-swi3(cr,x,y,z));
        float3 r = normalize(reflect(p-i,norm));
      swi3(c,x,y,z) = _fabs(r)*_tex2DVecN(iChannel0,r.x,r.y,15).xyz;
      col = c;
        depth = x;
        
    }  
}
__DEVICE__ void scene (inout float3 p, inout float3 d, inout float4 col, float i) {
  float depth = 1e3;
    float3 norm=to_float3(0);
    float4 c = to_float4(0);
    float t = iTime/pi;
    for (int i = 0; i < 4; i++) {
        tri(p,d,
            rot(pi*float(i)*to_float3(1,0,0)*2.0f/4.0f)*mat3(
                 4,0,0,
                0,4,0,
                0,0,4
            )
        ,c,depth,norm);
      tri(p,d,
            rot(pi*float(i)*to_float3(1,0,0)*2.0f/4.0f)*mat3(
                 -4,0,0,
                0,-4,0,
                0,0,-4
            )
        ,c,depth,norm);
        tri(p,d,
            rot(to_float3(t,0,0)+pi*float(i)*to_float3(1,0,0)*2.0f/4.0f)*mat3(
                 1,0,0,
                0,1,0,
                0,0,1
            )
        ,c,depth,norm);
        tri(p,d,
            rot(to_float3(t,0,0)+pi*float(i)*to_float3(1,0,0)*2.0f/4.0f)*mat3(
                 -1,0,0,
                0,-1,0,
                0,0,-1
            )
        ,c,depth,norm);
        
        sph (p,d,to_float4_aw(_sinf(t+float(i)),_cosf(t+float(i)),_sinf(phi*t+float(i*i)),0.2f), c, depth, norm);
    } 
    p = p + d*depth;
    d = reflect(d,norm);
    p += 0.0001f*d;
    col += 0.6f*c/(1.0f+0.3f*i);
}

__KERNEL__ void IHaveAQuestionFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = ((fragCoord)/iResolution*2.0f-1.0f)*iResolution/swi2(iResolution,y,y);
  mat3 m = rot(to_float3(1,2,3)*0.1f*iTime);
    float3 p = m*to_float3(0,0,-2);
    float3 d = m*normalize(to_float3_aw(uv,2));
    
    float4 col = to_float4(0,0,0,0);
    
    for (int i = 0; i < 10; i++) scene (p,d,col,float(i));
    
    
    fragColor = col;


  SetFragmentShaderComputedColor(fragColor);
}
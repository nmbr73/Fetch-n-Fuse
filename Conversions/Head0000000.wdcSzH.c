
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel0


#define M_NONE -1.0
#define M_NOISE 1.0

__DEVICE__ float hash(float h) {
  return fract_f(_sinf(h) * 43758.5453123f);
}

__DEVICE__ float2 hash( float2 p ){
  p = to_float2( dot(p,to_float2(127.1f,311.7f)),dot(p,to_float2(269.5f,183.3f)));
  return fract_f2(sin_f2(p)*43758.5453f);
}

__DEVICE__ float noise(float3 x) {
  float3 p = _floor(x);
  float3 f = fract_f3(x);
  f = f * f * (3.0f - 2.0f * f);

  float n = p.x + p.y * 157.0f + 113.0f * p.z;
  return _mix(
         _mix(_mix(hash(n + 0.0f), hash(n + 1.0f), f.x),
         _mix(hash(n + 157.0f), hash(n + 158.0f), f.x), f.y),
         _mix(_mix(hash(n + 113.0f), hash(n + 114.0f), f.x),
         _mix(hash(n + 270.0f), hash(n + 271.0f), f.x), f.y), f.z);
}

__DEVICE__ float3 random3(float3 c) {
  float j = 4096.0f*_sinf(dot(c,to_float3(17.0f, 59.4f, 15.0f)));
  float3 r;
  r.z = fract_f(512.0f*j);
  j *= 0.125f;
  r.x = fract_f(512.0f*j);
  j *= 0.125f;
  r.y = fract(512.0f*j);
  return r-0.5f;
}

#define OCTAVES 2
__DEVICE__ float fbm(float3 x) {
  float v = 0.0f;
  float a = 0.5f;
  float3 shift = to_float3_s(100);
  for (int i = 0; i < OCTAVES; ++i) {
    v += a * noise(x);
    x = x * 2.0f + shift;
    a *= 0.5f;
  }
  return v;
}

#define  MAX_MARCHING_STEPS  256
#define MAX_DIST  150.0f
#define EPSILON   0.001f

__DEVICE__ float sdBox( float3 p, float3 b )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f)-2.0f;
}

__DEVICE__ float sdTorus( float3 p, float2 t )
{
  float2 q = to_float2(length(swi2(p,x,z))-t.x,p.y);
  return length(q)-t.y;
}


__DEVICE__ float sdCapsule( float3 p, float h, float r )
{
    p.y -= clamp( p.y, 0.0f, h );
    return length( p ) - r;
}

__DEVICE__ float sdCone( in float3 p, in float h, in float r1, in float r2 )
{
    float2 q = to_float2( length(swi2(p,x,z)), p.y );
    
    float2 k1 = to_float2(r2,h);
    float2 k2 = to_float2(r2-r1,2.0f*h);
    float2 ca = to_float2(q.x-_fminf(q.x,(q.y<0.0f)?r1:r2), _fabs(q.y)-h);
    float2 cb = q - k1 + k2*clamp( dot((k1-q),k2)/dot(k2,k2), 0.0f, 1.0f );
    float s = (cb.x<0.0f && ca.y<0.0f) ? -1.0f : 1.0f;
    return s*_sqrtf( _fminf(dot(ca,ca),dot(cb,cb)) );
}

__DEVICE__ float opSmI( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) + k*h*(1.0f-h); 
}

__DEVICE__ float opSmU( float d1, float d2, float k ) {
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); }

__DEVICE__ float opSmS( float d1, float d2, float k ) {
    float h = clamp( 0.5f - 0.5f*(d2+d1)/k, 0.0f, 1.0f );
    return _mix( d2, -d1, h ) + k*h*(1.0f-h); }

__DEVICE__ mat3 rotateY(float t) {
  float c = _cosf(t);float s = _sinf(t);
  return to_mat3_f3( to_float3(c, 0, s),
                     to_float3(0, 1, 0),
                     to_float3(-s, 0, c)  );
}

__DEVICE__ mat3 rotateX(float t) {
    float c = _cosf(t);float s = _sinf(t);
  return to_mat3_f3( to_float3(1, 0, 0),
                     to_float3(0, c, -s),
                     to_float3(0, s, c)  );
}

__DEVICE__ mat3 rotateZ(float t) {
    float c = _cosf(t);float s = _sinf(t);
  return to_mat3_f3( to_float3(c, -s, 0),
                     to_float3(s, c, 0),
                     to_float3(0, 0, 1)  );
}

__DEVICE__ mat3 scale(float x,float y,float z){
    return to_mat3_f3( to_float3(x,0.0f,0.0f),
                       to_float3(0.0f,y,0.0f),
                       to_float3(0.0f,0.0f,z)  );
}

__DEVICE__ float head(float3 sP){
    float s1 = length(mul_f3_mat3(sP,scale(1.2f,1.0f,1.0f))-to_float3(0.0f,0.5f,0.0f))-0.5f;
    float s2 = length(mul_f3_mat3(sP,scale(1.2f,1.3f,1.2f))-to_float3(0.0f,0.12f,0.15f))-0.4f;
    float s3 = opSmU(s1,s2,0.2f);
    //float s5 = opSmU(s3,neck,0.1f);
    float s6 = sdCone(mul_f3_mat3( (sP-to_float3(0.0f,0.25f,0.34f)),rotateX(-2.1f) ),0.22f,0.2f,0.0f)-0.03f;
    float s7 = opSmU(s3,s6,0.1f);
    float s8 = length(sP-to_float3(0.0f,-0.08f,0.27f))-0.2f;
    float s9 = opSmU(s7,s8,0.1f);
    float s10 = length( mul_f3_mat3(mul_f3_mat3(mul_f3_mat3(mul_f3_mat3((to_float3(_fabs(sP.x),sP.y,sP.z)-to_float3(0.4f,0.25f,-0.03f)),rotateY(0.37f)),rotateZ(1341.6f)),rotateX(1149.15f)),scale(8.0f,1.0f,1.2f)))-0.12f;
    float s11 = opSmU(s9,s10,0.05f);
    float s12 = length(sP-to_float3(0.0f,-0.14f,0.56f))-0.09f;
    float s13 = opSmS(s12,s11,0.1f);
    return s13;
}

__DEVICE__ float displace(float3 sP,float geo, float iTime){
    float final;
    if(geo<=0.01f){
      float disp = geo-(0.1f*fbm(mul_f3_mat3(sP*3.0f,rotateY(iTime+12.0f*(0.3f*(_sinf(sP.y*1.0f+iTime*0.3f))+0.6f)))+to_float3(0.0f,0.0f,2.0f*iTime))-0.195f);
      final = opSmS(disp,geo,0.3f);
    }else{
        final = geo;
    }
    return final;
}


__DEVICE__ float sceneSDF(float3 sP, float iTime) {
    float3 sPh=mul_f3_mat3(mul_f3_mat3(mul_f3_mat3(sP,rotateY(noise(to_float3_s(iTime*0.5f))-0.5f)),rotateX(0.1f*noise(to_float3_s(iTime*0.5f+123.456f))-0.09f)),rotateZ(0.1f*noise(to_float3_s(iTime*0.5f+222.111f))-0.05f));
    
    float _head = head(sPh);

    float s4 = sdCapsule(sP-to_float3(0.0f,-0.5f,-0.15f),1.0f,0.26f);
    float s14 = length(sP-to_float3(0.0f,-0.7f,-0.18f))-0.4f;
    float s15 = opSmU(s4,s14,0.1f);
    float s16 = length(to_float3((sP.x),sP.y,sP.z)-to_float3(0.5f,-1.1f,-0.18f))-0.43f;
    float s17 = opSmU(s15,s16,0.5f);
    float s16n = length(to_float3(-(sP.x),sP.y,sP.z)-to_float3(0.5f,-1.1f,-0.18f))-0.43f;
    float s18 = opSmU(s17,s16n,0.5f);
    float torso = opSmU(s18,s4,0.1f);
    torso-=0.03f*noise(sP*8.0f);
    torso = displace(sP,torso,iTime);
    _head-=0.03f*noise(sPh*8.0f);
    _head = displace(sPh,_head,iTime);
    float final=opSmU(torso,_head,0.01f);

    return final;
}

__DEVICE__ float shortestDistanceToSurface(float3 eye, float3 marchingDirection, float iTime) {
    float depth;
    float dd=0.0f;
    for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
        float dist = (sceneSDF(eye + dd * marchingDirection,iTime));
        dd += dist;
        if (dist<EPSILON) {
            depth=dd;
        }else if(dist>MAX_DIST){
            return dd;
            break;
        }
        
    }
    return depth;
}
            
__DEVICE__ float3 rayDirection(float fieldOfView, float2 size, float2 fragCoord) {
    float2 xy = fragCoord - size / 2.0f;
    float z = size.y / _tanf(radians(fieldOfView) / 2.0f);
    return normalize(to_float3_aw(xy, -z));
}

__DEVICE__ float3 eN(float3 p, float iTime) {
    return normalize(to_float3(
        sceneSDF(to_float3(p.x + EPSILON, p.y, p.z),iTime) - sceneSDF(to_float3(p.x - EPSILON, p.y, p.z),iTime),
        sceneSDF(to_float3(p.x, p.y + EPSILON, p.z),iTime) - sceneSDF(to_float3(p.x, p.y - EPSILON, p.z),iTime),
        sceneSDF(to_float3(p.x, p.y, p.z  + EPSILON),iTime) - sceneSDF(to_float3(p.x, p.y, p.z - EPSILON),iTime)
    ));
}

__DEVICE__ float AO(float3 p,float3 n, float iTime){
    float ao =0.0f;
    float d;
    for(int i=1;i<=3;i++){
        d=3.0f*float(i);
        ao+=_fmaxf(0.0f,(d-sceneSDF(p+n*d,iTime))/d);
    }
    return ao;
}


__KERNEL__ void Head0000000Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float3 dir = rayDirection(45.0f, iResolution, fragCoord);
    float3 eye = to_float3(0.0f, 0.0f, 5.0f);
    //dir*=rotateY(iTime*0.5f);eye*=rotateY(iTime*0.5f);
    float3 color=to_float3_s(0.0f);
    float sdf = shortestDistanceToSurface(eye, dir,iTime);
    if(sdf>MAX_DIST || sdf==0.0f){
        fragColor = to_float4_s(0.0f);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    float dist = sdf;
    float3 p = eye + dist * dir;
    float3 N = eN(p,iTime);
    float occ = 1.0f-AO(p,N,iTime);

    float3 ref = (reflect(dir,N));
    color=(swi3(decube_f3(iChannel0,ref),x,y,z)*0.5f+to_float3_s(0.5f))*occ*(1.0f-smoothstep(4.5f,6.0f,dist));
    
        
    fragColor = to_float4_aw(color, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define MAX_STEPS 100
#define MIN_DISTANCE 0.01f
#define MAX_DISTANCE 100.0f
__DEVICE__ float hash(float n){
    return fract(_sinf(n)*43758.5453f);
}
struct Mesh
{
    float3 pos;
    float3 scale;
    float3 rotation;
};
__DEVICE__ float noise(in float3 x){
    float3 p=_floor(x);
    float3 f=fract_f3(x);
    
    f=f*f*(3.0f-2.0f*f);
    float n=p.x+p.y*57.0f+113.0f*p.z;
    return _mix(_mix(_mix(hash(n+0.0f),hash(n+1.0f),f.x),
                _mix(hash(n+57.0f),hash(n+58.0f),f.x),f.y),
           _mix(_mix(hash(n+113.0f),hash(n+114.0f),f.x),
                _mix(hash(n+170.0f),hash(n+171.0f),f.x),f.y),f.z);
}

__DEVICE__ float3 noise3(float3 x){
    return to_float3(noise(x+to_float3(123.456f,0.567f,0.37f)),
    noise(x+to_float3(0.11f,47.43f,19.17f)),
    noise(x));
}

// a sphere with a little bit of warp
__DEVICE__ float sdf(float3 p,float r, float iTime){
    float3 n=to_float3(_sinf(iTime),_sinf(iTime*0.6f),_cosf(iTime*0.4f));
    float3 q=0.2f*(noise3(p+n)-0.5f);
    
    return length(q+p)-r;
}



__DEVICE__ mat2 Rot(float a){
    float s=_sinf(a),c=_cosf(a);
    return to_mat2(c,-s,s,c);
}
__DEVICE__ float smin(float a,float b,float k)
{
    float h=_fmaxf(k-_fabs(a-b),0.0f)/k;
    return _fminf(a,b)-h*h*k*(1.0f/4.0f);
}
__DEVICE__ float3 transform(float3 p,Mesh mesh)
{
    float3 point=p-mesh.pos;
    swi2S(point,y,z, mul_f2_mat2(swi2(point,y,z),Rot(mesh.rotation.x)));
    swi2S(point,x,z, mul_f2_mat2(swi2(point,x,z),Rot(mesh.rotation.y)));
    swi2S(point,x,y, mul_f2_mat2(swi2(point,x,y),Rot(mesh.rotation.z)));
    point/=mesh.scale;
    return point;
}
__DEVICE__ float getDist(float3 p, float iTime, Mesh BUBBLE_MESH[4])
{
  
    float3 point=p;
    point=transform(p,BUBBLE_MESH[0]);
    float s1=sdf(point,0.3f,iTime);
    point=transform(p,BUBBLE_MESH[1]);
    float s2=sdf(point,0.5f, iTime);
    point=transform(p,BUBBLE_MESH[2]);
    float s3=sdf(point,1.0f,iTime);
    point=transform(p,BUBBLE_MESH[3]);
    float s4=sdf(point,0.7f,iTime);
    s1=smin(s1,s2,0.3f);
    s1=smin(s1,s3,0.3f);
    s1=smin(s1,s4,0.3f);
    return s1;
}
__DEVICE__ float RayMarch(float3 ro,float3 rd, float iTime, Mesh BUBBLE_MESH[4])
{
    float dist=0.0f;
    for(int i=0;i<MAX_STEPS;i++)
    {
        float d=getDist(ro+rd*dist,iTime, BUBBLE_MESH);
        dist+=d;
        if(d<MIN_DISTANCE||dist>MAX_DISTANCE)  break;
    }
    return dist;
}

__DEVICE__ float3 getNormal(float3 p, float iTime, Mesh BUBBLE_MESH[4])
{
    float d=getDist(p,iTime,BUBBLE_MESH);
    float2 e=to_float2(0.01f,0);
    return normalize(d-to_float3(getDist(p-swi3(e,x,y,y),iTime,BUBBLE_MESH),getDist(p-swi3(e,y,x,y),iTime,BUBBLE_MESH),getDist(p-swi3(e,y,y,x),iTime,BUBBLE_MESH)));
}

#ifdef xxx
__DEVICE__ void update()
{
    BUBBLE1_MESH.pos = to_float3(_cosf(iTime),_sinf(1.3f*iTime),4.0f+_sinf(iTime));
    BUBBLE2_MESH.pos = to_float3(0.0f,_cosf(iTime),4.0f+_sinf(1.3f*iTime));
    BUBBLE3_MESH.pos = to_float3(_cosf(1.3f*iTime),_sinf(iTime),4.2f);
    BUBBLE4_MESH.pos = to_float3(_sinf(iTime),1,4);
}
#endif


__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}



__DEVICE__ float3 refractTex(float3 camdir,float3 normal,float eta, __TEXTURE2D__ iChannel0, float refmul, float refoff)
{
    float3 rd = _refract_f3(camdir,normal,eta,refmul,refoff);
    float2 uv = swi2(rd,x,y)-0.5f;
    return swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);//.rgb;
}
__DEVICE__ float3 reflectTex(float3 camdir,float3 normal, __TEXTURE2D__ iChannel0)
{
    float3 rd=reflect(camdir,normal);
    float2 uv=0.5f-swi2(rd,x,y)*0.3f;
    return swi3(texture(iChannel0,uv),x,y,z);//.rgb;  // Lod 1.0f !!
}
__DEVICE__ float3 illuminate(in float3 pos,in float3 camdir, __TEXTURE2D__ iChannel0, float iTime, Mesh BUBBLE_MESH[4], float refmul, float refoff)
{
    float3 normal=getNormal(pos,iTime,BUBBLE_MESH);
    
    const float ETA=1.03f;
    float3 refrd=-1.0f*_refract_f3(camdir,normal,ETA,refmul,refoff);
    float3 refro=pos+10.0f*refrd;
    float refdist=RayMarch(refro,refrd,iTime,BUBBLE_MESH);
    float3 refpos=refro+refdist*refrd;
    float3 refnormal=getNormal(refpos,iTime,BUBBLE_MESH);
    
    float3 etaRatioRGB=to_float3(1.02f,1.04f,1.07f);
float zzzzzzzzzzzzzzzzzz;    
    float3 refracted_color;
    refracted_color.x = refractTex(camdir,normal,etaRatioRGB.x,iChannel0,refmul,refoff).x;
    refracted_color.y = refractTex(camdir,normal,etaRatioRGB.y,iChannel0,refmul,refoff).y;
    refracted_color.z = refractTex(camdir,normal,etaRatioRGB.z,iChannel0,refmul,refoff).z;
    float3 reflected_color = reflectTex(camdir,normal,iChannel0);
    float3 texture = 0.8f*refracted_color+0.2f*reflected_color;
    return texture;
}


__KERNEL__ void BubblemovingJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
  
    CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 1.0f); 
    CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.0f);

    Mesh BUBBLE_MESH[4]={{to_float3_s(0),to_float3_s(1),to_float3_s(0)},
                         {to_float3_s(0),to_float3_s(1),to_float3_s(0)},
                         {to_float3_s(0),to_float3_s(1),to_float3_s(0)},
                         {to_float3_s(0),to_float3_s(1),to_float3_s(0)}};

float IIIIIIIIIIIIIIIIIIII;
    float2 uv=(fragCoord-0.5f*iResolution)/iResolution.y;
    float3 ro=to_float3(0,0,0);
    // Normalized pixel coordinates (from 0 to 1)
    float3 rd=normalize(to_float3(uv.x,uv.y,1));
    
    BUBBLE_MESH[0].pos = to_float3(_cosf(iTime),_sinf(1.3f*iTime),4.0f+_sinf(iTime));
    BUBBLE_MESH[1].pos = to_float3(0.0f,_cosf(iTime),4.0f+_sinf(1.3f*iTime));
    BUBBLE_MESH[2].pos = to_float3(_cosf(1.3f*iTime),_sinf(iTime),4.2f);
    BUBBLE_MESH[3].pos = to_float3(_sinf(iTime),1,4);
    
    float3 col=to_float3_s(1);
    
    float dist=RayMarch(ro,rd,iTime,BUBBLE_MESH);
    if(dist>=MAX_DISTANCE)
    {
        float2 st=fragCoord/iResolution;
        col=swi3(_tex2DVecN(iChannel0,st.x,st.y,15),x,y,z);
    }else
    {
        float3 p=ro+rd*dist;
        col=illuminate(p,rd, iChannel0, iTime, BUBBLE_MESH,refmul,refoff);
    }
    
    // Output to screen
    fragColor=to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
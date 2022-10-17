

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_STEPS 100
#define MIN_DISTANCE .01
#define MAX_DISTANCE 100.
float hash(float n){
    return fract(sin(n)*43758.5453);
}
struct Mesh
{
    vec3 pos;
    vec3 scale;
    vec3 rotation;
};
float noise(in vec3 x){
    vec3 p=floor(x);
    vec3 f=fract(x);
    
    f=f*f*(3.-2.*f);
    float n=p.x+p.y*57.+113.*p.z;
    return mix(mix(mix(hash(n+0.),hash(n+1.),f.x),
    mix(hash(n+57.),hash(n+58.),f.x),f.y),
    mix(mix(hash(n+113.),hash(n+114.),f.x),
    mix(hash(n+170.),hash(n+171.),f.x),f.y),f.z);
}

vec3 noise3(vec3 x){
    return vec3(noise(x+vec3(123.456,.567,.37)),
    noise(x+vec3(.11,47.43,19.17)),
    noise(x));
}

// a sphere with a little bit of warp
float sdf(vec3 p,float r){
    vec3 n=vec3(sin(iTime),sin(iTime*.6),cos(iTime*.4));
    vec3 q=.2*(noise3(p+n)-.5);
    
    return length(q+p)-r;
}
Mesh BUBBLE1_MESH=Mesh(vec3(0),vec3(1),vec3(0));
Mesh BUBBLE2_MESH=Mesh(vec3(0),vec3(1),vec3(0));
Mesh BUBBLE3_MESH=Mesh(vec3(0),vec3(1),vec3(0));
Mesh BUBBLE4_MESH=Mesh(vec3(0),vec3(1),vec3(0));
mat2 Rot(float a){
    float s=sin(a),c=cos(a);
    return mat2(c,-s,s,c);
}
float smin(float a,float b,float k)
{
    float h=max(k-abs(a-b),0.)/k;
    return min(a,b)-h*h*k*(1./4.);
}
vec3 transform(vec3 p,Mesh mesh)
{
    vec3 point=p-mesh.pos;
    point.yz*=Rot(mesh.rotation.x);
    point.xz*=Rot(mesh.rotation.y);
    point.xy*=Rot(mesh.rotation.z);
    point/=mesh.scale;
    return point;
}
float getDist(vec3 p)
{
    vec3 point=p;
    point=transform(p,BUBBLE1_MESH);
    float s1=sdf(point,.3);
    point=transform(p,BUBBLE2_MESH);
    float s2=sdf(point,.5);
    point=transform(p,BUBBLE3_MESH);
    float s3=sdf(point,1.);
    point=transform(p,BUBBLE4_MESH);
    float s4=sdf(point,.7);
    s1=smin(s1,s2,.3);
    s1=smin(s1,s3,.3);
    s1=smin(s1,s4,.3);
    return s1;
}
float RayMarch(vec3 ro,vec3 rd)
{
    float dist=0.;
    for(int i=0;i<MAX_STEPS;i++)
    {
        float d=getDist(ro+rd*dist);
        dist+=d;
        if(d<MIN_DISTANCE||dist>MAX_DISTANCE)break;
    }
    return dist;
}

vec3 getNormal(vec3 p)
{
    float d=getDist(p);
    vec2 e=vec2(.01,0);
    return normalize(d-vec3(getDist(p-e.xyy),getDist(p-e.yxy),getDist(p-e.yyx)));
}
void update()
{
    BUBBLE1_MESH.pos=vec3(cos(iTime),sin(1.3*iTime),4.+sin(iTime));
    BUBBLE2_MESH.pos=vec3(0.,cos(iTime),4.+sin(1.3*iTime));
    BUBBLE3_MESH.pos=vec3(cos(1.3*iTime),sin(iTime),4.2);
    BUBBLE4_MESH.pos=vec3(sin(iTime),1,4);
}

vec3 refractTex(vec3 camdir,vec3 normal,float eta)
{
    vec3 rd=refract(camdir,normal,eta);
    vec2 uv=rd.xy-.5;
    return texture(iChannel0,uv).rgb;
}
vec3 reflectTex(vec3 camdir,vec3 normal)
{
    vec3 rd=reflect(camdir,normal);
    vec2 uv=.5-rd.xy*.3;
    return textureLod(iChannel0,uv,1.).rgb;
}
vec3 illuminate(in vec3 pos,in vec3 camdir)
{
    vec3 normal=getNormal(pos);
    
    const float ETA=1.03;
    vec3 refrd=-refract(camdir,normal,ETA);
    vec3 refro=pos+10.*refrd;
    float refdist=RayMarch(refro,refrd);
    vec3 refpos=refro+refdist*refrd;
    vec3 refnormal=getNormal(refpos);
    
    vec3 etaRatioRGB=vec3(1.02,1.04,1.07);
    
    vec3 refracted_color;
    refracted_color.r=refractTex(camdir,normal,etaRatioRGB.r).r;
    refracted_color.g=refractTex(camdir,normal,etaRatioRGB.g).g;
    refracted_color.b=refractTex(camdir,normal,etaRatioRGB.b).b;
    vec3 reflected_color=reflectTex(camdir,normal);
    vec3 texture=.8*refracted_color+.2*reflected_color;
    return texture;
}
void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    vec2 uv=(fragCoord-.5*iResolution.xy)/iResolution.y;
    vec3 ro=vec3(0,0,0);
    // Normalized pixel coordinates (from 0 to 1)
    vec3 rd=normalize(vec3(uv.x,uv.y,1));
    
    update();
    
    vec3 col=vec3(1);
    
    float dist=RayMarch(ro,rd);
    if(dist>=MAX_DISTANCE)
    {
        vec2 st=fragCoord/iResolution.xy;
        col=texture(iChannel0,st).rgb;
    }else
    {
        vec3 p=ro+rd*dist;
        col=illuminate(p,rd);
    }
    
    // Output to screen
    fragColor=vec4(col,1.);
}
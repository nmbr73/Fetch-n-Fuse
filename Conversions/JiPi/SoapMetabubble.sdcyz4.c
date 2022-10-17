
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Connect Image 'Texture: Font 1' to iChannel0

// raymarching based from https://www.shadertoy.com/view/wdGGz3
#define MAX_STEPS 256
#define MAX_DIST 256.0f
#define SURF_DIST 0.0005f
#define Rot(a) to_mat2(_cosf(a),-_sinf(a),_sinf(a),_cosf(a))
#define antialiasing(n) n/_fminf(iResolution.y,iResolution.x)
#define S(d,b) smoothstep(antialiasing(1.0f),b,d)
#define B(p,s) _fmaxf(_fabs((p).x)-s.x,_fabs((p).y)-s.y)
#define MATERIAL 0

#define ZERO 0 //(_fminf(iFrame,0))

__DEVICE__ float3 N33(float3 p) {
    float3 a = fract_f3(p*to_float3(123.34f,234.34f,345.65f));
    a+=dot(a,a+34.45f);
    return fract_f3(to_float3(a.x*a.y,a.y*a.z,a.z*a.x));
}

__DEVICE__ float smin( float a, float b, float k ) {
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ float hash(float2 co){
    return fract(_sinf(dot(swi2(co,x,y) ,to_float2(12.9898f,78.233f))) * 43758.5453f);
}

__DEVICE__ float metaball(float3 p, float i, float t) {
    float3 n = N33(to_float3_s(i));
    float3 p2 = sin_f3(n*t)*0.2f;
    float3 spp = p-p2;
    float sp = length(spp)-0.01f;
    return sp;
}

__DEVICE__ float2 GetDist(float3 p, float iTime) {

    float k = 0.7f;
    float d = 10.0f;
    float t = iTime*3.0f;

    d = smin(d,metaball(p,0.3f, t),k); 
    d = smin(d,metaball(p,0.6f, t),k); 
    d = smin(d,metaball(p,0.9f, t),k); 

    float2 model = to_float2(d,MATERIAL);
    return model;
}

__DEVICE__ float2 RayMarch(float3 ro, float3 rd, float side, int stepnum, float iTime) {
    float2 dO = to_float2_s(0.0f);
    
    for(int i=0; i<stepnum; i++) {
        float3 p = ro + rd*dO.x;
        float2 dS = GetDist(p,iTime);
        dO.x += dS.x*side;
        dO.y = dS.y;
        
        if(dO.x>MAX_DIST || _fabs(dS.x)<SURF_DIST) break;
    }
    
    return dO;
}

__DEVICE__ float3 GetNormal(float3 p, float iTime) {
    float d = GetDist(p,iTime).x;
    float2 e = to_float2(0.001f, 0);
    
    float3 n = d - to_float3(
        GetDist(p-swi3(e,x,y,y),iTime).x,
        GetDist(p-swi3(e,y,x,y),iTime).x,
        GetDist(p-swi3(e,y,y,x),iTime).x);
    
    return normalize(n);
}

__DEVICE__ float3 R(float2 uv, float3 p, float3 l, float z) {
    float3 f = normalize(l-p),
        r = normalize(cross(to_float3(0,1,0), f)),
        u = cross(f,r),
        c = p+f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i-p);
    return d;
}

// thx iq! https://iquilezles.org/articles/distfunctions2d/
__DEVICE__ float sdBox( in float2 p, in float2 b )
{
    float2 d = abs_f2(p)-b;
    return length(_fmaxf(d,to_float2_s(0.0f))) + _fminf(max(d.x,d.y),0.0f);
}


__DEVICE__ float charS(float2 p){
    float2 prevP = p;
    p.y*=1.5f;
    float d = _fabs(length(p-to_float2(-0.02f,0.06f))-0.06f)-0.02f;
    float d2 = B(p-to_float2(0.03f,0.02f),to_float2(0.045f,0.04f));
    d = _fmaxf(-d2,d);
    
    d2 = _fabs(length(p-to_float2(-0.02f,-0.06f))-0.06f)-0.02f;
    float d3 = B(p-to_float2(-0.06f,-0.02f),to_float2(0.045f,0.04f));
    d2 = _fmaxf(-d3,d2);
    
    d = _fminf(d,d2);
    return d;
}

__DEVICE__ float charH(float2 p){
    float2 prevP = p;
    p.y*=1.5f;
    p.x = _fabs(p.x)-0.06f;
    float d = B(p,to_float2(0.02f,0.14f));
    p = prevP;
    float d2 = B(p,to_float2(0.08f,0.02f));
    d = _fminf(d,d2);
    return d;
}

__DEVICE__ float charA(float2 p){
    float2 prevP = p;
    p.y*=1.5f;
    p.x = _fabs(p.x)-0.04f;
    p=mul_f2_mat2(p,Rot(radians(-15.0f)));
    float d = B(p,to_float2(0.02f,0.16f));
    p = prevP;
    float d2 = B(p-to_float2(0.0f,-0.03f),to_float2(0.05f,0.02f));
    d = _fminf(d,d2);
    d = _fmaxf((_fabs(p.y)-0.09f),d);
    p = prevP;
    p=mul_f2_mat2(p,Rot(radians(22.0f)));
    d2 =  B(p-to_float2(-0.037f,-0.12f),to_float2(0.019f,0.12f));
    return d;
}

__DEVICE__ float charD(float2 p){
    float2 prevP = p;
    p.y*=1.5f;
    float d = _fabs(sdBox(p,to_float2(0.02f,0.075f))-0.04f)-0.02f;
    d = _fmaxf(-p.x-0.03f,d);
    float d2 = B(p-to_float2(-0.05f,0.0f),to_float2(0.02f,0.135f));
    d = _fminf(d,d2);
    return d;
}

__DEVICE__ float charE(float2 p){
    float2 prevP = p;
    p.y*=1.5f;
    float d = B(p,to_float2(0.065f,0.13f));
    p.y = _fabs(p.y)-0.055f;
    float d2 = B(p-to_float2(0.03f,0.0f),to_float2(0.065f,0.03f));
    d = _fmaxf(-d2,d);
    return d;
}

__DEVICE__ float charR(float2 p){
    float2 prevP = p;
    p.y*=1.5f;
    float d = _fabs(sdBox(p-to_float2(-0.01f,0.05f),to_float2(0.03f,0.022f))-0.04f)-0.023f;
    d = _fmaxf(-p.x-0.03f,d);
    float d2 = B(p-to_float2(-0.05f,0.0f),to_float2(0.02f,0.135f));
    d = _fminf(d,d2);
    p=mul_f2_mat2(p,Rot(radians(-20.0f)));
    d2 = B(p-to_float2(0.02f,-0.14f),to_float2(0.02f,0.13f));
    p=mul_f2_mat2(p,Rot(radians(20.0f)));
    d2 = _fmaxf(-p.y-0.132f,d2);
    
    d = _fminf(d,d2);
    return d;
}

__DEVICE__ float shaderText(float2 p){
    float2 prevP = p;
    p.y*=0.9f;
    float d = charS(p-to_float2(-0.5f,0.0f));
    float d2 = charH(p-to_float2(-0.34f,0.0f));
    d = _fminf(d,d2);
    d2 = charA(p-to_float2(-0.14f,0.0f));
    d = _fminf(d,d2);
    d2 = charD(p-to_float2(0.05f,0.0f));
    d = _fminf(d,d2);
    d2 = charE(p-to_float2(0.22f,0.0f));
    d = _fminf(d,d2);
    d2 = charR(p-to_float2(0.38f,0.0f));
    d = _fminf(d,d2);
    return d;
}

//--------------------------------------------------------------
__DEVICE__ float sdFont(float2 p, int c, __TEXTURE2D__ iChannel0) {
    float2 uv = (p + to_float2(float(c%16), float(15-c/16)) + 0.5f)/16.0f;
    return _fmaxf(_fmaxf(_fabs(p.x) - 0.25f, _fmaxf(p.y - 0.35f, -0.38f - p.y)), texture(iChannel0, uv).w - 127.0f/255.0f);
}

__DEVICE__ float sdMessage(float2 p, float scale, __TEXTURE2D__ iChannel0) { 
    p /= scale;
    
    //int txt[] = {74,105,80,105,32,32};
    int txt[] = {110,109,98,114,55,51};
    
    float d;
    d = sdFont(p, txt[0],iChannel0);
    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[1],iChannel0));
    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[2],iChannel0));
    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[3],iChannel0));
    
    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[4],iChannel0));

    p.x-=0.5f;
    d = _fminf(d, sdFont(p, txt[5],iChannel0));
    
    return d*scale;
}
//--------------------------------------------------------------




__DEVICE__ float3 drawBg(float2 p, float3 col, float2 iResolution, float scale, __TEXTURE2D__ iChannel0){
    //float d = shaderText(p*1.7f);
    float d = sdMessage(p*1.7f, scale, iChannel0);
    col = _mix(col,to_float3_s(0.0f),S(d,0.0f));
    return col;
}

__DEVICE__ float3 reflectMaterial(float3 p, float3 rd, float3 n, float2 iResolution,float iTime, float scale, __TEXTURE2D__ iChannel0) {
    float3 r = reflect(rd,n);
    
    float3 refTex = drawBg(swi2(p,x,y),to_float3_s(_fmaxf(0.95f,r.y)),iResolution,scale,iChannel0)+(r*_sinf(iTime)*0.5f);

    return refTex;
}

__DEVICE__ float3 materials(int mat, float3 n, float3 rd, float3 p, float3 col, float2 iResolution,float iTime, float scale, __TEXTURE2D__ iChannel0){
    if(mat == MATERIAL){
        col = reflectMaterial(p,rd,n,iResolution,iTime,scale,iChannel0);
    }
    return col;
}

__KERNEL__ void SoapMetabubbleFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
    CONNECT_SLIDER0(scale, 0.0f, 5.0f, 1.0f);
    CONNECT_POINT0(TextOff, 0.0f, 0.0f);

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 prevUV = uv;
    float2 m =  swi2(iMouse,x,y)/iResolution;
    
    float3 ro = to_float3(0, 0, -1.0f);
    
    float3 rd = R(uv, ro, to_float3(0,0.0f,0.0f), 1.0f);
    float2 d = RayMarch(ro, rd, 1.0f,MAX_STEPS,iTime);
    float3 col = to_float3_s(1.0f);
    
    if(d.x<MAX_DIST) {
        float3 p = ro + rd * d.x;
        float3 n = GetNormal(p,iTime);
        int mat = (int)(d.y);
        col = materials(mat,n,rd,p+to_float3_aw(TextOff,0.0f),col,iResolution,iTime,scale, iChannel0);
    } else {
        col = drawBg(uv+TextOff,col,iResolution,scale,iChannel0);
    }
    
    // gamma correction
    col = pow_f3( col, to_float3_s(0.9545f) );    

    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
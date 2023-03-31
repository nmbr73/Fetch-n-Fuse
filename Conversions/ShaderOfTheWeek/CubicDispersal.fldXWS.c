
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


//Building on ideas from
//https://www.shadertoy.com/view/fd3SRN
//https://www.shadertoy.com/view/fsySWm
//https://www.shadertoy.com/view/stdGz4
//https://www.shadertoy.com/view/7sKGRy
//https://www.shadertoy.com/view/fsyGD3
//https://www.shadertoy.com/view/fdyGDt
//https://www.shadertoy.com/view/7dVGDd
//https://www.shadertoy.com/view/NsKGDy

//I had some plans to make a more elaborate shape using the "fully animated subdivision"
//but it ended up not looking that interesting when applied to an octree and it's too
//expensive to make shapes out of multiple "sheets" of this.

//I hope you enjoy it none the less :) 
//(sorry if it's expensive I didn't do much opmimizing)

#define MDIST 150.0f
#define STEPS 164.0f
#define pi 3.1415926535f
#define pmod(p,x) (mod_f(p,x)+0.5f*(x))
#define rot(a) to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))

//this is a useless trick but it's funny
#define vmm(v,minOrMax) minOrMax(v.x,minOrMax(v.y,v.z))

//iq box sdf
__DEVICE__ float ebox( float3 p, float3 b ){
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(  _fmaxf(q.x, _fmaxf(q.y,q.z)) , (0.0f));
}
//iq palette
__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d ){
  return a + b*cos_f3(2.0f*pi*(c*t+d));
}
__DEVICE__ float h11 (float a) {
  return fract(_sinf((a)*12.9898f)*43758.5453123f);
}
//https://www.shadertoy.com/view/fdlSDl
__DEVICE__ float2 tanha_f2(float2 x) {
  float2 x2 = x*x;
  return clamp(x*(27.0f + x2)/(27.0f+9.0f*x2), -1.0f, 1.0f);
}
__DEVICE__ float tanha(float x) {
  float x2 = x*x;
  return clamp(x*(27.0f + x2)/(27.0f+9.0f*x2), -1.0f, 1.0f);
}

struct sdResult
{
    float2 center;
    float2 dim;
    float id;
    float vol;
};

__DEVICE__ struct sdResult subdiv(float2 p,float seed,float iTime){
    float2 dMin = to_float2_s(-10.0f);
    float2 dMax = to_float2_s(10.0f);
    float t = iTime*0.6f;
    float t2 = iTime;
    float2 dim = dMax - dMin;
    float id = 0.0f;
    float ITERS = 6.0f;
    
    float MIN_SIZE = 0.1f;
    float MIN_ITERS = 1.0f;
    
    //big thanks to @0b5vr for letting me use his cleaner subdiv implementation
    //https://www.shadertoy.com/view/NsKGDy
    float2 diff2 = to_float2_s(1);
    for(float i = 0.0f;i<ITERS;i+=1.0f){
        float2 divHash=tanha_f2(to_float2(_sinf(t2*pi/3.0f+id+i*t2*0.05f),_cosf(t2*pi/3.0f+h11(id)*100.0f+i*t2*0.05f))*3.0f)*0.35f+0.5f;
        //divHash=to_float2(_sinf(t*pi/3.0f+id),_cosf(t*pi/3.0f+h11(id)*100.0f))*0.5f+0.5f;
        //if(iMouse.z>0.5f){divHash = _mix(divHash,M,0.9f);}
        divHash = _mix(to_float2_s(0.5f),divHash,tanha(_sinf(t*0.8f)*5.0f)*0.2f+0.4f);
        float2 divide = divHash * dim + dMin;
        divide = clamp(divide, dMin + MIN_SIZE+0.01f, dMax - MIN_SIZE-0.01f);
        float2 minAxis = _fminf(abs_f2(dMin - divide), abs_f2(dMax - divide));
        float minSize = _fminf( minAxis.x, minAxis.y);
        bool smallEnough = minSize < MIN_SIZE;
        if (smallEnough && i + 1.0f > MIN_ITERS) { break; }
        dMax = mix_f2( dMax, divide, step( p, divide ));
        dMin = mix_f2( divide, dMin, step( p, divide ));
        diff2 =step( p, divide)-
        to_float2(h11(diff2.x+seed)*10.0f,h11(diff2.y+seed)*10.0f);
        id = length(diff2)*100.0f;
        dim = dMax - dMin;
    }
    float2 center = (dMin + dMax)/2.0f;
    struct sdResult result;
    result.center = center;
    result.id = id;
    result.dim = dim;
    result.vol = dim.x*dim.y;
    return result;
}

__DEVICE__ float dibox(float3 p,float3 b,float3 rd){
    float3 dir = sign_f3(rd)*b;   
    float3 rc = (dir-p)/rd;
    return _fminf(rc.x,rc.z)+0.01f; 
}

__DEVICE__ float3 map(float3 p,float iTime, float3 rdg, bool traverse){
    float seed = sign_f(p.y)-0.3f;
    seed = 1.0f;
    //p.y = _fabs(p.y)-4.0f;

    float2 a = to_float2(99999,1);
    float2 b = to_float2_s(2);
    
    a.x = p.y-2.0f;
    float id = 0.0f;
    if(a.x<0.1||!traverse){
        float t = iTime;
        struct sdResult sdr = subdiv(swi2(p,x,z),seed,iTime);
        float3 centerOff = to_float3(sdr.center.x,0,sdr.center.y);
        float2 dim = sdr.dim;

        float rnd = 0.05f;
        float size = _fminf(dim.y,dim.x)*1.0f;
        //size = 1.0f;
        size+=(_sinf((centerOff.x+centerOff.z)*0.6f+t*4.5f)*0.5f+0.5f)*2.0f;
        size = _fminf(size,4.0f);
        a.x = ebox(p-centerOff-to_float3(0,0,0),to_float3(dim.x,size,dim.y)*0.5f-rnd)-rnd;
        if(traverse){
            b.x = dibox(p-centerOff,to_float3(dim.x,1,dim.y)*0.5f,rdg);
            a = (a.x<b.x)?a:b;
        }
        id = sdr.id;
    }
    return to_float3_aw(a,id);
}
__DEVICE__ float3 norm(float3 p,float iTime, float3 rdg, bool traverse){
    float2 e = to_float2(0.01f,0.0f);
    return normalize(map(p,iTime,rdg,traverse).x-to_float3(
                     map(p-swi3(e,x,y,y),iTime,rdg,traverse).x,
                     map(p-swi3(e,y,x,y),iTime,rdg,traverse).x,
                     map(p-swi3(e,y,y,x),iTime,rdg,traverse).x));
}

__KERNEL__ void CubicDispersalFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
  
    CONNECT_COLOR0(ColorAL, 0.0f, 0.33f, 0.66f, 1.0f); 
    CONNECT_COLOR1(Color1, 0.204f, 0.267f, 0.373f, 1.0f); 
    CONNECT_COLOR2(Color2, 0.169f, 0.000f, 0.169f, 1.0f); 
    CONNECT_COLOR3(Color3, 0.984f, 0.996f, 0.804f, 1.0f); 
    CONNECT_COLOR4(ColorBKG1, 0.373f, 0.835f, 0.988f, 1.0f); 
    CONNECT_COLOR5(ColorBKG2, 0.424f, 0.059f, 0.925f, 1.0f); 
    CONNECT_SLIDER0(Brightness, 0.0f, 2.0f, 1.05f);
    
    CONNECT_SLIDER1(mix_ao, -1.0f, 2.0f, 0.65f);
    CONNECT_SLIDER2(mix_col, 0.0f, 1.0f, 0.5f);
    CONNECT_SLIDER3(diff1, 0.0f, 3.0f, 0.7f);
    CONNECT_SLIDER4(diff2, 0.0f, 3.0f, 0.3f);
    
    CONNECT_SLIDER5(ID, 0.0f, 1000.0f, 0.0f);
    CONNECT_SLIDER6(IDOff, 0.0f, 100.0f, 1.0f);
    CONNECT_POINT0(TexOff, 0.0f, 0.0f);
    CONNECT_SLIDER7(TexScale, 0.0f, 10.0f, 1.0f);
  
    mat2 unused;
  
    bool traverse = true;

    float2 R = iResolution;
    float2 uv = (fragCoord-0.5f*swi2(R,x,y))/R.y;
    float3 col = to_float3_s(0);
    
    float3 ro = to_float3(0,6.0f,-12)*1.2f;

    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , rot(0.35f)));
    float3 lk = to_float3(-1,-3,0.5f);
    if(iMouse.z>0.0f){
       ro*=2.0f;
       lk = to_float3_s(0);
       swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z),rot(2.0f*(iMouse.y/iResolution.y-0.5f))));
       swi2S(ro,z,x, mul_f2_mat2(swi2(ro,z,x),rot(-9.0f*(iMouse.x/iResolution.x-0.5f))));
    }
    
    float3 f = (normalize(lk-ro));
    float3 r = normalize(cross(to_float3(0,1,0),f));
    float3 rd = normalize(f*(1.8f)+r*uv.x+uv.y*cross(f,r));
    float3 rdg = rd;
    float3 p = ro;
    float dO = 0.0f;
    float3 d;
    bool hit = false;
        
    for(float i = 0.0f; i<STEPS; i+=1.0f){
        p = ro+rd*dO;
        d = map(p,iTime,rdg,traverse);
        dO+=d.x;
        if(d.x<0.005f){
            hit = true;
            break;
        }
        if(dO>MDIST)break;
    }
    
    if(hit&&d.y!=2.0f){
        traverse = false;
        float3 n = norm(p,iTime,rdg,traverse);
        float3 r = reflect(rd,n);
        float3 e = to_float3_s(0.5f);
        //float3 al = pal(fract(d.z)*0.35f-0.8f,e*1.2f,e,e*2.0f,to_float3(0,0.33f,0.66f));
        float3 al = pal(fract(d.z)*0.35f-0.8f,e*1.2f,e,e*2.0f,swi3(ColorAL,x,y,z));
        col = al;
        float3 ld = normalize(to_float3(0,45,0)-p);

        //sss from nusan
        float sss=0.1f;
        float sssteps = 10.0f;
        for(float i=1.0f; i<sssteps; i+=1.0f){
            float dist = i*0.2f;
            sss += smoothstep(0.0f,1.0f,map(p+ld*dist,iTime,rdg,traverse).x/dist)/(sssteps*1.5f);
        }
        sss = clamp(sss,0.0f,1.0f);
        
        //float diff = _fmaxf(0.0f,dot(n,ld))*0.7f+0.3f;
        float diff = _fmaxf(0.0f,dot(n,ld))*diff1+diff2;
        float amb = dot(n,ld)*0.45f+0.55f;
        float spec = _powf(_fmaxf(0.0f,dot(r,ld)),13.0f);
        //blackle ao 
        #define AO(a,n,p) smoothstep(-a,a,map(p+n*a,iTime,rdg,traverse).x)
        float ao = AO(0.1f,n,p)*AO(0.2f,n,p)*AO(0.3f,n,p);

        spec = smoothstep(0.0f,1.0f,spec);
        //col = to_float3(0.204f,0.267f,0.373f)*
        //                _mix(to_float3(0.169f,0.000f,0.169f),to_float3(0.984f,0.996f,0.804f),_mix(amb,diff,0.75f))
        //                +spec*0.3f;
        col = swi3(Color1,x,y,z)*
                   _mix(swi3(Color2,x,y,z),swi3(Color3,x,y,z),_mix(amb,diff,0.75f))
                   +spec*0.3f;
        
        
        col+=sss*al;
        col*=_mix(ao,1.0f,mix_ao);//0.65f);
        col = pow_f3(col,to_float3_s(0.85f));
        
        float2 tuv = to_float2(p.x+d.x,p.z+d.y)*TexScale+TexOff;//to_float2(p.x+TexOff.x,p.z+TexOff.y)*TexScale;
        if (d.z > ID-IDOff && d.z < ID+IDOff) col = swi3(_tex2DVecN(iChannel0,tuv.x,tuv.y,15),x,y,z); 
    }
    else{
        //col = _mix(to_float3(0.373f,0.835f,0.988f),to_float3(0.424f,0.059f,0.925f),length(uv));
        col = _mix(swi3(ColorBKG1,x,y,z),swi3(ColorBKG2,x,y,z),length(uv));
    }
    
    col *=1.0f-0.5f*_powf(length(uv*to_float2(0.8f,1.0f)),2.7f);
    float3 col2 = smoothstep(to_float3(0.0f, 0.0f, 0.0f), to_float3(1.1f, 1.1f, 1.3f), col);
    //col = _mix(col,col2,0.5f)*Brightness;//1.05f;
    col = _mix(col,col2,mix_col)*Brightness;//1.05f;

    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
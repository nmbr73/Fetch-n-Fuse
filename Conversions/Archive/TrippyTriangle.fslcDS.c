
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define STEPS 250.0f
#define MDIST 100.0f
#define pi 3.1415926535f
#define rot(a) to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))
#define sat(a) clamp(a,0.0f,1.0f)

//Comment to remove triangle wobble
#define WOBBLE 

//ADJUST AA HERE
//#define AA 2.0f

//Camera Control
#define CAM

//based on ideas from 
//https://www.shadertoy.com/view/fsVSzw
//https://www.shadertoy.com/view/MscSDB
//https://www.shadertoy.com/view/3ddGzn
#define h13(n) fract_f3((n)*to_float3(12.9898f,78.233f,45.6114f)*43758.5453123f)
__DEVICE__ float2 vor(float2 v, float3 p, float3 s){
    p = abs_f3(fract_f3(p-s)-0.5f);
    float a = _fmaxf(p.x,_fmaxf(p.y,p.z));
    float b = _fminf(v.x,a);
    float c = _fmaxf(v.x,_fminf(v.y,a));
    return to_float2(b,c);
}

__DEVICE__ float vorMap(float3 p){
    float2 v = to_float2_s(5.0f);
    v = vor(v,p,h13(0.96f));
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y),rot(1.2f)));
    v = vor(v,p,h13(0.55f));
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z),rot(2.0f)));
    v = vor(v,p,h13(0.718f));
    swi2S(p,z,x, mul_f2_mat2(swi2(p,z,x),rot(2.7f)));
    v = vor(v,p,h13(0.3f));
    return v.y-v.x; 
}

//box sdf
__DEVICE__ float box(float3 p, float3 b){
  float3 q = abs_f3(p)-b;
  return length(_fmaxf(q,to_float3_s(0.0f)))+_fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}


//float va = 0.0f; //voronoi animations
//float sa = 0.0f; //size change animation
//float rlg; //global ray length
//bool hitonce = false; //for tracking complications with the voronoi 


//I put quite a lot of effort into making the normals inside the voronoi correct but 
//in the end the normals are only partially correct and I barely used them, however
//the code is still messy from my failed attempt :)
__DEVICE__ float2 map(float3 p, float3 n, float iTime, float va, float sa, float rlg, bool hitonce){
    float2 a = to_float2_s(1);
    float2 b = to_float2_s(2);
    float3 po = p;
    float3 no = n;
    p-=n;
    float len = 9.5f;
    len+=sa;
    float len2 = len-1.0f;
    p.x-=(len/2.0f);
    a.x = box(p,to_float3(1,1,len));
    a.x = _fminf(a.x,box(p-to_float3(0,len2,len2),to_float3(1,len,1)));
    a.x = _fminf(a.x,box(p-to_float3(-len2,0,-len2),to_float3(len,1,1)));
    float tip = box(p-to_float3(len2,len2*2.0f,len2),to_float3(len2,1,1));   
    
    //float cut = (p.xz*=rot(pi/4.0-0.15)).y; //ORG

    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot(pi/4.0f-0.15f)));
    float cut = (swi2(p,x,z)).y;
    
    tip = _fmaxf(-cut+len2/2.0f,tip);
    a.x = _fminf(a.x,tip);
    b.x = tip;
    a.x-=0.4f;
    p = po;
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot(pi/4.0f)));
    swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y),rot(-0.9553155f)));
    po = p;
    swi2S(n,x,z, mul_f2_mat2(swi2(n,x,z),rot(pi/4.0f)));
    swi2S(n,x,y, mul_f2_mat2(swi2(n,x,y),rot(-0.9553155f)));
    swi2S(p,x,z, swi2(p,x,z)-swi2(n,x,y));
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot(-iTime*0.3f)));
    if(hitonce)  a.x = _fmaxf(a.x,-vorMap(to_float3(p.x,p.z,rlg+n.z)*0.35f+3.0f)+va*1.6f);
    p = po;
    b.y = 3.0f;
    p-=n;
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot(pi/6.0f)));
    p.x+=1.75f;
    p.z+=0.4f;
    po = p;
    for(float i = 0.0f; i<3.0f; i+=1.0f){ //blocks
        b.y+=i;
        swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot((2.0f*pi/3.0f)*i)));
        float t = (iTime+i*((2.0f*pi)/9.0f))*3.0f;
        p.y-=35.0f-50.0f*step(_sinf(t),0.0f);
        p.x+=4.5f;
        swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y),rot(t)));
        p.x-=4.5f;
        swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot(t)));
        b.x = box(p,to_float3(1.5f,0.5f,0.5f))-0.25f;
        a = (a.x<b.x)?a:b;
        p = po;
    }
    return a;
}
__DEVICE__ float3 norm(float3 p,float iTime, float va, float sa, float rlg, bool hitonce){
    float2 e= to_float2(0.0001f,0);
    return normalize(map(p,to_float3_s(0),iTime,va,sa,rlg,hitonce).x-to_float3(
                     map(p,swi3(e,x,y,y),iTime,va,sa,rlg,hitonce).x,
                     map(p,swi3(e,y,x,y),iTime,va,sa,rlg,hitonce).x,
                     map(p,swi3(e,y,y,x),iTime,va,sa,rlg,hitonce).x));
}

__DEVICE__ void render(out float4 *fragColor, in float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float *rlg, bool hitonce){
    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float3 col = to_float3_s(0);
    uv.x-=0.025f;
    float2 uv2 = uv;
    float2 uv3 = uv;
    
    //Calculating the animation for the size wobble and voronoi crumble
    uv2.y-=0.1f;
    uv2=mul_f2_mat2(uv2,rot(iTime*1.25f));
    float ang =_atan2f(uv2.x,uv2.y)/(pi*2.0f)+0.5f;
    float range = 0.175f;
    float sa = 0.0f;
    #ifdef WOBBLE
    sa = _sinf(ang*10.0f+iTime*2.5f)*0.3f;
    #endif
    ang = smoothstep(0.0f,range,ang)*smoothstep(0.0f,range,1.0f-ang);
    float va = (1.0f-ang)*0.175f;
    uv=mul_f2_mat2(uv,rot(-pi/6.0f));
    
    float3 ro = to_float3(5,5,5)*6.5f;
    
    #ifdef CAM
    if(iMouse.z>0.0f){
        swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z),rot(2.0f*(iMouse.y/iResolution.y-0.5f))))
        swi2S(ro,z,x, mul_f2_mat2(swi2(ro,z,x),rot(-7.0f*(iMouse.x/iResolution.x-0.5f))))
    }    
    #endif
    
    //maybe there is an easier way to make an orthographic target camera
    //but this is what I figured out
    float3 lk = to_float3(0,0,0);
    float3 f = normalize(lk-ro);
    float3 r = normalize(cross(to_float3(0,1,0),f));
    float3 rd = f+uv.x*r+uv.y*cross(f,r);
    ro+=(rd-f)*17.0f;
    rd=f;

    float3 p = ro;
    float rl = 0.0f;
    float2 d= to_float2_s(0);
    float shad = 0.0f;
    float rlh = 0.0f;
    float i2 = 0.0f; 
    
    //Spaghetified raymarcher 
    for(float i = 0.0f; i<STEPS; i+=1.0f){
        p = ro+rd*rl;
        d = map(p, to_float3_s(0),iTime,va,sa,*rlg,hitonce);
        rl+=d.x;
        if((d.x)<0.0001f){
            shad = i2/STEPS;
            if(hitonce)break;
            hitonce = true;
            rlh = rl;
        }
        if(rl>MDIST||(!hitonce&&i>STEPS-2.0f)){
            d.y = 0.0f;
            break;
        }
        *rlg = rl-rlh;
        if(hitonce&&*rlg>3.0f){hitonce = false; i2 = 0.0f;}  
        if(hitonce) i2+=1.0f;
    }
    //Color Surface
    if(d.y>0.0f){
        float3 n = norm(p,iTime,va,sa,*rlg,hitonce);
        float3 r = reflect(rd,n);
        float3 ld = normalize(to_float3(0,1,0));
        float spec = _powf(_fmaxf(0.0f,dot(r,ld)),13.0f);

        //Color the triangle
        float3 n2 = n*0.65f+0.35f;
        col += _mix(to_float3(1,0,0),to_float3(0,1,0),sat(uv3.y*1.1f))*n2.x;
        uv3=mul_f2_mat2(uv3,rot(-(2.0f*pi)/3.0f));
        col += _mix(to_float3(0,1.0f,0),to_float3(0,0,1),sat(uv3.y*1.1f))*n2.y;
        uv3=mul_f2_mat2(uv3,rot(-(2.0f*pi)/3.0f));
        col += _mix(to_float3(0,0,1),to_float3(1,0,0),sat(uv3.y*1.1f))*n2.z;
        

        
        //NuSan SSS
        float sss=0.5f;
        float sssteps = 10.0f;
        for(float i=1.0f; i<sssteps; i+=1.0f){
            float dist = i*0.2f;
            sss += smoothstep(0.0f,1.0f,map(p+ld*dist,to_float3_s(0),iTime,va,sa,*rlg,hitonce).x/dist)/(sssteps*1.5f);
        }
        sss = clamp(sss,0.0f,1.0f);
        
        //blackle AO
        #define AO(a,n,p) smoothstep(-a,a,map(p,-n*a,iTime,va,sa,*rlg,hitonce).x)
        float ao = AO(1.9f,n,p)*AO(3.0f,n,p)*AO(7.0f,n,p);
        
        //Apply AO on the triangle
        if(*rlg<0.001f){
            col*=_mix(ao,1.0f,0.2f);
        }
        //Color the inside of the crumbled bits 
        else {
            col = to_float3_s(0.2f-shad);
        }
        //Color the moving blocks
        if(d.y>1.0f){
            col = (n*0.6f+0.4f)*to_float3_s(sss)+spec;
        }
        //a bit of gamma correction
        col = pow_f3(col,to_float3_s(0.7f));
    }
    //Color Background
    else{
        float3 bg = _mix(to_float3(0.345f,0.780f,0.988f),to_float3(0.361f,0.020f,0.839f),length(uv));
        col = bg;
    }
    *fragColor = to_float4_aw(col,1.0f);
}

//External AA, (I compacted it for fun)
__KERNEL__ void TrippyTriangleFuse(float4 O, float2 C, float iTime, float2 iResolution, float4 iMouse)
{
  
    CONNECT_SLIDER0(AA, 1.0f, 5.0f, 1.0f);
  
    mat2 temp = to_mat2(1,2,3,4);
  
    float rlg=0.0f; //global ray length
    bool hitonce = false; //for tracking complications with the voronoi

    float px=1.0f/AA,i,j;float4 cl2,cl=to_float4_s(0.0f);
    if(AA==1.0f)  {render(&cl,C,iTime,iResolution,iMouse,&rlg,hitonce); O=cl; SetFragmentShaderComputedColor(O); return;}
    
    for(i=0.0f;i<AA +_fminf(iTime,0.0f);i+=1.0f){for(j=0.0f;j<AA;j+=1.0f){
      float2 C2 = to_float2(C.x+px*i,C.y+px*j);
      render(&cl2,C2,iTime,iResolution,iMouse,&rlg,hitonce);
      cl+=cl2;
      rlg=0.0f; 
      hitonce = false;
    }}
    cl/=AA*AA; O=cl;

  SetFragmentShaderComputedColor(O);
}
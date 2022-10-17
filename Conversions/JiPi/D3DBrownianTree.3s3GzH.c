
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//const int NUM_BALLS=1000;//change this to whatever your computer can handle
//const float SPHERE_RANGE=5.0f;

//const float MIN_SIZE=0.01f;
//const float MAX_SIZE=0.1f;

//Hashes from David Hoskins at https://www.shadertoy.com/view/4djSRW
__DEVICE__ float hash13(float3 p3){
  p3  = fract_f3(p3 * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float3 hash33(float3 p3){
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract_f3((swi3(p3,x,x,y) + swi3(p3,y,x,x))*swi3(p3,z,y,x));

}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


//Ball Motion

//nudge balls around until they are touching the center structure
//when they touch the center structure they become part of it
//ball size is based on distance to center
//balls have a bias with their random motion to move towards the center
__KERNEL__ void D3DBrownianTreeFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, float iTime, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_INTSLIDER0(NUM_BALLS, 0, 3000, 1000);
    CONNECT_SLIDER0(SPHERE_RANGE, 0.0f, 10.0f, 5.0f);
    CONNECT_SLIDER1(MIN_SIZE, -1.0f, 1.0f, 0.01f);
    CONNECT_SLIDER2(MAX_SIZE, -1.0f, 1.0f, 0.1f);
  
    fragCoord+=0.5f;
 float AAAAAAAAAAAAAAA;
    int md=(int)(fragCoord.x+_floor(fragCoord.y)*iResolution.x);
    int irx=(int)(iResolution.x);
    float rC=length(swi2(texture(iChannel0,(make_float2(to_int2(0,0))+0.5f)/iResolution),x,y)-iResolution);
    if(iFrame==0||rC>0.0f||Reset){
        fragColor = to_float4_aw((0.5f*hash13(to_float3_aw(fragCoord,(int)(iTime)))+0.5f)*normalize(hash33(to_float3_aw(fragCoord,(int)(iTime)))-0.5f),-0.02f);
        if(md==1){fragColor=to_float4(0.0f,0.0f,0.0f,MAX_SIZE);}
        if(md==0){fragColor=to_float4(iResolution.x,iResolution.y,0.0f,0.0f);}
    }else if(md<=NUM_BALLS&&md>0){
        float d=2e20;
        float4 p=texture(iChannel0,(make_float2(to_int2_cfloat(fragCoord))+0.5f)/iResolution);
        if(p.w>0.0f){
          fragColor=p;
          SetFragmentShaderComputedColor(fragColor);
          return;
        }
        swi3S(p,x,y,z, swi3(p,x,y,z) + p.w*(hash33(to_float3_aw(fragCoord,iFrame+(int)(iTime)))-0.5f));
        //contain within range
        if(length(swi3(p,x,y,z))>1.0f) {swi3S(p,x,y,z, normalize(swi3(p,x,y,z)));}
        swi3S(p,x,y,z, swi3(p,x,y,z)*0.999f);
        p.w=-MAX_SIZE/(((MAX_SIZE/MIN_SIZE)-1.0f)*_powf(length(swi3(p,x,y,z)),2.0f)+1.0f);
        //ball tests
        for(int i=1;i<=NUM_BALLS;i++){
            float4 s=texture(iChannel0,(make_float2(to_int2(i%irx,i/irx))+0.5f)/iResolution);
            //if sign is negative don't care about collision
            //negative sign means the ball is not added to the structure yet
            //also handles self check with the 0
            d=_fminf(d*sign_f(s.w),length(swi3(p,x,y,z)-swi3(s,x,y,z))-_fabs(s.w))*sign_f(s.w);
        }
        
        //did it collide?
        if(d<_fabs(p.w)){
            fragColor=to_float4_aw(swi3(p,x,y,z),_fabs(p.w));
        }else{
            fragColor=p;
        }
    }else if(md==0){
        fragColor=to_float4(iResolution.x,iResolution.y,0.0f,0.0f);
    }else{
      fragColor=to_float4_s(-1.0f);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


//Camera Rotation, Click And Drag With Velocity

__KERNEL__ void D3DBrownianTreeFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_INTSLIDER0(NUM_BALLS, 0, 3000, 1000);
    CONNECT_SLIDER0(SPHERE_RANGE, 0.0f, 10.0f, 5.0f);
    CONNECT_SLIDER1(MIN_SIZE, -1.0f, 1.0f, 0.01f);
    CONNECT_SLIDER2(MAX_SIZE, -1.0f, 1.0f, 0.1f); 
  
    fragCoord+=0.5f;
 
    if(iFrame==0 || Reset){
        fragColor=to_float4(0.0f,0.0f,0.0f,1.0f);
        //initialize velocity
        if(fragCoord.x>1.0f&&fragCoord.x<3.0f&&fragCoord.y<1.0f){
          fragColor=to_float4(0.08f,0.02f,0.0f,0.0f);
        }
    }else{
        if(fragCoord.x<1.0f&&fragCoord.y<1.0f){
            //float4 p=texelFetch(iChannel0,to_int2(0),0);
            //float4 v=texelFetch(iChannel0,to_int2(1,0),0)/3.0f;
            float4 p=texture(iChannel0,(make_float2(to_int2(0,0))+0.5f)/iResolution);
            float4 v=texture(iChannel0,(make_float2(to_int2(1,0))+0.5f)/iResolution)/3.0f;
            
            float2 rx = to_float2( _sinf(v.x/2.0f),_cosf(v.x/2.0f));
            float2 ry = to_float2(-_sinf(v.y/2.0f),_cosf(v.y/2.0f));
            //Quaternion multiplication simplification for basis elements
            float4 d=to_float4(-rx.y*ry.y,rx.x*ry.x,-rx.x*ry.y,rx.y*ry.x);
            //Full Quaternion multiplication
            fragColor = normalize(to_float4(d.x*p.x-d.y*p.y-d.z*p.z-d.w*p.w,
                                            d.x*p.y+d.y*p.x+d.z*p.w-d.w*p.z,
                                            d.x*p.z-d.y*p.w+d.z*p.x+d.w*p.y,
                                            d.x*p.w+d.y*p.z-d.z*p.y+d.w*p.x));
        }else if(fragCoord.x<2.0f&&fragCoord.y<1.0f){
            float4 v=texture(iChannel0,(make_float2(to_int2(1,0))+0.5f)/iResolution);
            float4 m=texture(iChannel0,(make_float2(to_int2(2,0))+0.5f)/iResolution);
            v=0.98f*v;
            if(m.z>0.5f){
              swi2S(v,x,y, swi2(v,x,y)+(swi2(iMouse,x,y)-swi2(m,x,y))/iResolution);
            }
            fragColor=v;
        }else if(fragCoord.x<3.0f&&fragCoord.y<1.0f){
            fragColor = iMouse;
        }else{
            fragColor = to_float4_s(0.0f);
        }
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


//3D Brownian Tree (sorta because there is a bias towards the center)
//Drag with the mouse to look-around



//Distance between sphere and ray if they intersect, -1 otherwise
__DEVICE__ float sphereRayDist(float3 o, float3 r, float4 s){
    float3 oc = o-swi3(s,x,y,z);
    float b = dot(oc,r);
    float c = dot(oc,oc)-s.w*s.w;
    float t = b*b-c;
    if( t > 0.0f) 
        t = -b - _sqrtf(t);
    return t;
}

//Thanks to iq for this helpful shadow function
__DEVICE__ float sphereRayShadow(float3 o,float3 r,float4 s,float k){
    float3 oc = o - swi3(s,x,y,z);
    float b = dot( oc, r );
    float c = dot( oc, oc ) - s.w*s.w;
    float h = b*b - c;
    return (b>0.0f) ? step(-0.0001f,c) : smoothstep( 0.0f, 1.0f, h*k/b );
}

//color palette code from IQ https://www.shadertoy.com/view/ll2GD3
__DEVICE__ float3 pal(float t,float3 a,float3 b,float3 c,float3 d){
    const float TAU=2.0f*_acosf(-1.0f);
  
    return a+b*cos_f3(TAU*(c*t+d));
}

//render scene with balls
__DEVICE__ float3 trace(float3 o,float3 r, float2 iResolution, int NUM_BALLS, float SPHERE_RANGE, __TEXTURE2D__ iChannel0){
    int irx=(int)(iResolution.x);
    bool hit=false;
    float md=2e20;
    float4 msp=to_float4_s(-1.0f);
    for(int i=1;i<=NUM_BALLS;i++){
        float4 sphere=SPHERE_RANGE*texture(iChannel0,(make_float2(to_int2(i%irx,i/irx))+0.5f)/iResolution);
        float d=sphereRayDist(o,r,sphere);
        if(d<md&&d>=0.0f){
          msp=sphere;
            md=d;
            hit=true;
        }
    }

    if(hit){
        float3 no=o+r*md;
        float3 c=pal(msp.w,to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,1.0f,1.0f),to_float3(0.0f,0.33f,0.67f) );
        float3 lp=to_float3(0.1f,0.1f,0);
        float l;
        l=dot(normalize(r-lp),normalize(swi3(msp,x,y,z)-(o+r*md)));
        l+=_powf(l,32.0f);
        r=-1.0f*normalize(r-lp);
        no+=r*0.001f;
        float shd=1.0f;
        for(int i=1;i<=NUM_BALLS;i++){
          float4 sphere=SPHERE_RANGE*texture(iChannel0,(make_float2(to_int2(i%irx,i/irx))+0.5f)/iResolution);
            shd*=sphereRayShadow(no,r,sphere,40.0f);
        }
        return c*l*shd;
    }else{
        //super lazy stars in the background
        return to_float3_s(hash13(round(r*100.0f))>0.99f);
    }
}

//Apply Quaternion Rotation to float3
__DEVICE__ float3 rot(float4 q,float3 r) {return 2.0f*cross(swi3(q,x,y,z),q.w*r+cross(swi3(q,x,y,z),r));}

__KERNEL__ void D3DBrownianTreeFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0); 
    CONNECT_INTSLIDER0(NUM_BALLS, 0, 3000, 1000);
    CONNECT_SLIDER0(SPHERE_RANGE, 0.0f, 10.0f, 5.0f);
    CONNECT_SLIDER1(MIN_SIZE, -1.0f, 1.0f, 0.01f);
    CONNECT_SLIDER2(MAX_SIZE, -1.0f, 1.0f, 0.1f);
    
  float IIIIIIIIIIIIIIIIIIIII;
    fragCoord+=0.5f;

    float2 uv=(2.0f*fragCoord-iResolution)/iResolution.y;
    float3 o = to_float3(0.0f,0.0f,-1.2f*SPHERE_RANGE);
    float3 r = normalize(to_float3_aw(uv,1.0f));
    float4 t=texture(iChannel1,(make_float2(to_int2(0,0))+0.5f)/iResolution);
    r+=rot(t,r);
    o+=rot(t,o);
    fragColor=to_float4_aw(trace(o,normalize(r),iResolution, NUM_BALLS,SPHERE_RANGE, iChannel0),1.0f);


  SetFragmentShaderComputedColor(fragColor);
}